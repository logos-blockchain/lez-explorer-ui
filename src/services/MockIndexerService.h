#pragma once

#include "IndexerService.h"

#include <QMap>

class MockIndexerService : public IndexerService {
public:
    MockIndexerService();

    std::optional<Account> getAccount(const QString& accountId) override;
    std::optional<Block> getBlockById(quint64 blockId) override;
    std::optional<Block> getBlockByHash(const QString& hash) override;
    std::optional<Transaction> getTransaction(const QString& hash) override;
    QVector<Block> getBlocks(std::optional<quint64> before, int limit) override;
    quint64 getLatestBlockId() override;
    QVector<Transaction> getTransactionsByAccount(const QString& accountId, int offset, int limit) override;
    SearchResults search(const QString& query) override;

private:
    void generateData();
    QString randomHash();
    QString randomAccountId();
    Transaction generatePublicTransaction();
    Transaction generatePrivacyPreservingTransaction();
    Transaction generateProgramDeploymentTransaction();

    QVector<Block> m_blocks;
    QMap<QString, Block> m_blocksByHash;
    QMap<QString, Transaction> m_transactionsByHash;
    QMap<QString, Account> m_accounts;
};
