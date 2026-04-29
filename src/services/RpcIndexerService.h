#pragma once

#include "IndexerService.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QWebSocket>

#include <optional>

class QEventLoop;

class RpcIndexerService : public IndexerService {
    Q_OBJECT

public:
    explicit RpcIndexerService(const QString& endpoint = IndexerService::defaultEndpoint(), QObject* parent = nullptr);
    ~RpcIndexerService() override;

    QString endpoint() const override;
    void setEndpoint(const QString& endpoint) override;

    std::optional<Account> getAccount(const QString& accountId) override;
    std::optional<Block> getBlockById(quint64 blockId) override;
    std::optional<Block> getBlockByHash(const QString& hash) override;
    std::optional<Transaction> getTransaction(const QString& hash) override;
    QVector<Block> getBlocks(std::optional<quint64> before, int limit) override;
    quint64 getLastFinalizedBlockId() override;
    QVector<Transaction> getTransactionsByAccount(const QString& accountId, int offset, int limit) override;
    SearchResults search(const QString& query) override;

private slots:
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString& message);

private:
    void handleFinalizedBlockNotification(const QJsonValue& result);
    void fetchAndEmitFinalizedBlock(quint64 blockId);

    bool ensureConnected();
    void ensureFinalizedBlocksSubscription();
    std::optional<QJsonValue> callRpc(const QString& method, const QJsonArray& params = QJsonArray());

    std::optional<Block> parseBlock(const QJsonValue& value) const;
    std::optional<Transaction> parseTransaction(const QJsonValue& value) const;
    std::optional<Account> parseAccount(const QString& accountId, const QJsonValue& value) const;

    static QString jsonValueToString(const QJsonValue& value);
    static quint64 jsonValueToU64(const QJsonValue& value);
    static int base64Size(const QString& value);

    QWebSocket m_socket;
    QUrl m_endpoint;
    quint64 m_nextRequestId = 1;
    QHash<quint64, QJsonObject> m_responses;
    QHash<quint64, QEventLoop*> m_waiters;
    quint64 m_subscriptionRequestId = 0;
    QJsonValue m_subscriptionId;
    bool m_subscriptionActive = false;
    quint64 m_lastNotifiedBlockId = 0;
};
