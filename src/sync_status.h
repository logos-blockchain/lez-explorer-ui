#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

// Parsed view of the core module's `getStatus` JSON, mapped onto the states the
// banner shows: "Starting" until the sequencer is up, "Syncing" while the executor
// replays the channel's history, "CaughtUp" once it is online, and the UI-only
// "Stopped" when no sequencer runs (with `startErrorCode` if its start failed).
struct SyncStatus {
    QString state;
    // Chain height; 0 until the executor reports one.
    quint64 blockId = 0;
    // Bootstrap progress in Bedrock slots; 0 when not reported.
    quint64 replayedSlot = 0;
    quint64 targetSlot = 0;
    // FfiOperationError of a failed start; 0 otherwise.
    qlonglong startErrorCode = 0;
    QString error;
};

inline SyncStatus parseSyncStatus(const QString& json) {
    // Empty JSON means the sequencer isn't running (not configured / not started).
    SyncStatus status;
    status.state = QStringLiteral("Stopped");

    if (json.isEmpty()) {
        return status;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        status.state = QStringLiteral("Error");
        status.error = QStringLiteral("Invalid sequencer status: %1").arg(json);
        return status;
    }

    // 64-bit values arrive as decimal strings, or null when not reported.
    const QJsonObject obj = doc.object();
    const auto u64 = [&obj](const char* key) { return obj.value(QLatin1String(key)).toString().toULongLong(); };
    const QString state = obj.value(QStringLiteral("state")).toString();
    if (state == QLatin1String("StartFailed")) {
        status.startErrorCode = static_cast<qlonglong>(obj.value(QStringLiteral("error_code")).toDouble());
        return status;
    }
    if (state == QLatin1String("Starting")) {
        status.state = state;
        return status;
    }
    status.state = state == QLatin1String("Online") ? QStringLiteral("CaughtUp") : QStringLiteral("Syncing");
    status.blockId = u64("height");
    status.replayedSlot = u64("replayed_up_to_slot");
    status.targetSlot = u64("target_slot");
    return status;
}
