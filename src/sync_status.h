#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

// Parsed view of the indexer module's `getStatus` JSON (schema owned by
// indexer_core and passed through the FFI verbatim). `state` carries the
// core's `IndexerSyncState` variant name as serialized (Starting / Syncing /
// CaughtUp / Error / Stalled), plus the UI-only "Stopped" when the indexer
// isn't running at all.
struct SyncStatus {
    QString state;
    quint64 blockId = 0;
    QString error;
};

inline SyncStatus parseSyncStatus(const QString& json) {
    // Empty JSON means the indexer isn't running (not configured / not started);
    // a real status always carries one of the core's states.
    SyncStatus status;
    status.state = QStringLiteral("Stopped");

    if (json.isEmpty()) {
        return status;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return status;
    }

    // A missing state from a running indexer reads as still booting so the
    // banner shows progress rather than a bogus error.
    const QJsonObject obj = doc.object();
    status.state = obj.value(QStringLiteral("state")).toString(QStringLiteral("Starting"));
    status.blockId = static_cast<quint64>(obj.value(QStringLiteral("indexed_block_id")).toDouble(0));
    status.error = obj.value(QStringLiteral("last_error")).toString();
    return status;
}
