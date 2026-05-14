#pragma once

#include <QString>

struct Account {
    QString accountId;
    QString programOwner;
    QString balance;
    QString nonce;
    int dataSizeBytes = 0;
};
