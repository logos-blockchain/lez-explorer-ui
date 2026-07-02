#include "lez_explorer_ui_backend.h"

#include <algorithm>
#include <utility>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>
#include <QStandardPaths>
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

    // Map the indexer FFI's OperationStatus to a human message.
    QString startErrorMessage(qlonglong code) {
        switch (code) {
        case 2: // OperationStatus::InitializationError
            return QStringLiteral(
                "Indexer failed to initialize (InitializationError). This is usually a "
                "storage problem — the data directory is locked by another running "
                "indexer, or isn't writable — or an invalid config field. It is not "
                "necessarily the JSON you just edited."
            );
        case 3: // OperationStatus::ClientError
            return QStringLiteral(
                "Indexer client error (ClientError) while starting — check the bedrock "
                "node address in the config and that the node is reachable."
            );
        case 1: // OperationStatus::NullPointer
            return QStringLiteral("Indexer failed to start (NullPointer).");
        default:
            return QStringLiteral("Indexer failed to start (OperationStatus %1).").arg(code);
        }
    }

    // Starter config shown in the Settings editor (and its "Reset to default").
    // A working local-dev indexer config; the user edits the fields (bedrock addr,
    // channel id, initial accounts/commitments, signing key, ...) and saves.
    const char* const kDefaultConfig = R"JSON({
    "consensus_info_polling_interval": "1s",
    "bedrock_config": {
        "addr": "http://localhost:8080",
        "backoff": {
            "start_delay": "100ms",
            "max_retries": 5
        }
    },
    "channel_id": "0101010101010101010101010101010101010101010101010101010101010101"
})JSON";

    // 64/128-bit values arrive as decimal strings (to dodge JSON double precision
    // loss), but be lenient and also accept a JSON number.
    quint64 jsonU64(const QJsonValue& value) {
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

    QString jsonStr(const QJsonValue& value) {
        if (value.isString()) {
            return value.toString();
        }
        if (value.isDouble()) {
            return QString::number(static_cast<qulonglong>(value.toDouble()));
        }
        return {};
    }

    // Account ids are Base58 (matching the wallet UI and canonical LEZ); block/tx
    // hashes are hex. These mirror the indexer module's codec so the explorer can
    // recognise an account-shaped query and always display the canonical Base58
    // form.
    const char* const kBase58Alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    int base58Index(char c) {
        for (int i = 0; i < 58; ++i) {
            if (kBase58Alphabet[i] == c) {
                return i;
            }
        }
        return -1;
    }

    // Decode a Base58 string (plain Bitcoin alphabet, no checksum) into raw bytes.
    // Returns false on any non-Base58 character. Leading '1's map to leading zeros.
    bool base58Decode(const QString& s, QByteArray& out) {
        const QString t = s.trimmed();
        if (t.isEmpty()) {
            return false;
        }
        int leadingZeros = 0;
        for (int i = 0; i < t.size() && t.at(i) == QLatin1Char('1'); ++i) {
            ++leadingZeros;
        }
        QByteArray digits; // base-256, least-significant first
        for (const QChar qc : t) {
            const int idx = qc.unicode() < 128 ? base58Index(static_cast<char>(qc.unicode())) : -1;
            if (idx < 0) {
                return false;
            }
            int carry = idx;
            for (int j = 0; j < digits.size(); ++j) {
                carry += static_cast<unsigned char>(digits[j]) * 58;
                digits[j] = static_cast<char>(carry & 0xFF);
                carry >>= 8;
            }
            while (carry > 0) {
                digits.append(static_cast<char>(carry & 0xFF));
                carry >>= 8;
            }
        }
        out = QByteArray(leadingZeros, '\0');
        for (int i = digits.size(); i-- > 0;) {
            out.append(digits[i]);
        }
        return true;
    }

    // Base58-encode raw bytes (plain Bitcoin alphabet). Inverse of base58Decode.
    QString bytes32ToBase58(const QByteArray& data) {
        int leadingZeros = 0;
        while (leadingZeros < data.size() && data[leadingZeros] == '\0') {
            ++leadingZeros;
        }
        QByteArray digits; // base-58, least-significant first
        for (int i = 0; i < data.size(); ++i) {
            int carry = static_cast<unsigned char>(data[i]);
            for (int j = 0; j < digits.size(); ++j) {
                carry += static_cast<unsigned char>(digits[j]) * 256;
                digits[j] = static_cast<char>(carry % 58);
                carry /= 58;
            }
            while (carry > 0) {
                digits.append(static_cast<char>(carry % 58));
                carry /= 58;
            }
        }
        QString out(leadingZeros, QLatin1Char('1'));
        for (int i = digits.size(); i-- > 0;) {
            out.append(QLatin1Char(kBase58Alphabet[static_cast<unsigned char>(digits[i])]));
        }
        return out;
    }

    // Canonical Base58 form of an account-id query, or "" if not account-shaped.
    // Accepts Base58 (must decode to 32 bytes) or 64-char hex (optionally 0x): a
    // hex search is normalised to Base58 so the result is displayed in canonical
    // form.
    QString canonicalAccountId(const QString& query) {
        const QString trimmed = query.trimmed();
        QByteArray decoded;
        if (base58Decode(trimmed, decoded) && decoded.size() == 32) {
            return trimmed;
        }
        QString hex = trimmed;
        if (hex.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
            hex = hex.mid(2);
        }
        static const QRegularExpression hexRe(QStringLiteral("^[0-9a-fA-F]{64}$"));
        if (hexRe.match(hex).hasMatch()) {
            return bytes32ToBase58(QByteArray::fromHex(hex.toLatin1()));
        }
        return {};
    }

    QString timestampText(quint64 timestamp) {
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

    QVariantMap txObjectToMap(const QJsonObject& obj) {
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
            tx.insert(
                QStringLiteral("instructionDataCount"), obj.value(QStringLiteral("instruction_data")).toArray().size()
            );
            tx.insert(QStringLiteral("signatureCount"), obj.value(QStringLiteral("signature_count")).toInt());
        } else if (type == QLatin1String("PrivacyPreserving")) {
            tx.insert(QStringLiteral("accounts"), accountsOf(obj.value(QStringLiteral("accounts")).toArray()));
            tx.insert(
                QStringLiteral("newCommitmentsCount"), obj.value(QStringLiteral("new_commitments_count")).toInt()
            );
            tx.insert(QStringLiteral("nullifiersCount"), obj.value(QStringLiteral("nullifiers_count")).toInt());
            tx.insert(
                QStringLiteral("encryptedStatesCount"), obj.value(QStringLiteral("encrypted_states_count")).toInt()
            );
            tx.insert(
                QStringLiteral("validityWindowStart"),
                static_cast<qlonglong>(jsonU64(obj.value(QStringLiteral("validity_window_start"))))
            );
            tx.insert(
                QStringLiteral("validityWindowEnd"),
                static_cast<qlonglong>(jsonU64(obj.value(QStringLiteral("validity_window_end"))))
            );
            tx.insert(QStringLiteral("signatureCount"), obj.value(QStringLiteral("signature_count")).toInt());
            tx.insert(QStringLiteral("proofSizeBytes"), obj.value(QStringLiteral("proof_size")).toInt());
        } else if (type == QLatin1String("ProgramDeployment")) {
            tx.insert(QStringLiteral("bytecodeSizeBytes"), obj.value(QStringLiteral("bytecode_size")).toInt());
        }

        return tx;
    }

    QVariantMap blockObjectToMap(const QJsonObject& obj) {
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

    QVariantMap blockJsonToMap(const QString& json) {
        if (json.isEmpty()) {
            return {};
        }
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        return doc.isObject() ? blockObjectToMap(doc.object()) : QVariantMap();
    }

    QVariantMap accountJsonToMap(const QString& accountId, const QString& json) {
        if (json.isEmpty()) {
            return {};
        }
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (!doc.isObject()) {
            return {};
        }
        const QJsonObject obj = doc.object();

        QVariantMap account;
        account.insert(QStringLiteral("accountId"),
                       accountId); // payload omits it; inject the queried id
        account.insert(QStringLiteral("programOwner"), jsonStr(obj.value(QStringLiteral("program_owner"))));
        QString balance = jsonStr(obj.value(QStringLiteral("balance")));
        account.insert(QStringLiteral("balance"), balance.isEmpty() ? QStringLiteral("0") : balance);
        QString nonce = jsonStr(obj.value(QStringLiteral("nonce")));
        account.insert(QStringLiteral("nonce"), nonce.isEmpty() ? QStringLiteral("0") : nonce);
        account.insert(QStringLiteral("dataSizeBytes"), obj.value(QStringLiteral("data_size")).toInt());
        return account;
    }

    QVariantMap txJsonToMap(const QString& json) {
        if (json.isEmpty()) {
            return {};
        }
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        return doc.isObject() ? txObjectToMap(doc.object()) : QVariantMap();
    }

} // namespace

LezExplorerUiBackend::LezExplorerUiBackend() {
    setDefaultConfig(QString::fromUtf8(kDefaultConfig));
}

LezExplorerUiBackend::~LezExplorerUiBackend() {
    // Defensive: ensure no poll fires while the backend is being torn down.
    if (m_pollTimer) {
        m_pollTimer->stop();
    }
}

void LezExplorerUiBackend::onContextReady() {
    setConnectionStatus(QStringLiteral("Connecting"));

    // Reload + auto-start the previously-saved config, if any (set-once UX).
    const QString saved = readConfigFile();
    if (!saved.isEmpty()) {
        setConfigText(saved);
        startIndexerFromFile();
    }

    // The indexer pushes no events — poll the tip for live head updates.
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &LezExplorerUiBackend::pollTip);
    m_pollTimer->start();

    // pollTip() makes a synchronous cross-process call into lez_indexer_module.
    // On shutdown the host SIGTERMs that dependency and only waits ~3s before
    // SIGKILL, while our IPC timeout is ~20s — so a poll landing mid-teardown
    // blocks on a dead peer and freezes the window. Stop polling the instant the
    // app starts quitting to close that window from our side.
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        if (m_pollTimer) {
            m_pollTimer->stop();
        }
    });

    // Kick an immediate poll so the view fills without waiting a full interval.
    pollTip();
}

