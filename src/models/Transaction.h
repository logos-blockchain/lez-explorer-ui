#pragma once

#include <QString>
#include <QVector>

enum class TransactionType {
    Public,
    PrivacyPreserving,
    ProgramDeployment
};

inline QString transactionTypeToString(TransactionType type)
{
    switch (type) {
    case TransactionType::Public: return "Public";
    case TransactionType::PrivacyPreserving: return "Privacy-Preserving";
    case TransactionType::ProgramDeployment: return "Program Deployment";
    }
    return "Unknown";
}

struct AccountRef {
    QString accountId;
    QString nonce;
};

struct Transaction {
    QString hash;
    TransactionType type = TransactionType::Public;

    // Public transaction fields
    QString programId;
    QVector<AccountRef> accounts;
    QVector<quint32> instructionData;
    int signatureCount = 0;
    int proofSizeBytes = 0;

    // Privacy-preserving transaction fields
    int newCommitmentsCount = 0;
    int nullifiersCount = 0;
    int encryptedStatesCount = 0;
    quint64 validityWindowStart = 0;
    quint64 validityWindowEnd = 0;

    // Program deployment fields
    int bytecodeSizeBytes = 0;
};
