#pragma once

#include <QWidget>

class ExplorerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ExplorerWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};
