#include "MockIndexerService.h"

#include <QRandomGenerator>
#include <algorithm>

namespace {

QString randomHexString(int bytes)
{
    QString result;
    result.reserve(bytes * 2);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < bytes; ++i) {
        result += QString("%1").arg(rng->bounded(256), 2, 16, QChar('0'));
    }
    return result;
}

// Generate a base58-like string (simplified — uses alphanumeric chars without 0/O/I/l)
QString randomBase58String(int length)
{
    static const char chars[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    QString result;
    result.reserve(length);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < length; ++i) {
        result += chars[rng->bounded(static_cast<int>(sizeof(chars) - 1))];
    }
    return result;
}

} // namespace

MockIndexerService::MockIndexerService()
{
    generateData();
}

QString MockIndexerService::randomHash()
{
    return randomHexString(32);
}

QString MockIndexerService::randomAccountId()
{
    return randomBase58String(44);
}

Transaction MockIndexerService::generatePublicTransaction()
{
    auto* rng = QRandomGenerator::global();

    Transaction tx;
    tx.hash = randomHash();
    tx.type = TransactionType::Public;
    tx.programId = randomBase58String(44);
    tx.signatureCount = rng->bounded(1, 4);
    tx.proofSizeBytes = rng->bounded(0, 2048);

    int accountCount = rng->bounded(1, 5);
    for (int i = 0; i < accountCount; ++i) {
        // Reuse existing accounts sometimes
        QString accId;
        if (!m_accounts.isEmpty() && rng->bounded(100) < 60) {
            auto keys = m_accounts.keys();
            accId = keys[rng->bounded(keys.size())];
        } else {
            accId = randomAccountId();
        }
        tx.accounts.append({accId, QString::number(rng->bounded(1, 100))});
    }

    int instrCount = rng->bounded(1, 8);
    for (int i = 0; i < instrCount; ++i) {
        tx.instructionData.append(rng->generate());
    }

    return tx;
}

Transaction MockIndexerService::generatePrivacyPreservingTransaction()
{
    auto* rng = QRandomGenerator::global();

    Transaction tx;
    tx.hash = randomHash();
    tx.type = TransactionType::PrivacyPreserving;
    tx.proofSizeBytes = rng->bounded(512, 4096);
    tx.newCommitmentsCount = rng->bounded(1, 6);
    tx.nullifiersCount = rng->bounded(1, 4);
    tx.encryptedStatesCount = rng->bounded(1, 3);
    tx.validityWindowStart = rng->bounded(1, 50);
    tx.validityWindowEnd = tx.validityWindowStart + rng->bounded(50, 200);

    int accountCount = rng->bounded(1, 4);
    for (int i = 0; i < accountCount; ++i) {
        QString accId;
        if (!m_accounts.isEmpty() && rng->bounded(100) < 60) {
            auto keys = m_accounts.keys();
            accId = keys[rng->bounded(keys.size())];
        } else {
            accId = randomAccountId();
        }
        tx.accounts.append({accId, QString::number(rng->bounded(1, 100))});
    }

    return tx;
}

Transaction MockIndexerService::generateProgramDeploymentTransaction()
{
    auto* rng = QRandomGenerator::global();

    Transaction tx;
    tx.hash = randomHash();
    tx.type = TransactionType::ProgramDeployment;
    tx.bytecodeSizeBytes = rng->bounded(1024, 65536);
    tx.signatureCount = 1;

    return tx;
}

