#include "RpcIndexerService.h"

#include <QByteArray>
#include <QDateTime>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRegularExpression>
#include <QTimer>
#include <QTimeZone>

#include <algorithm>

namespace {

QString compactJson(const QJsonValue& value)
{
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    return {};
}

quint64 extractFinalizedBlockId(const QJsonValue& value)
{
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (number <= 0.0) {
            return 0;
        }
        return static_cast<quint64>(number);
    }

    if (value.isString()) {
        bool ok = false;
        const qulonglong parsed = value.toString().toULongLong(&ok);
        return ok ? parsed : 0;
    }

    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        if (obj.contains("block_id")) {
            return extractFinalizedBlockId(obj.value("block_id"));
        }
        if (obj.contains("blockId")) {
            return extractFinalizedBlockId(obj.value("blockId"));
        }
        if (obj.contains("id")) {
            return extractFinalizedBlockId(obj.value("id"));
        }
        if (obj.contains("result")) {
            return extractFinalizedBlockId(obj.value("result"));
        }
        if (obj.contains("header")) {
            return extractFinalizedBlockId(obj.value("header").toObject().value("block_id"));
        }
    }

    if (value.isArray()) {
        const QJsonArray arr = value.toArray();
        for (const auto& item : arr) {
            const quint64 blockId = extractFinalizedBlockId(item);
            if (blockId) {
                return blockId;
            }
        }
    }

    return 0;
}

} // namespace

RpcIndexerService::RpcIndexerService(const QString& endpoint, QObject* parent)
    : IndexerService(parent)
    , m_endpoint(QUrl(endpoint.trimmed()))
{
    connect(&m_socket, &QWebSocket::connected, this, &RpcIndexerService::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &RpcIndexerService::onDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &RpcIndexerService::onTextMessageReceived);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(&m_socket, &QWebSocket::errorOccurred, this, &RpcIndexerService::onSocketError);
#else
    connect(&m_socket,
        QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
        this,
        &RpcIndexerService::onSocketError);
#endif
}

RpcIndexerService::~RpcIndexerService()
{
    m_socket.close();
}

QString RpcIndexerService::endpoint() const
{
    return m_endpoint.toString();
}

void RpcIndexerService::setEndpoint(const QString& endpoint)
{
    const QString trimmed = endpoint.trimmed();
    const QUrl newEndpoint(trimmed.isEmpty() ? IndexerService::defaultEndpoint() : trimmed);
    if (!newEndpoint.isValid()) {
        qWarning() << "Invalid indexer endpoint URL:" << endpoint;
        return;
    }

    if (newEndpoint == m_endpoint) {
        return;
    }

    m_endpoint = newEndpoint;
    m_nextRequestId = 1;
    m_responses.clear();
    m_subscriptionRequestId = 0;
    m_subscriptionId = QJsonValue();
    m_subscriptionActive = false;
    m_lastNotifiedBlockId = 0;

    for (auto it = m_waiters.begin(); it != m_waiters.end(); ++it) {
        if (it.value()) {
            it.value()->quit();
        }
    }
    m_waiters.clear();

    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }
}

std::optional<Account> RpcIndexerService::getAccount(const QString& accountId)
{
    auto result = callRpc("getAccount", QJsonArray{accountId});
    if (!result) {
        return std::nullopt;
    }
    return parseAccount(accountId, *result);
}

std::optional<Block> RpcIndexerService::getBlockById(quint64 blockId)
{
    auto result = callRpc("getBlockById", QJsonArray{static_cast<qint64>(blockId)});
    if (!result || result->isNull()) {
        return std::nullopt;
    }
    return parseBlock(*result);
}

std::optional<Block> RpcIndexerService::getBlockByHash(const QString& hash)
{
    auto result = callRpc("getBlockByHash", QJsonArray{hash});
    if (!result || result->isNull()) {
        return std::nullopt;
    }
    return parseBlock(*result);
}

std::optional<Transaction> RpcIndexerService::getTransaction(const QString& hash)
{
    auto result = callRpc("getTransaction", QJsonArray{hash});
    if (!result || result->isNull()) {
        return std::nullopt;
    }
    return parseTransaction(*result);
}

