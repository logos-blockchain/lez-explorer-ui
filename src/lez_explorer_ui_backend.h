#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "logos_ui_plugin_context.h"
#include "rep_lez_explorer_ui_source.h"

class QTimer;

/**
 * @brief Backend for the LEZ block explorer ui_qml view (universal model).
 *
 * The only hand-written C++ in this module: it derives the repc-generated
 * `LezExplorerUiSimpleSource` (so it implements the `.rep` view contract) and
 * `LogosUiPluginContext` (which supplies `onContextReady()` and the Qt-typed
 * `modules()` accessors for the `lez_indexer_module` dependency). The
 * `*Plugin` / `*Interface` glue is generated.
 *
 * It reads from the indexer with typed calls — `modules().lez_indexer_module.*`
 * — which return the indexer's compact JSON as QString. This class converts
 * that JSON into QVariantMap / QVariantList (schema preserved from the former
 * `LpIndexerService`) for the QML view, and runs a 2 s poll on the chain tip to
 * keep `recentBlocks` / `chainHeight` live (the indexer emits no events).
 *
 * Config is edited in-app (Settings page) as JSON, not as a path: the indexer's
 * `start_indexer` takes a file path, so this class persists the edited JSON to a
 * writable file and starts the indexer from there. The last-saved config is
 * reloaded and auto-started on the next launch.
 */
class LezExplorerUiBackend : public LezExplorerUiSimpleSource, public LogosUiPluginContext {
public:
    LezExplorerUiBackend();
    ~LezExplorerUiBackend() override;

    // Fires when ui-host wires the plugin's LogosAPI: modules() is live, so we
    // load any saved config, start it, and start the poll here.
    void onContextReady() override;

    // .rep SLOTs.
    bool applyConfigJson(QString json) override;
    bool resetIndexerCache() override;
    void refreshBlocks() override;
    void loadMoreBlocks() override;
    QVariantMap getBlockById(QString id) override;
    QVariantMap getBlockByHash(QString hash) override;
    QVariantMap getTransaction(QString hash) override;
    QVariantMap getAccount(QString accountId) override;
    QVariantList getTransactionsByAccount(QString accountId, int offset, int limit) override;
    QVariantMap search(QString query) override;

private:
    // Tip poll: detect new heads and append/rebuild the recent-blocks feed.
    void pollTip();
    // Parse the indexer's status JSON, publish the `syncStatus` PROP + the
    // legacy `connectionStatus`, and return the indexed tip (0 if none yet).
    quint64 applySyncStatus(const QString& json);
    // Fetch a single block as a normalized map ({} if missing).
    QVariantMap fetchBlock(quint64 blockId);
    // Parse a getBlocks(...) JSON array payload into a list of block maps.
    QVariantList parseBlockArrayJson(const QString& json) const;

    // Writable path where the edited config JSON is persisted, and where the
    // indexer is started from. Computed once (lazily).
    QString configFilePath();
    bool writeConfigFile(const QString& json);
    QString readConfigFile() const;
    // start_indexer from configFilePath(); returns true on success.
    bool startIndexerFromFile();

    QString m_configFilePath;
    // Human message of the most recent failed start, cleared on a successful
    // start. Injected into syncStatus while the indexer is stopped so the
    // banner can say WHY instead of a neutral "not running".
    QString m_lastStartError;
    // Remaining pollTip re-attempts of the launch auto-start. The indexer
    // module process can come up long after onContextReady, and a typed call
    // made before it listens silently returns success without doing anything.
    int m_autoStartRetriesLeft = 0;
    QVariantList m_recentBlocks;
    quint64 m_newestLoadedId = 0; // highest block id currently in m_recentBlocks
    quint64 m_oldestLoadedId = 0; // lowest block id currently in m_recentBlocks
    quint64 m_lastPolledTip = 0;
    bool m_polledOnce = false;
    QTimer* m_pollTimer = nullptr;
};
