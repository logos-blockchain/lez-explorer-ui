#pragma once

#include <QWidget>

class QLineEdit;

class SearchBar : public QWidget {
    Q_OBJECT

public:
    explicit SearchBar(QWidget* parent = nullptr);

    void clear();

signals:
    void searchRequested(const QString& query);

private:
    QLineEdit* m_input = nullptr;
};