QVector<Block> RpcIndexerService::getBlocks(std::optional<quint64> before, int limit)
{
    QJsonArray params;
    if (before.has_value()) {
        params.append(static_cast<qint64>(*before));
    } else {
        params.append(QJsonValue::Null);
    }
    params.append(limit);

    auto result = callRpc("getBlocks", params);
    if (!result || !result->isArray()) {
        return {};
    }

    QVector<Block> blocks;
    const QJsonArray array = result->toArray();
    blocks.reserve(array.size());
    for (const auto& item : array) {
        auto block = parseBlock(item);
        if (block) {
            blocks.append(*block);
        }
    }
    return blocks;
}

quint64 RpcIndexerService::getLastFinalizedBlockId()
{
    auto result = callRpc("getLastFinalizedBlockId");
    if (!result) {
        return 0;
    }

    const quint64 latestFinalized = jsonValueToU64(*result);
    if (latestFinalized > m_lastNotifiedBlockId) {
        m_lastNotifiedBlockId = latestFinalized;
    }

    return latestFinalized;
}

QVector<Transaction> RpcIndexerService::getTransactionsByAccount(const QString& accountId, int offset, int limit)
{
    auto result = callRpc("getTransactionsByAccount", QJsonArray{accountId, offset, limit});
    if (!result || !result->isArray()) {
        return {};
    }

    QVector<Transaction> transactions;
    const QJsonArray array = result->toArray();
    transactions.reserve(array.size());
    for (const auto& item : array) {
        auto tx = parseTransaction(item);
        if (tx) {
            transactions.append(*tx);
        }
    }
    return transactions;
}

SearchResults RpcIndexerService::search(const QString& query)
{
    SearchResults results;
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) {
        return results;
    }

    static const QRegularExpression hashRegex(QStringLiteral("^[0-9a-fA-F]{64}$"));
    if (hashRegex.match(trimmed).hasMatch()) {
        if (auto block = getBlockByHash(trimmed)) {
            results.blocks.append(*block);
        }
        if (auto tx = getTransaction(trimmed)) {
            results.transactions.append(*tx);
        }
    }

    if (auto account = getAccount(trimmed)) {
        results.accounts.append(*account);
    }

    bool ok = false;
    const quint64 blockId = trimmed.toULongLong(&ok);
    if (ok) {
        if (auto block = getBlockById(blockId)) {
            const auto duplicate = std::any_of(results.blocks.begin(), results.blocks.end(),
                [blockId](const Block& existing) {
                    return existing.blockId == blockId;
                });
            if (!duplicate) {
                results.blocks.append(*block);
            }
        }
    }

    return results;
}

void RpcIndexerService::onConnected()
{
    qInfo() << "Connected to indexer RPC:" << m_endpoint;
    ensureFinalizedBlocksSubscription();
}

void RpcIndexerService::onDisconnected()
{
    m_subscriptionRequestId = 0;
    m_subscriptionId = QJsonValue();
    m_subscriptionActive = false;

    for (auto it = m_waiters.begin(); it != m_waiters.end(); ++it) {
        if (it.value()) {
            it.value()->quit();
        }
    }
    m_waiters.clear();
}

void RpcIndexerService::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qWarning() << "Indexer RPC socket error:" << m_socket.errorString();
}

void RpcIndexerService::onTextMessageReceived(const QString& message)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid JSON-RPC payload:" << parseError.errorString();
        return;
    }

    const QJsonObject payload = doc.object();

    const QString method = payload.value("method").toString();
    if (method == "subscribeToFinalizedBlocks") {
        const QJsonObject params = payload.value("params").toObject();
        if (params.isEmpty()) {
            return;
        }

        if (!m_subscriptionId.isNull() && params.contains("subscription")) {
            const QJsonValue subscriptionValue = params.value("subscription");
            if (subscriptionValue != m_subscriptionId) {
                return;
            }
        }

        handleFinalizedBlockNotification(params.value("result"));
        return;
    }

    if (!payload.contains("id")) {
        return;
    }

    const quint64 id = jsonValueToU64(payload.value("id"));
    if (!id) {
        return;
    }

    if (id == m_subscriptionRequestId) {
        m_subscriptionRequestId = 0;
        if (payload.contains("error")) {
            m_subscriptionActive = false;
            m_subscriptionId = QJsonValue();
            qWarning() << "Failed to subscribe to finalized blocks:" << compactJson(payload.value("error"));
        } else {
            m_subscriptionId = payload.value("result");
            m_subscriptionActive = !m_subscriptionId.isUndefined() && !m_subscriptionId.isNull();
            if (!m_subscriptionActive) {
                qWarning() << "Unexpected finalized blocks subscription result";
            }
        }
    }

    m_responses.insert(id, payload);
    auto* waiter = m_waiters.value(id, nullptr);
    if (waiter) {
        waiter->quit();
    }
}

