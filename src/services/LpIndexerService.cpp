#include "LpIndexerService.h"

#include "logos_api.h"
#include "logos_api_client.h"

#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QTimer>
#include <QTimeZone>
#include <QVariant>

#include <algorithm>

namespace {

constexpr auto kModuleName = "lez_indexer_module";
constexpr int kPollIntervalMs = 2000;

// 64/128-bit values arrive as decimal strings (to dodge JSON double precision
// loss), but be lenient and also accept a JSON number.
quint64 jsonU64(const QJsonValue& value)
{
    if (value.isString()) {
        bool ok = false;
        const qulonglong parsed = value.toString().toULongLong(&ok);
        return ok ? parsed : 0;
    }
    if (value.isDouble()) {
        const double number = value.toDouble();
        return number > 0.0 ? static_cast<quint64>(number) : 0;
    }
    return 0;
}

QString jsonStr(const QJsonValue& value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(static_cast<qulonglong>(value.toDouble()));
    }
    return {};
}

QDateTime timestampFromSeconds(quint64 timestamp)
{
    QDateTime dt;
    // Tolerate either seconds or milliseconds since epoch.
    if (timestamp > 1000000000000ULL) {
        dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp), QTimeZone::UTC);
    } else {
        dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp), QTimeZone::UTC);
    }
    return dt.isValid() ? dt : QDateTime::currentDateTimeUtc();
}

} // namespace

LpIndexerService::LpIndexerService(LogosAPI* logosAPI, const QString& configPath, const QString& port, QObject* parent)
    : IndexerService(parent)
    , m_logosAPI(logosAPI)
    , m_configPath(configPath.trimmed())
    , m_port(port.trimmed())
{
    if (m_logosAPI) {
        m_client = m_logosAPI->getClient(kModuleName);
    }
    if (!m_client) {
        qWarning() << "LpIndexerService: no LogosAPI client for" << kModuleName;
    }

    // Best-effort boot (no-op provider-side if already running, or if no config).
    startIndexer();

    // The provider has no push event — poll the tip for live head updates.
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &LpIndexerService::pollForNewBlocks);
    m_pollTimer->start();
}

LpIndexerService::~LpIndexerService() = default;

QString LpIndexerService::endpoint() const
{
    return m_configPath;
}

void LpIndexerService::setEndpoint(const QString& endpoint)
{
    const QString trimmed = endpoint.trimmed();
    if (trimmed == m_configPath) {
        return;
    }
    m_configPath = trimmed;
    // Re-baseline the live poller; the (re)started indexer may ingest afresh.
    m_lastNotifiedBlockId = 0;
    m_polledOnce = false;
    startIndexer();
}

void LpIndexerService::startIndexer()
{
    if (!m_client || m_configPath.isEmpty()) {
        return; // assume the indexer was started elsewhere
    }
    const QVariant result = m_client->invokeRemoteMethod(kModuleName, "start_indexer", m_configPath, m_port);
    const int code = result.isValid() ? result.toInt() : -1;
    if (code != 0) {
        qWarning() << "LpIndexerService: start_indexer returned" << code << "for" << m_configPath;
    }
}

std::optional<Account> LpIndexerService::getAccount(const QString& accountId)
{
    if (!m_client) {
        return std::nullopt;
    }
    const QVariant r = m_client->invokeRemoteMethod(kModuleName, "getAccount", accountId);
    return parseAccount(accountId, r.isValid() ? r.toString() : QString());
}

std::optional<Block> LpIndexerService::getBlockById(quint64 blockId)
{
    if (!m_client) {
        return std::nullopt;
    }
    const QVariant r = m_client->invokeRemoteMethod(kModuleName, "getBlockById", QString::number(blockId));
    return parseBlock(r.isValid() ? r.toString() : QString());
}

std::optional<Block> LpIndexerService::getBlockByHash(const QString& hash)
{
    if (!m_client) {
        return std::nullopt;
    }
    const QVariant r = m_client->invokeRemoteMethod(kModuleName, "getBlockByHash", hash);
    return parseBlock(r.isValid() ? r.toString() : QString());
}

