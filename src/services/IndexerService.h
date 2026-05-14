#pragma once

#include "models/Block.h"
#include "models/Transaction.h"
#include "models/Account.h"

#include <QObject>
#include <QVector>
#include <optional>

struct SearchResults {
    QVector<Block> blocks;
    QVector<Transaction> transactions;
    QVector<Account> accounts;
};

class IndexerService : public QObject {
    Q_OBJECT

public:
    explicit IndexerService(QObject* parent = nullptr) : QObject(parent) {}
    ~IndexerService() override = default;

    virtual std::optional<Account> getAccount(const QString& accountId) = 0;
    virtual std::optional<Block> getBlockById(quint64 blockId) = 0;
    virtual std::optional<Block> getBlockByHash(const QString& hash) = 0;
    virtual std::optional<Transaction> getTransaction(const QString& hash) = 0;
    virtual QVector<Block> getBlocks(std::optional<quint64> before, int limit) = 0;
    virtual quint64 getLatestBlockId() = 0;
    virtual QVector<Transaction> getTransactionsByAccount(const QString& accountId, int offset, int limit) = 0;
    virtual SearchResults search(const QString& query) = 0;

signals:
    void newBlockAdded(Block block);
};