void RpcIndexerService::handleFinalizedBlockNotification(const QJsonValue& result)
{
    const quint64 blockId = extractFinalizedBlockId(result);
    if (!blockId || blockId <= m_lastNotifiedBlockId) {
        return;
    }

    QTimer::singleShot(0, this, [this, blockId]() {
        fetchAndEmitFinalizedBlock(blockId);
    });
}

void RpcIndexerService::fetchAndEmitFinalizedBlock(quint64 blockId)
{
    auto block = getBlockById(blockId);
    if (block) {
        if (blockId <= m_lastNotifiedBlockId) {
            return;
        }
        m_lastNotifiedBlockId = blockId;
        emit newBlockAdded(*block);
        return;
    }

    // Fallback to latest block snapshot if the specific id is briefly unavailable.
    auto latestBlocks = getBlocks(std::nullopt, 1);
    if (!latestBlocks.isEmpty()) {
        const Block& latest = latestBlocks.first();
        if (latest.blockId >= blockId && latest.blockId > m_lastNotifiedBlockId) {
            m_lastNotifiedBlockId = latest.blockId;
            emit newBlockAdded(latest);
            return;
        }
    }

    qWarning() << "Finalized block announced but not yet available:" << blockId;
}

bool RpcIndexerService::ensureConnected()
{
    if (!m_endpoint.isValid() || m_endpoint.scheme().isEmpty()) {
        qWarning() << "Invalid indexer RPC URL:" << m_endpoint;
        return false;
    }

    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        ensureFinalizedBlocksSubscription();
        return true;
    }

    if (m_socket.state() == QAbstractSocket::UnconnectedState) {
        m_socket.open(m_endpoint);
    }

    QEventLoop waitLoop;
    QTimer timeout;
    bool timedOut = false;

    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, &waitLoop, [&]() {
        timedOut = true;
        waitLoop.quit();
    });

    connect(&m_socket, &QWebSocket::connected, &waitLoop, &QEventLoop::quit);
    connect(&m_socket, &QWebSocket::disconnected, &waitLoop, &QEventLoop::quit);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(&m_socket, &QWebSocket::errorOccurred, &waitLoop, &QEventLoop::quit);
#else
    connect(&m_socket,
        QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
        &waitLoop,
        &QEventLoop::quit);
#endif

    timeout.start(4000);
    waitLoop.exec();

    if (timedOut) {
        qWarning() << "Timeout connecting to indexer RPC:" << m_endpoint;
        m_socket.abort();
        return false;
    }

    const bool isConnected = m_socket.state() == QAbstractSocket::ConnectedState;
    if (isConnected) {
        ensureFinalizedBlocksSubscription();
    }

    return isConnected;
}

void RpcIndexerService::ensureFinalizedBlocksSubscription()
{
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }
    if (m_subscriptionActive || m_subscriptionRequestId != 0) {
        return;
    }

    const quint64 requestId = m_nextRequestId++;
    m_subscriptionRequestId = requestId;

    QJsonObject request;
    request.insert("jsonrpc", "2.0");
    request.insert("id", static_cast<qint64>(requestId));
    request.insert("method", "subscribeToFinalizedBlocks");
    request.insert("params", QJsonArray());

    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

