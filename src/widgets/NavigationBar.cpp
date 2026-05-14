#include "NavigationBar.h"
#include "Style.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QIcon>

NavigationBar::NavigationBar(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    const QSize iconSize(22, 22);

    m_backBtn = new QPushButton(this);
    m_backBtn->setIcon(QIcon(":/icons/arrow-left.svg"));
    m_backBtn->setIconSize(iconSize);
    m_backBtn->setFixedSize(42, 42);
    m_backBtn->setEnabled(false);
    m_backBtn->setToolTip("Back");
    m_backBtn->setStyleSheet(Style::navButton());

    m_forwardBtn = new QPushButton(this);
    m_forwardBtn->setIcon(QIcon(":/icons/arrow-right.svg"));
    m_forwardBtn->setIconSize(iconSize);
    m_forwardBtn->setFixedSize(42, 42);
    m_forwardBtn->setEnabled(false);
    m_forwardBtn->setToolTip("Forward");
    m_forwardBtn->setStyleSheet(Style::navButton());

    auto* homeBtn = new QPushButton(this);
    homeBtn->setIcon(QIcon(":/icons/home.svg"));
    homeBtn->setIconSize(iconSize);
    homeBtn->setFixedSize(42, 42);
    homeBtn->setToolTip("Home");
    homeBtn->setStyleSheet(Style::navButton());

    auto* titleLabel = new QLabel("LEZ Explorer");
    titleLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; margin-left: 8px;").arg(Style::Color::accent()));

    layout->addWidget(m_backBtn);
    layout->addWidget(m_forwardBtn);
    layout->addWidget(homeBtn);
    layout->addWidget(titleLabel);
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
