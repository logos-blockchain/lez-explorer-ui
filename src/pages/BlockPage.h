#pragma once

#include "models/Block.h"

#include <QWidget>

class BlockPage : public QWidget {
    Q_OBJECT

public:
    explicit BlockPage(const Block& block, QWidget* parent = nullptr);

signals:
    void blockClicked(quint64 blockId);
    void transactionClicked(const QString& hash);
};
