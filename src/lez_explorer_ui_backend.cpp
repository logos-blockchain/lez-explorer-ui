#include "lez_explorer_ui_backend.h"

#include <algorithm>
#include <utility>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QTimeZone>
#include <QTimer>

// Generated umbrella: LogosModules (behind modules()) built from
// metadata.json#dependencies — the Qt-typed wrappers for lez_indexer_module.
#include "logos_sdk.h"

namespace {

constexpr int kPollIntervalMs = 2000;
// Above this gap we rebuild the feed from the backend rather than fetch every
// missing block one by one (matches the former LpIndexerService threshold).
constexpr quint64 kGapRebuildThreshold = 8;

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

QString timestampText(quint64 timestamp)
{
    QDateTime dt;
    // Tolerate either seconds or milliseconds since epoch.
    if (timestamp > 1000000000000ULL) {
        dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp), QTimeZone::UTC);
    } else {
        dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp), QTimeZone::UTC);
    }
    if (!dt.isValid()) {
        dt = QDateTime::currentDateTimeUtc();
    }
    return dt.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) + QStringLiteral(" UTC");
}

QVariantMap txObjectToMap(const QJsonObject& obj)
{
    QVariantMap tx;
    if (obj.isEmpty()) {
        return tx;
    }

    tx.insert(QStringLiteral("hash"), jsonStr(obj.value(QStringLiteral("hash"))));
    const QString type = obj.value(QStringLiteral("type")).toString();
    tx.insert(QStringLiteral("type"), type);

    const auto accountsOf = [](const QJsonArray& arr) {
        QVariantList accounts;
        for (const auto& entry : arr) {
            const QJsonObject account = entry.toObject();
            QVariantMap ref;
            ref.insert(QStringLiteral("accountId"), jsonStr(account.value(QStringLiteral("account_id"))));
            QString nonce = jsonStr(account.value(QStringLiteral("nonce")));
            ref.insert(QStringLiteral("nonce"), nonce.isEmpty() ? QStringLiteral("0") : nonce);
            accounts.append(ref);
        }
        return accounts;
    };

    if (type == QLatin1String("Public")) {
        tx.insert(QStringLiteral("programId"), jsonStr(obj.value(QStringLiteral("program_id"))));
        tx.insert(QStringLiteral("accounts"), accountsOf(obj.value(QStringLiteral("accounts")).toArray()));
        tx.insert(QStringLiteral("instructionDataCount"),
            obj.value(QStringLiteral("instruction_data")).toArray().size());
        tx.insert(QStringLiteral("signatureCount"), obj.value(QStringLiteral("signature_count")).toInt());
    } else if (type == QLatin1String("PrivacyPreserving")) {
        tx.insert(QStringLiteral("accounts"), accountsOf(obj.value(QStringLiteral("accounts")).toArray()));
        tx.insert(QStringLiteral("newCommitmentsCount"), obj.value(QStringLiteral("new_commitments_count")).toInt());
        tx.insert(QStringLiteral("nullifiersCount"), obj.value(QStringLiteral("nullifiers_count")).toInt());
        tx.insert(QStringLiteral("encryptedStatesCount"), obj.value(QStringLiteral("encrypted_states_count")).toInt());
        tx.insert(QStringLiteral("validityWindowStart"),
            static_cast<qlonglong>(jsonU64(obj.value(QStringLiteral("validity_window_start")))));
        tx.insert(QStringLiteral("validityWindowEnd"),
            static_cast<qlonglong>(jsonU64(obj.value(QStringLiteral("validity_window_end")))));
        tx.insert(QStringLiteral("signatureCount"), obj.value(QStringLiteral("signature_count")).toInt());
        tx.insert(QStringLiteral("proofSizeBytes"), obj.value(QStringLiteral("proof_size")).toInt());
    } else if (type == QLatin1String("ProgramDeployment")) {
        tx.insert(QStringLiteral("bytecodeSizeBytes"), obj.value(QStringLiteral("bytecode_size")).toInt());
    }

    return tx;
}

QVariantMap blockObjectToMap(const QJsonObject& obj)
{
    QVariantMap block;
    if (obj.isEmpty()) {
        return block;
    }

    block.insert(QStringLiteral("blockId"), static_cast<qlonglong>(jsonU64(obj.value(QStringLiteral("block_id")))));
    block.insert(QStringLiteral("hash"), jsonStr(obj.value(QStringLiteral("hash"))));
    block.insert(QStringLiteral("prevBlockHash"), jsonStr(obj.value(QStringLiteral("prev_block_hash"))));
    block.insert(QStringLiteral("signature"), jsonStr(obj.value(QStringLiteral("signature"))));
    block.insert(QStringLiteral("timestampText"), timestampText(jsonU64(obj.value(QStringLiteral("timestamp")))));

    const QString status = obj.value(QStringLiteral("bedrock_status")).toString(QStringLiteral("Pending"));
    block.insert(QStringLiteral("status"), status);

    const QJsonArray txArray = obj.value(QStringLiteral("transactions")).toArray();
    QVariantList transactions;
    transactions.reserve(txArray.size());
    for (const auto& txValue : txArray) {
        const QVariantMap tx = txObjectToMap(txValue.toObject());
        if (!tx.isEmpty()) {
            transactions.append(tx);
        }
    }
    block.insert(QStringLiteral("transactions"), transactions);
    block.insert(QStringLiteral("txCount"), transactions.size());

    return block;
}

