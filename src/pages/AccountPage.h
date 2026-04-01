#pragma once

#include "models/Account.h"
#include "services/IndexerService.h"

#include <QWidget>

class AccountPage : public QWidget {
    Q_OBJECT

public:
    explicit AccountPage(const Account& account, IndexerService* indexer, QWidget* parent = nullptr);

signals:
    void transactionClicked(const QString& hash);
};