std::optional<Transaction> LpIndexerService::getTransaction(const QString& hash)
{
    if (!m_client) {
        return std::nullopt;
    }
    const QVariant r = m_client->invokeRemoteMethod(kModuleName, "getTransaction", hash);
    const QString json = r.isValid() ? r.toString() : QString();
    if (json.isEmpty()) {
        return std::nullopt;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return std::nullopt;
    }
    return parseTransaction(doc.object());
}

QVector<Block> LpIndexerService::getBlocks(std::optional<quint64> before, int limit)
{
    if (!m_client) {
        return {};
    }
    const QString beforeArg = before.has_value() ? QString::number(*before) : QString();
    const QVariant r = m_client->invokeRemoteMethod(kModuleName, "getBlocks", beforeArg, QString::number(limit));
    const QString json = r.isValid() ? r.toString() : QString();
    if (json.isEmpty()) {
        return {};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        return {};
    }

    QVector<Block> blocks;
    const QJsonArray array = doc.array();
    blocks.reserve(array.size());
    for (const auto& item : array) {
        if (auto block = parseBlockObject(item.toObject())) {
            blocks.append(*block);
        }
    }
    return blocks;
}

quint64 LpIndexerService::getLastFinalizedBlockId()
{
    if (!m_client) {
        return 0;
    }
    const QVariant r = m_client->invokeRemoteMethod(kModuleName, "getLastFinalizedBlockId");
    if (!r.isValid()) {
        return 0;
    }
    bool ok = false;
    const qulonglong parsed = r.toString().toULongLong(&ok);
    return ok ? parsed : 0;
}

QVector<Transaction> LpIndexerService::getTransactionsByAccount(const QString& accountId, int offset, int limit)
{
    if (!m_client) {
        return {};
    }
    const QVariant r = m_client->invokeRemoteMethod(
        kModuleName, "getTransactionsByAccount", accountId, QString::number(offset), QString::number(limit));
    const QString json = r.isValid() ? r.toString() : QString();
    if (json.isEmpty()) {
        return {};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        return {};
    }

    QVector<Transaction> transactions;
    const QJsonArray array = doc.array();
    transactions.reserve(array.size());
    for (const auto& item : array) {
        if (auto tx = parseTransaction(item.toObject())) {
            transactions.append(*tx);
        }
    }
    return transactions;
}

SearchResults LpIndexerService::search(const QString& query)
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
        if (auto account = getAccount(trimmed)) {
            results.accounts.append(*account);
        }
    }

    bool ok = false;
    const quint64 blockId = trimmed.toULongLong(&ok);
    if (ok) {
        if (auto block = getBlockById(blockId)) {
            const bool duplicate = std::any_of(results.blocks.begin(), results.blocks.end(),
                [blockId](const Block& existing) { return existing.blockId == blockId; });
            if (!duplicate) {
                results.blocks.append(*block);
            }
        }
    }

    return results;
}

void LpIndexerService::pollForNewBlocks()
{
    const quint64 latest = getLastFinalizedBlockId();
    if (latest == 0) {
        return;
    }

    // First successful poll establishes a baseline without re-emitting blocks
    // the initial MainPage::refresh already loaded.
    if (!m_polledOnce) {
        m_polledOnce = true;
        m_lastNotifiedBlockId = latest;
        return;
    }

    if (latest <= m_lastNotifiedBlockId) {
        return;
    }

    // For a large jump (or a gap), emit only the new head: MainPage::onNewBlock
    // detects the discontinuity and rebuilds the recent list from the backend.
    if (latest - m_lastNotifiedBlockId > 8) {
        if (auto block = getBlockById(latest)) {
            m_lastNotifiedBlockId = latest;
            emit newBlockAdded(*block);
        }
        return;
    }

    for (quint64 id = m_lastNotifiedBlockId + 1; id <= latest; ++id) {
        if (auto block = getBlockById(id)) {
            emit newBlockAdded(*block);
        }
    }
    m_lastNotifiedBlockId = latest;
}

std::optional<Block> LpIndexerService::parseBlock(const QString& json) const
{
    if (json.isEmpty()) {
        return std::nullopt;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return std::nullopt;
    }
    return parseBlockObject(doc.object());
}