QVariantMap blockJsonToMap(const QString& json)
{
    if (json.isEmpty()) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return doc.isObject() ? blockObjectToMap(doc.object()) : QVariantMap();
}

QVariantMap accountJsonToMap(const QString& accountId, const QString& json)
{
    if (json.isEmpty()) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return {};
    }
    const QJsonObject obj = doc.object();

    QVariantMap account;
    account.insert(QStringLiteral("accountId"), accountId); // payload omits it; inject the queried id
    account.insert(QStringLiteral("programOwner"), jsonStr(obj.value(QStringLiteral("program_owner"))));
    QString balance = jsonStr(obj.value(QStringLiteral("balance")));
    account.insert(QStringLiteral("balance"), balance.isEmpty() ? QStringLiteral("0") : balance);
    QString nonce = jsonStr(obj.value(QStringLiteral("nonce")));
    account.insert(QStringLiteral("nonce"), nonce.isEmpty() ? QStringLiteral("0") : nonce);
    account.insert(QStringLiteral("dataSizeBytes"), obj.value(QStringLiteral("data_size")).toInt());
    return account;
}

QVariantMap txJsonToMap(const QString& json)
{
    if (json.isEmpty()) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return doc.isObject() ? txObjectToMap(doc.object()) : QVariantMap();
}

} // namespace

LezExplorerUiBackend::LezExplorerUiBackend() = default;

LezExplorerUiBackend::~LezExplorerUiBackend() = default;

void LezExplorerUiBackend::onContextReady()
{
    setConnectionStatus(QStringLiteral("Connecting"));

    // The indexer pushes no events — poll the tip for live head updates.
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &LezExplorerUiBackend::pollTip);
    m_pollTimer->start();

    // Kick an immediate poll so the view fills without waiting a full interval.
    pollTip();
}

bool LezExplorerUiBackend::applyIndexerConfig(QString configPath, QString port)
{
    const QString trimmedConfig = configPath.trimmed();
    const QString trimmedPort = port.trimmed();
    if (!trimmedPort.isEmpty()) {
        m_port = trimmedPort;
    }
    setIndexerConfig(trimmedConfig);

    bool started = true;
    if (!trimmedConfig.isEmpty()) {
        const qlonglong code = modules().lez_indexer_module.start_indexer(trimmedConfig, m_port);
        started = (code == 0);
        if (!started) {
            emit error(QStringLiteral("start_indexer failed (code %1) for %2").arg(code).arg(trimmedConfig));
        }
    }

    // Re-baseline the poller; a (re)started indexer may ingest afresh.
    m_lastPolledTip = 0;
    m_polledOnce = false;
    refreshBlocks();
    return started;
}

void LezExplorerUiBackend::refreshBlocks()
{
    const QString json = modules().lez_indexer_module.getBlocks(QString(), QStringLiteral("10"));
    m_recentBlocks = parseBlockArrayJson(json);

    m_newestLoadedId = 0;
    m_oldestLoadedId = 0;
    for (const QVariant& entry : std::as_const(m_recentBlocks)) {
        const quint64 id = entry.toMap().value(QStringLiteral("blockId")).toULongLong();
        if (m_newestLoadedId == 0 || id > m_newestLoadedId) {
            m_newestLoadedId = id;
        }
        if (m_oldestLoadedId == 0 || id < m_oldestLoadedId) {
            m_oldestLoadedId = id;
        }
    }
    setRecentBlocks(m_recentBlocks);
}

void LezExplorerUiBackend::loadMoreBlocks()
{
    if (m_oldestLoadedId <= 1) {
        return; // already at genesis
    }
    const QString json
        = modules().lez_indexer_module.getBlocks(QString::number(m_oldestLoadedId), QStringLiteral("10"));
    const QVariantList older = parseBlockArrayJson(json);
    if (older.isEmpty()) {
        return;
    }

    m_recentBlocks.append(older);
    for (const QVariant& entry : older) {
        const quint64 id = entry.toMap().value(QStringLiteral("blockId")).toULongLong();
        if (id != 0 && (m_oldestLoadedId == 0 || id < m_oldestLoadedId)) {
            m_oldestLoadedId = id;
        }
    }
    setRecentBlocks(m_recentBlocks);
}

QVariantMap LezExplorerUiBackend::getBlockById(QString id)
{
    return blockJsonToMap(modules().lez_indexer_module.getBlockById(id));
}