bool LezExplorerUiBackend::applyConfigJson(QString json) {
    const QString trimmed = json.trimmed();
    if (trimmed.isEmpty()) {
        emit error(QStringLiteral("Config is empty."));
        return false;
    }
    // Reject invalid JSON up front so we never start the indexer from garbage.
    QJsonParseError parseError;
    QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit error(QStringLiteral("Invalid JSON: %1").arg(parseError.errorString()));
        return false;
    }

    if (!writeConfigFile(json)) {
        emit error(QStringLiteral("Could not write config to %1").arg(configFilePath()));
        return false;
    }
    setConfigText(json);

    const bool ok = startIndexerFromFile();

    // Re-baseline the poller; a (re)started indexer may ingest afresh.
    m_lastPolledTip = 0;
    m_polledOnce = false;
    refreshBlocks();
    return ok;
}

bool LezExplorerUiBackend::resetIndexerCache() {
    // reset_storage stops the indexer and deletes its RocksDB store; then we
    // restart from the saved config so it re-indexes the current chain from
    // scratch. The recovery path for a store left stale against a reset chain.
    const qlonglong code = modules().lez_indexer_module.reset_storage(configFilePath());
    if (code != 0) {
        emit error(QStringLiteral("Failed to delete the indexer cache (code %1).").arg(code));
        return false;
    }

    // Store is empty now; re-baseline the poller and restart ingestion.
    m_lastPolledTip = 0;
    m_polledOnce = false;
    const bool ok = startIndexerFromFile();
    refreshBlocks();
    return ok;
}

