#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>

#include "Transaction.h"

enum class BedrockStatus {
    Pending,
    Safe,
    Finalized
};

inline QString bedrockStatusToString(BedrockStatus status)
{
    switch (status) {
    case BedrockStatus::Pending: return "Pending";
    case BedrockStatus::Safe: return "Safe";
    case BedrockStatus::Finalized: return "Finalized";
    }
    return "Unknown";
}

struct Block {
    quint64 blockId = 0;
    QString hash;
    QString prevBlockHash;
    QDateTime timestamp;
    QString signature;
    QVector<Transaction> transactions;
    BedrockStatus bedrockStatus = BedrockStatus::Pending;
};