void MockIndexerService::generateData()
{
    auto* rng = QRandomGenerator::global();

    // Generate accounts
    for (int i = 0; i < 15; ++i) {
        Account acc;
        acc.accountId = randomAccountId();
        acc.programOwner = randomBase58String(44);
        acc.balance = QString::number(rng->bounded(0, 1000000));
        acc.nonce = QString::number(rng->bounded(0, 500));
        acc.dataSizeBytes = rng->bounded(0, 4096);
        m_accounts[acc.accountId] = acc;
    }

    // Generate blocks
    QString prevHash = QString(64, '0');
    QDateTime timestamp = QDateTime::currentDateTimeUtc().addSecs(-25 * 12); // ~5 min ago

    for (quint64 id = 1; id <= 25; ++id) {
        Block block;
        block.blockId = id;
        block.prevBlockHash = prevHash;
        block.hash = randomHash();
        block.timestamp = timestamp;
        block.signature = randomHexString(64);
        block.bedrockParentId = randomHexString(32);

        // Older blocks are finalized, middle ones safe, recent ones pending
        if (id <= 15) {
            block.bedrockStatus = BedrockStatus::Finalized;
        } else if (id <= 22) {
            block.bedrockStatus = BedrockStatus::Safe;
        } else {
            block.bedrockStatus = BedrockStatus::Pending;
        }

        // Generate 1-5 transactions per block
        int txCount = rng->bounded(1, 6);
        for (int t = 0; t < txCount; ++t) {
            Transaction tx;
            int typeRoll = rng->bounded(100);
            if (typeRoll < 60) {
                tx = generatePublicTransaction();
            } else if (typeRoll < 85) {
                tx = generatePrivacyPreservingTransaction();
            } else {
                tx = generateProgramDeploymentTransaction();
            }
            block.transactions.append(tx);
            m_transactionsByHash[tx.hash] = tx;

            // Ensure accounts referenced in transactions exist
            for (const auto& accRef : tx.accounts) {
                if (!m_accounts.contains(accRef.accountId)) {
                    Account acc;
                    acc.accountId = accRef.accountId;
                    acc.programOwner = randomBase58String(44);
                    acc.balance = QString::number(rng->bounded(0, 1000000));
                    acc.nonce = accRef.nonce;
                    acc.dataSizeBytes = rng->bounded(0, 4096);
                    m_accounts[acc.accountId] = acc;
                }
            }
        }

        m_blocks.append(block);
        m_blocksByHash[block.hash] = block;
        prevHash = block.hash;
        timestamp = timestamp.addSecs(12);
    }
}

std::optional<Account> MockIndexerService::getAccount(const QString& accountId)
{
    auto it = m_accounts.find(accountId);
    if (it != m_accounts.end()) {
        return *it;
    }
    return std::nullopt;
}

std::optional<Block> MockIndexerService::getBlockById(quint64 blockId)
{
    if (blockId >= 1 && blockId <= static_cast<quint64>(m_blocks.size())) {
        return m_blocks[static_cast<int>(blockId - 1)];
    }
    return std::nullopt;
}

std::optional<Block> MockIndexerService::getBlockByHash(const QString& hash)
{
    auto it = m_blocksByHash.find(hash);
    if (it != m_blocksByHash.end()) {
        return *it;
    }
    return std::nullopt;
}

std::optional<Transaction> MockIndexerService::getTransaction(const QString& hash)
{
    auto it = m_transactionsByHash.find(hash);
    if (it != m_transactionsByHash.end()) {
        return *it;
    }
    return std::nullopt;
}

QVector<Block> MockIndexerService::getBlocks(std::optional<quint64> before, int limit)
{
    QVector<Block> result;
    int startIdx = m_blocks.size() - 1;

    if (before.has_value()) {
        startIdx = static_cast<int>(*before) - 2; // -1 for 0-index, -1 for "before"
    }

    for (int i = startIdx; i >= 0 && result.size() < limit; --i) {
        result.append(m_blocks[i]);
    }

    return result;
}

quint64 MockIndexerService::getLatestBlockId()
{
    if (m_blocks.isEmpty()) return 0;
    return m_blocks.last().blockId;
}

QVector<Transaction> MockIndexerService::getTransactionsByAccount(const QString& accountId, int offset, int limit)
{
    QVector<Transaction> result;

    // Search all transactions for ones referencing this account
    for (auto it = m_transactionsByHash.begin(); it != m_transactionsByHash.end(); ++it) {
        for (const auto& accRef : it->accounts) {
            if (accRef.accountId == accountId) {
                result.append(*it);
                break;
            }
        }
    }

    // Apply offset and limit
    if (offset >= result.size()) return {};
    return result.mid(offset, limit);
}

SearchResults MockIndexerService::search(const QString& query)
{
    SearchResults results;
    QString trimmed = query.trimmed();

    if (trimmed.isEmpty()) return results;

    // Try as block ID (numeric)
    bool ok = false;
    quint64 blockId = trimmed.toULongLong(&ok);
    if (ok) {
        auto block = getBlockById(blockId);
        if (block) {
            results.blocks.append(*block);
        }
    }

    // Try as hash (hex string, 64 chars)
    if (trimmed.size() == 64) {
        auto block = getBlockByHash(trimmed);
        if (block) {
            results.blocks.append(*block);
        }
        auto tx = getTransaction(trimmed);
        if (tx) {
            results.transactions.append(*tx);
        }
    }

    // Try as account ID
    auto account = getAccount(trimmed);
    if (account) {
        results.accounts.append(*account);
    }

    return results;
}
