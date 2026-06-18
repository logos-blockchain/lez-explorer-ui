#pragma once

#include "IndexerService.h"

#include <QString>
#include <optional>

class LogosAPI;
class LogosAPIClient;
class QTimer;
class QJsonObject;

// IndexerService implementation that talks to `lez_indexer_module` IN-PROCESS
// over the Logos protocol (Qt Remote Objects), via
// LogosAPI::getClient(...)->invokeRemoteMethod(...). Replaces the old WebSocket
// JSON-RPC RpcIndexerService.
//
// The provider returns a flat, compact JSON schema (hex hashes; 64/128-bit
// values as decimal strings; counts/sizes as numbers; "" == not-found), which
// differs from the old RPC schema — so this class carries its OWN parsers.
//
// The provider exposes no new-block push event, so live head updates are POLLED
// on a 2 s timer over getLastFinalizedBlockId.
//
// `endpoint()`/`setEndpoint()` are repurposed to the indexer CONFIG PATH (an
// absolute path to indexer_config.json) used to best-effort start the indexer;
// blank means "the indexer is already running" (start is idempotent provider-side).
class LpIndexerService : public IndexerService {
    Q_OBJECT

public:
    explicit LpIndexerService(
        LogosAPI* logosAPI,
        const QString& configPath = QString(),
        const QString& port = QStringLiteral("8779"),
        QObject* parent = nullptr
    );
    ~LpIndexerService() override;

    QString endpoint() const override;            // the indexer config path
    void setEndpoint(const QString& endpoint) override;  // set config path + (re)start

    std::optional<Account> getAccount(const QString& accountId) override;
    std::optional<Block> getBlockById(quint64 blockId) override;
    std::optional<Block> getBlockByHash(const QString& hash) override;
    std::optional<Transaction> getTransaction(const QString& hash) override;
    QVector<Block> getBlocks(std::optional<quint64> before, int limit) override;
    quint64 getLastFinalizedBlockId() override;
    QVector<Transaction> getTransactionsByAccount(const QString& accountId, int offset, int limit) override;
    SearchResults search(const QString& query) override;

private:
    void startIndexer();
    void pollForNewBlocks();

    // Parsers for the provider's compact schema.
    std::optional<Block> parseBlock(const QString& json) const;
    std::optional<Block> parseBlockObject(const QJsonObject& obj) const;
    std::optional<Transaction> parseTransaction(const QJsonObject& obj) const;
    std::optional<Account> parseAccount(const QString& accountId, const QString& json) const;

    LogosAPI* m_logosAPI = nullptr;
    LogosAPIClient* m_client = nullptr;
    QString m_configPath;
    QString m_port;
    QTimer* m_pollTimer = nullptr;
    quint64 m_lastNotifiedBlockId = 0;
    bool m_polledOnce = false;
};