std::optional<QJsonValue> RpcIndexerService::callRpc(const QString& method, const QJsonArray& params)
{
    if (!ensureConnected()) {
        return std::nullopt;
    }

    const quint64 id = m_nextRequestId++;
    QJsonObject request;
    request.insert("jsonrpc", "2.0");
    request.insert("id", static_cast<qint64>(id));
    request.insert("method", method);
    request.insert("params", params);

    QEventLoop waitLoop;
    QTimer timeout;
    bool timedOut = false;

    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, &waitLoop, [&]() {
        timedOut = true;
        waitLoop.quit();
    });
    connect(&m_socket, &QWebSocket::disconnected, &waitLoop, &QEventLoop::quit);

    m_waiters.insert(id, &waitLoop);
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact)));

    timeout.start(5000);
    waitLoop.exec();
    m_waiters.remove(id);

    if (timedOut) {
        qWarning() << "RPC call timeout:" << method;
        return std::nullopt;
    }

    if (!m_responses.contains(id)) {
        qWarning() << "No RPC response for" << method;
        return std::nullopt;
    }

    const QJsonObject response = m_responses.take(id);
    if (response.contains("error")) {
        qWarning() << "RPC error for" << method << compactJson(response.value("error"));
        return std::nullopt;
    }
    if (!response.contains("result")) {
        qWarning() << "RPC result missing for" << method;
        return std::nullopt;
    }

    const std::optional<QJsonValue> result = response.value("result");
    qDebug() << "RPC call result for" << method << "(" << params << "):" << jsonValueToString(*result);
    return result;
}

std::optional<Block> RpcIndexerService::parseBlock(const QJsonValue& value) const
{
    if (!value.isObject()) {
        return std::nullopt;
    }

    const QJsonObject blockObj = value.toObject();
    const QJsonObject headerObj = blockObj.value("header").toObject();
    const QJsonObject bodyObj = blockObj.value("body").toObject();
    if (headerObj.isEmpty()) {
        return std::nullopt;
    }

    Block block;
    block.blockId = jsonValueToU64(headerObj.value("block_id"));
    block.hash = jsonValueToString(headerObj.value("hash"));
    block.prevBlockHash = jsonValueToString(headerObj.value("prev_block_hash"));
    block.signature = jsonValueToString(headerObj.value("signature"));
    block.bedrockParentId = jsonValueToString(blockObj.value("bedrock_parent_id"));

    const quint64 timestamp = jsonValueToU64(headerObj.value("timestamp"));
    if (timestamp > 1000000000000ULL) {
        block.timestamp = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp), QTimeZone::UTC);
    } else {
        block.timestamp = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp), QTimeZone::UTC);
    }
    if (!block.timestamp.isValid()) {
        block.timestamp = QDateTime::currentDateTimeUtc();
    }

    const QString status = blockObj.value("bedrock_status").toString("Pending");
    if (status == "Finalized") {
        block.bedrockStatus = BedrockStatus::Finalized;
    } else if (status == "Safe") {
        block.bedrockStatus = BedrockStatus::Safe;
    } else {
        block.bedrockStatus = BedrockStatus::Pending;
    }

    const QJsonArray txArray = bodyObj.value("transactions").toArray();
    block.transactions.reserve(txArray.size());
    for (const auto& txValue : txArray) {
        auto tx = parseTransaction(txValue);
        if (tx) {
            block.transactions.append(*tx);
        }
    }

    return block;
}

