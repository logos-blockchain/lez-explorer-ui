#include "SearchBar.h"
#include "Style.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

SearchBar::SearchBar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->setSpacing(8);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Search by block ID / block hash / tx hash / account ID...");
    m_input->setMinimumHeight(38);
    m_input->setStyleSheet(Style::searchInput());

    auto* searchBtn = new QPushButton("Search", this);
    searchBtn->setMinimumHeight(38);
    searchBtn->setStyleSheet(Style::searchButton());

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