void LezExplorerUiBackend::refreshBlocks() {
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

void LezExplorerUiBackend::loadMoreBlocks() {
    if (m_oldestLoadedId <= 1) {
        return; // already at genesis
    }
    const QString json =
        modules().lez_indexer_module.getBlocks(QString::number(m_oldestLoadedId), QStringLiteral("10"));
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

QVariantMap LezExplorerUiBackend::getBlockById(QString id) {
    return blockJsonToMap(modules().lez_indexer_module.getBlockById(id));
}

QVariantMap LezExplorerUiBackend::getBlockByHash(QString hash) {
    return blockJsonToMap(modules().lez_indexer_module.getBlockByHash(hash));
}

QVariantMap LezExplorerUiBackend::getTransaction(QString hash) {
    return txJsonToMap(modules().lez_indexer_module.getTransaction(hash));
}

QVariantMap LezExplorerUiBackend::getAccount(QString accountId) {
    return accountJsonToMap(accountId, modules().lez_indexer_module.getAccount(accountId));
}

QVariantList LezExplorerUiBackend::getTransactionsByAccount(QString accountId, int offset, int limit) {
    const QString json = modules().lez_indexer_module.getTransactionsByAccount(
        accountId, QString::number(offset), QString::number(limit)
    );
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

QVariantMap LezExplorerUiBackend::search(QString query) {
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
        }

        // Account ids are Base58 (canonical) or 64-char hex; query and display by
        // the canonical Base58 form. getAccount injects the id, so a payload with
        // only that one field means "not found".
        const QString accountId = canonicalAccountId(trimmed);
        if (!accountId.isEmpty()) {
            const QVariantMap account = getAccount(accountId);
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

quint64 LezExplorerUiBackend::applySyncStatus(const QString& json) {
    // Empty JSON means the indexer isn't running (not configured / not started);
    // a real status always carries one of the core's states.
    QString state = QStringLiteral("stopped");
    quint64 blockId = 0;
    QString errorMsg;

    if (!json.isEmpty()) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject obj = doc.object();
            state = obj.value(QStringLiteral("state")).toString(QStringLiteral("starting"));
            blockId = static_cast<quint64>(obj.value(QStringLiteral("indexedBlockId")).toDouble(0));
            errorMsg = obj.value(QStringLiteral("lastError")).toString();
        }
    }

    QVariantMap status;
    status.insert(QStringLiteral("state"), state);
    status.insert(QStringLiteral("blockId"), static_cast<qulonglong>(blockId));
    status.insert(QStringLiteral("error"), errorMsg);
    setSyncStatus(status);

    // Keep the legacy health indicator in step with the richer status.
    if (state == QLatin1String("error")) {
        setConnectionStatus(QStringLiteral("Error"));
    } else if (state == QLatin1String("syncing") || state == QLatin1String("caught_up")) {
        setConnectionStatus(QStringLiteral("Connected"));
    } else {
        setConnectionStatus(QStringLiteral("Connecting"));
    }

    return blockId;
}

void LezExplorerUiBackend::pollTip() {
    // One status call drives both the sync banner and the feed: indexedBlockId
    // is the L2 tip we page from.
    const quint64 tip = applySyncStatus(modules().lez_indexer_module.getStatus());
    if (tip == 0) {
        // No finalized block yet (starting / syncing from genesis, or stopped);
        // syncStatus already conveys which, so just wait for the next poll.
        return;
    }

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

QVariantMap LezExplorerUiBackend::fetchBlock(quint64 blockId) {
    return blockJsonToMap(modules().lez_indexer_module.getBlockById(QString::number(blockId)));
}

QVariantList LezExplorerUiBackend::parseBlockArrayJson(const QString& json) const {
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

namespace {
    // The writable path where the edited config JSON is persisted and the indexer
    // is started from (so the UI never deals with paths). Same logic, shared by the
    // const and non-const accessors.
    QString resolveConfigFilePath() {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (dir.isEmpty()) {
            dir = QDir::tempPath();
        }
        return QDir(dir).filePath(QStringLiteral("indexer_config.json"));
    }
} // namespace

QString LezExplorerUiBackend::configFilePath() {
    if (m_configFilePath.isEmpty()) {
        m_configFilePath = resolveConfigFilePath();
    }
    return m_configFilePath;
}

bool LezExplorerUiBackend::writeConfigFile(const QString& json) {
    const QString path = configFilePath();
    const QDir dir = QFileInfo(path).dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    const QByteArray bytes = json.toUtf8();
    const bool ok = file.write(bytes) == bytes.size();
    file.close();
    return ok;
}

QString LezExplorerUiBackend::readConfigFile() const {
    QFile file(m_configFilePath.isEmpty() ? resolveConfigFilePath() : m_configFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content;
}

bool LezExplorerUiBackend::startIndexerFromFile() {
    // stops the indexer if it's already running, then starts it with the config
    // file path.
    //
    // this has the effect of "restarting" the indexer with the new config.
    modules().lez_indexer_module.stop_indexer();
    const qlonglong code = modules().lez_indexer_module.start_indexer(configFilePath());
    if (code != 0) {
        emit error(startErrorMessage(code));
        return false;
    }
    return true;
}