QVariantMap LezExplorerUiBackend::getBlockByHash(QString hash)
{
    return blockJsonToMap(modules().lez_indexer_module.getBlockByHash(hash));
}

QVariantMap LezExplorerUiBackend::getTransaction(QString hash)
{
    return txJsonToMap(modules().lez_indexer_module.getTransaction(hash));
}

QVariantMap LezExplorerUiBackend::getAccount(QString accountId)
{
    return accountJsonToMap(accountId, modules().lez_indexer_module.getAccount(accountId));
}

QVariantList LezExplorerUiBackend::getTransactionsByAccount(QString accountId, int offset, int limit)
{
    const QString json = modules().lez_indexer_module.getTransactionsByAccount(
        accountId, QString::number(offset), QString::number(limit));
    if (json.isEmpty()) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        return {};
    }

    QVariantList result;
    const QJsonArray array = doc.array();
    result.reserve(array.size());
    for (const auto& item : array) {
        const QVariantMap tx = txObjectToMap(item.toObject());
        if (!tx.isEmpty()) {
            result.append(tx);
        }
    }
    return result;
}

QVariantMap LezExplorerUiBackend::search(QString query)
{
    QVariantList blocks;
    QVariantList transactions;
    QVariantList accounts;

    const QString trimmed = query.trimmed();
    if (!trimmed.isEmpty()) {
        static const QRegularExpression hashRegex(QStringLiteral("^[0-9a-fA-F]{64}$"));
        if (hashRegex.match(trimmed).hasMatch()) {
            const QVariantMap block = getBlockByHash(trimmed);
            if (!block.isEmpty()) {
                blocks.append(block);
            }
            const QVariantMap tx = getTransaction(trimmed);
            if (!tx.isEmpty()) {
                transactions.append(tx);
            }
            const QVariantMap account = getAccount(trimmed);
            // getAccount always injects the id; treat "no payload fields" as miss.
            if (account.size() > 1) {
                accounts.append(account);
            }
        }

        bool ok = false;
        const quint64 blockId = trimmed.toULongLong(&ok);
        if (ok) {
            const QVariantMap block = getBlockById(QString::number(blockId));
            if (!block.isEmpty()) {
                const bool duplicate = std::any_of(blocks.cbegin(), blocks.cend(), [blockId](const QVariant& e) {
                    return e.toMap().value(QStringLiteral("blockId")).toULongLong() == blockId;
                });
                if (!duplicate) {
                    blocks.append(block);
                }
            }
        }
    }

    QVariantMap results;
    results.insert(QStringLiteral("blocks"), blocks);
    results.insert(QStringLiteral("transactions"), transactions);
    results.insert(QStringLiteral("accounts"), accounts);
    return results;
}

void LezExplorerUiBackend::pollTip()
{
    const QString tipStr = modules().lez_indexer_module.getLastFinalizedBlockId();
    bool ok = false;
    const quint64 tip = tipStr.toULongLong(&ok);
    if (!ok || tip == 0) {
        setConnectionStatus(QStringLiteral("Connecting"));
        return;
    }

    setConnectionStatus(QStringLiteral("Connected"));
    setChainHeight(static_cast<qlonglong>(tip));

    // First successful poll: fill the feed and baseline without double-loading.
    if (!m_polledOnce) {
        m_polledOnce = true;
        m_lastPolledTip = tip;
        refreshBlocks();
        return;
    }

    if (tip <= m_lastPolledTip) {
        return;
    }

    // Large jump / gap: rebuild the recent list from the backend.
    if (tip - m_lastPolledTip > kGapRebuildThreshold) {
        m_lastPolledTip = tip;
        refreshBlocks();
        return;
    }

    // Contiguous growth: fetch the new heads and prepend (newest first).
    for (quint64 id = m_lastPolledTip + 1; id <= tip; ++id) {
        const QVariantMap block = fetchBlock(id);
        if (!block.isEmpty()) {
            m_recentBlocks.prepend(block);
            if (id > m_newestLoadedId) {
                m_newestLoadedId = id;
            }
            if (m_oldestLoadedId == 0) {
                m_oldestLoadedId = id;
            }
        }
    }
    m_lastPolledTip = tip;
    setRecentBlocks(m_recentBlocks);
}

QVariantMap LezExplorerUiBackend::fetchBlock(quint64 blockId)
{
    return blockJsonToMap(modules().lez_indexer_module.getBlockById(QString::number(blockId)));
}

QVariantList LezExplorerUiBackend::parseBlockArrayJson(const QString& json) const
{
    if (json.isEmpty()) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) {
        return {};
    }

    QVariantList blocks;
    const QJsonArray array = doc.array();
    blocks.reserve(array.size());
    for (const auto& item : array) {
        const QVariantMap block = blockObjectToMap(item.toObject());
        if (!block.isEmpty()) {
            blocks.append(block);
        }
    }
    return blocks;
}
