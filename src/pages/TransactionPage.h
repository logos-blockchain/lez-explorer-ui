#pragma once

#include "models/Transaction.h"

#include <QWidget>

class TransactionPage : public QWidget {
    Q_OBJECT

public:
    explicit TransactionPage(const Transaction& tx, QWidget* parent = nullptr);

signals:
    void accountClicked(const QString& accountId);
};