std::optional<Block> LpIndexerService::parseBlockObject(const QJsonObject& obj) const
{
    if (obj.isEmpty()) {
        return std::nullopt;
    }

    Block block;
    block.blockId = jsonU64(obj.value("block_id"));
    block.hash = jsonStr(obj.value("hash"));
    block.prevBlockHash = jsonStr(obj.value("prev_block_hash"));
    block.signature = jsonStr(obj.value("signature"));
    block.timestamp = timestampFromSeconds(jsonU64(obj.value("timestamp")));

    const QString status = obj.value("bedrock_status").toString("Pending");
    if (status == "Finalized") {
        block.bedrockStatus = BedrockStatus::Finalized;
    } else if (status == "Safe") {
        block.bedrockStatus = BedrockStatus::Safe;
    } else {
        block.bedrockStatus = BedrockStatus::Pending;
    }

    const QJsonArray txArray = obj.value("transactions").toArray();
    block.transactions.reserve(txArray.size());
    for (const auto& txValue : txArray) {
        if (auto tx = parseTransaction(txValue.toObject())) {
            block.transactions.append(*tx);
        }
    }

    return block;
}

std::optional<Transaction> LpIndexerService::parseTransaction(const QJsonObject& obj) const
{
    if (obj.isEmpty()) {
        return std::nullopt;
    }

    Transaction tx;
    tx.hash = jsonStr(obj.value("hash"));

    const QString type = obj.value("type").toString();

    auto appendAccounts = [&tx](const QJsonArray& accounts) {
        for (const auto& entry : accounts) {
            const QJsonObject account = entry.toObject();
            AccountRef ref;
            ref.accountId = jsonStr(account.value("account_id"));
            ref.nonce = jsonStr(account.value("nonce"));
            if (ref.nonce.isEmpty()) {
                ref.nonce = "0";
            }
            tx.accounts.append(ref);
        }
    };

    if (type == "Public") {
        tx.type = TransactionType::Public;
        tx.programId = jsonStr(obj.value("program_id"));
        appendAccounts(obj.value("accounts").toArray());

        const QJsonArray instructionData = obj.value("instruction_data").toArray();
        tx.instructionData.reserve(instructionData.size());
        for (const auto& v : instructionData) {
            tx.instructionData.append(static_cast<quint32>(jsonU64(v)));
        }
        tx.signatureCount = obj.value("signature_count").toInt();
        return tx;
    }

    if (type == "PrivacyPreserving") {
        tx.type = TransactionType::PrivacyPreserving;
        appendAccounts(obj.value("accounts").toArray());
        tx.newCommitmentsCount = obj.value("new_commitments_count").toInt();
        tx.nullifiersCount = obj.value("nullifiers_count").toInt();
        tx.encryptedStatesCount = obj.value("encrypted_states_count").toInt();
        tx.validityWindowStart = jsonU64(obj.value("validity_window_start"));
        tx.validityWindowEnd = jsonU64(obj.value("validity_window_end"));
        tx.signatureCount = obj.value("signature_count").toInt();
        tx.proofSizeBytes = obj.value("proof_size").toInt();
        return tx;
    }

    if (type == "ProgramDeployment") {
        tx.type = TransactionType::ProgramDeployment;
        tx.bytecodeSizeBytes = obj.value("bytecode_size").toInt();
        return tx;
    }

    return std::nullopt;
}

std::optional<Account> LpIndexerService::parseAccount(const QString& accountId, const QString& json) const
{
    if (json.isEmpty()) {
        return std::nullopt;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return std::nullopt;
    }
    const QJsonObject obj = doc.object();

    Account account;
    account.accountId = accountId; // provider payload omits it; inject the queried id
    account.programOwner = jsonStr(obj.value("program_owner"));
    account.balance = jsonStr(obj.value("balance"));
    account.nonce = jsonStr(obj.value("nonce"));
    account.dataSizeBytes = obj.value("data_size").toInt();

    if (account.balance.isEmpty()) {
        account.balance = "0";
    }
    if (account.nonce.isEmpty()) {
        account.nonce = "0";
    }

    return account;
}
