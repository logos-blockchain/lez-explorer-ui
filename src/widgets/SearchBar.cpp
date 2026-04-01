#include "SearchBar.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

SearchBar::SearchBar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Search by block ID / block hash / tx hash / account ID...");
    m_input->setMinimumHeight(32);

    auto* searchBtn = new QPushButton("Search", this);
    searchBtn->setMinimumHeight(32);

    layout->addWidget(m_input, 1);
    layout->addWidget(searchBtn);

    connect(m_input, &QLineEdit::returnPressed, this, [this]() {
        emit searchRequested(m_input->text());
    });
    connect(searchBtn, &QPushButton::clicked, this, [this]() {
        emit searchRequested(m_input->text());
    });
}

void SearchBar::clear()
{
    m_input->clear();
}
