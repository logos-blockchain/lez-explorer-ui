#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

// TODO: Update it with new JSONs when [955](https://github.com/logos-blockchain/logos-execution-zone/pull/955) merged in LEZ.

// Parsed view of the core module's `getStatus` JSON (schema owned by
// core_ffi and passed through the FFI verbatim). `state` carries the
// variant name as serialized (Starting / Syncing /
// CaughtUp / Error / Stalled), plus the UI-only "Stopped" when the sequencer
// isn't running at all.
struct SyncStatus {
    QString state;
    quint64 blockId = 0;
    QString error;
};

inline SyncStatus parseSyncStatus(const QString& json) {
    // Empty JSON means the sequencer isn't running (not configured / not started);
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

    // A missing state from a running sequencer reads as still booting so the
    // banner shows progress rather than a bogus error.
    const QJsonObject obj = doc.object();
    status.state = obj.value(QStringLiteral("state")).toString(QStringLiteral("Starting"));
    status.blockId = static_cast<quint64>(obj.value(QStringLiteral("indexed_block_id")).toDouble(0));
    status.error = obj.value(QStringLiteral("last_error")).toString();
    return status;
}

// TODO: Remove it when [955](https://github.com/logos-blockchain/logos-execution-zone/pull/955) merged in LEZ.

inline SyncStatus parseSyncStatusFromLatestBlockId(const QString& latest_block_id) {
    // Empty means the sequencer isn't running (not configured / not started).
    SyncStatus status;
    status.state = QStringLiteral("Stopped");

    const QString trimmed = latest_block_id.trimmed();
    if (trimmed.isEmpty()) {
        return status;
    }

    bool ok = false;
    const quint64 blockId = trimmed.toULongLong(&ok);
    if (!ok) {
        status.state = QStringLiteral("Error");
        status.error = QStringLiteral("Invalid latest block id: %1").arg(trimmed);
        return status;
    }

    status.state = QStringLiteral("Syncing");
    status.blockId = blockId;
    return status;
}