std::optional<Transaction> RpcIndexerService::parseTransaction(const QJsonValue& value) const
{
    if (!value.isObject()) {
        return std::nullopt;
    }

    const QJsonObject wrapper = value.toObject();
    Transaction tx;

    auto appendAccounts = [&tx](const QJsonArray& accountIds, const QJsonArray& nonces) {
        const int pairedCount = std::min(accountIds.size(), nonces.size());
        for (int i = 0; i < pairedCount; ++i) {
            AccountRef account;
            account.accountId = jsonValueToString(accountIds.at(i));
            account.nonce = jsonValueToString(nonces.at(i));
            tx.accounts.append(account);
        }
        for (int i = pairedCount; i < accountIds.size(); ++i) {
            AccountRef account;
            account.accountId = jsonValueToString(accountIds.at(i));
            account.nonce = "0";
            tx.accounts.append(account);
        }
    };

    auto fillWitnessFields = [&tx](const QJsonObject& witnessSet) {
        tx.signatureCount = witnessSet.value("signatures_and_public_keys").toArray().size();
        if (witnessSet.value("proof").isString()) {
            tx.proofSizeBytes = base64Size(witnessSet.value("proof").toString());
        }
    };

    if (wrapper.contains("Public")) {
        const QJsonObject publicTx = wrapper.value("Public").toObject();
        const QJsonObject message = publicTx.value("message").toObject();

        tx.type = TransactionType::Public;
        tx.hash = jsonValueToString(publicTx.value("hash"));
        tx.programId = jsonValueToString(message.value("program_id"));
        appendAccounts(message.value("account_ids").toArray(), message.value("nonces").toArray());

        const QJsonArray instructionData = message.value("instruction_data").toArray();
        tx.instructionData.reserve(instructionData.size());
        for (const auto& v : instructionData) {
            tx.instructionData.append(static_cast<quint32>(jsonValueToU64(v)));
        }

        fillWitnessFields(publicTx.value("witness_set").toObject());
        return tx;
    }

    if (wrapper.contains("PrivacyPreserving")) {
        const QJsonObject privateTx = wrapper.value("PrivacyPreserving").toObject();
        const QJsonObject message = privateTx.value("message").toObject();

        tx.type = TransactionType::PrivacyPreserving;
        tx.hash = jsonValueToString(privateTx.value("hash"));
        appendAccounts(message.value("public_account_ids").toArray(), message.value("nonces").toArray());

        tx.newCommitmentsCount = message.value("new_commitments").toArray().size();
        tx.nullifiersCount = message.value("new_nullifiers").toArray().size();
        tx.encryptedStatesCount = message.value("encrypted_private_post_states").toArray().size();

        const QJsonArray validityWindow = message.value("block_validity_window").toArray();
        if (!validityWindow.isEmpty()) {
            tx.validityWindowStart = jsonValueToU64(validityWindow.at(0));
        }
        if (validityWindow.size() > 1) {
            tx.validityWindowEnd = jsonValueToU64(validityWindow.at(1));
        }

        fillWitnessFields(privateTx.value("witness_set").toObject());
        return tx;
    }

    if (wrapper.contains("ProgramDeployment")) {
        const QJsonObject deployTx = wrapper.value("ProgramDeployment").toObject();
        const QJsonObject message = deployTx.value("message").toObject();

        tx.type = TransactionType::ProgramDeployment;
        tx.hash = jsonValueToString(deployTx.value("hash"));
        tx.bytecodeSizeBytes = base64Size(message.value("bytecode").toString());
        tx.signatureCount = 0;
        tx.proofSizeBytes = 0;
        return tx;
    }

    return std::nullopt;
}

std::optional<Account> RpcIndexerService::parseAccount(const QString& accountId, const QJsonValue& value) const
{
    if (!value.isObject()) {
        return std::nullopt;
    }

    const QJsonObject accountObj = value.toObject();

    Account account;
    account.accountId = accountId;
    account.programOwner = jsonValueToString(accountObj.value("program_owner"));
    account.balance = jsonValueToString(accountObj.value("balance"));
    account.nonce = jsonValueToString(accountObj.value("nonce"));
    account.dataSizeBytes = base64Size(accountObj.value("data").toString());

    if (account.balance.isEmpty()) {
        account.balance = "0";
    }
    if (account.nonce.isEmpty()) {
        account.nonce = "0";
    }

    return account;
}

QString RpcIndexerService::jsonValueToString(const QJsonValue& value)
{
    if (value.isString()) {
        return value.toString();
    }

    if (value.isDouble()) {
        const double number = value.toDouble();
        const qulonglong integer = static_cast<qulonglong>(number);
        if (number == static_cast<double>(integer)) {
            return QString::number(integer);
        }
        return QString::number(number, 'g', 16);
    }

    if (value.isBool()) {
        return value.toBool() ? "true" : "false";
    }

    return compactJson(value);
}

quint64 RpcIndexerService::jsonValueToU64(const QJsonValue& value)
{
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (number <= 0.0) {
            return 0;
        }
        return static_cast<quint64>(number);
    }

    if (value.isString()) {
        bool ok = false;
        const qulonglong parsed = value.toString().toULongLong(&ok);
        return ok ? parsed : 0;
    }

    return 0;
}

int RpcIndexerService::base64Size(const QString& value)
{
    if (value.isEmpty()) {
        return 0;
    }
    return QByteArray::fromBase64(value.toUtf8()).size();
}
