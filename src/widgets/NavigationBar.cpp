#include "NavigationBar.h"

#include <QHBoxLayout>
#include <QPushButton>

NavigationBar::NavigationBar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_backBtn = new QPushButton("<", this);
    m_backBtn->setFixedWidth(40);
    m_backBtn->setMinimumHeight(32);
    m_backBtn->setEnabled(false);
    m_backBtn->setToolTip("Back");

    m_forwardBtn = new QPushButton(">", this);
    m_forwardBtn->setFixedWidth(40);
    m_forwardBtn->setMinimumHeight(32);
    m_forwardBtn->setEnabled(false);
    m_forwardBtn->setToolTip("Forward");

    auto* homeBtn = new QPushButton("Home", this);
    homeBtn->setMinimumHeight(32);

    layout->addWidget(m_backBtn);
    layout->addWidget(m_forwardBtn);
    layout->addWidget(homeBtn);
    layout->addStretch();

    connect(m_backBtn, &QPushButton::clicked, this, &NavigationBar::backClicked);
    connect(m_forwardBtn, &QPushButton::clicked, this, &NavigationBar::forwardClicked);
    connect(homeBtn, &QPushButton::clicked, this, &NavigationBar::homeClicked);
}

void NavigationBar::setBackEnabled(bool enabled)
{
    m_backBtn->setEnabled(enabled);
}

void NavigationBar::setForwardEnabled(bool enabled)
{
    m_forwardBtn->setEnabled(enabled);
}
