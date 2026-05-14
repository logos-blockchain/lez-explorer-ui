#pragma once

#include <QWidget>

class QPushButton;

class NavigationBar : public QWidget {
    Q_OBJECT

public:
    explicit NavigationBar(QWidget* parent = nullptr);

    void setBackEnabled(bool enabled);
    void setForwardEnabled(bool enabled);

signals:
    void backClicked();
    void forwardClicked();
    void homeClicked();

private:
    QPushButton* m_backBtn = nullptr;
    QPushButton* m_forwardBtn = nullptr;
};
