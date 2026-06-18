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

    // Over the Logos protocol the "endpoint" is repurposed to the indexer
    // config path (absolute path to indexer_config.json); blank means the
    // indexer is already running. No network URL is involved anymore.
    static QString defaultEndpoint()
    {
        return QString();
    }

    virtual QString endpoint() const = 0;
    virtual void setEndpoint(const QString& endpoint) = 0;

    virtual std::optional<Account> getAccount(const QString& accountId) = 0;
    virtual std::optional<Block> getBlockById(quint64 blockId) = 0;
    virtual std::optional<Block> getBlockByHash(const QString& hash) = 0;
    virtual std::optional<Transaction> getTransaction(const QString& hash) = 0;
    virtual QVector<Block> getBlocks(std::optional<quint64> before, int limit) = 0;
    virtual quint64 getLastFinalizedBlockId() = 0;
    virtual QVector<Transaction> getTransactionsByAccount(const QString& accountId, int offset, int limit) = 0;
    virtual SearchResults search(const QString& query) = 0;

signals:
    void newBlockAdded(Block block);
};
