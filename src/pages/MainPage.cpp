#include "MainPage.h"
#include "Style.h"
#include "widgets/ClickableFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QPixmap>
#include <QPushButton>

MainPage::MainPage(IndexerService* indexer, QWidget* parent)
    : QWidget(parent)
    , m_indexer(indexer)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // Health indicator
    auto* healthRow = new QHBoxLayout();
    auto* healthIcon = new QLabel(this);
    healthIcon->setPixmap(QIcon(":/icons/activity.svg").pixmap(20, 20));
    m_healthLabel = new QLabel(this);
    healthRow->addWidget(healthIcon);
    healthRow->addWidget(m_healthLabel);
    healthRow->addStretch();
    outerLayout->addLayout(healthRow);

    // Scroll area for content
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; } QWidget#scrollContent { background: transparent; }");

    auto* scrollContent = new QWidget();
    scrollContent->setObjectName("scrollContent");
    m_contentLayout = new QVBoxLayout(scrollContent);
    m_contentLayout->setAlignment(Qt::AlignTop);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);

    refresh();
}

QWidget* MainPage::createSectionHeader(const QString& title, const QString& iconPath)
{
    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 12, 0, 6);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignVCenter);

    if (!iconPath.isEmpty()) {
        auto* icon = new QLabel();
        icon->setPixmap(QIcon(iconPath).pixmap(24, 24));
        icon->setFixedSize(24, 24);
        layout->addWidget(icon);
    }

    auto* label = new QLabel(title);
    QFont font = label->font();
    font.setPointSize(20);
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet(Style::sectionHeader());
    layout->addWidget(label);
    layout->addStretch();

    return container;
}

void MainPage::addBlockRow(QVBoxLayout* layout, const Block& block)
{
    auto* frame = new ClickableFrame();
    frame->setFrameShape(QFrame::NoFrame);
    frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

    auto* row = new QHBoxLayout(frame);
    row->setSpacing(10);

    auto* icon = new QLabel();
    icon->setPixmap(QIcon(":/icons/box.svg").pixmap(20, 20));
    row->addWidget(icon);

    auto* idLabel = new QLabel(QString("Block #%1").arg(block.blockId));
    QFont boldFont = idLabel->font();
    boldFont.setBold(true);
    idLabel->setFont(boldFont);

    QString statusStr = bedrockStatusToString(block.bedrockStatus);
    auto* statusLabel = new QLabel(statusStr);
    statusLabel->setStyleSheet(Style::badge(Style::statusColor(statusStr)));

    auto* hashLabel = new QLabel(block.hash);
    hashLabel->setStyleSheet(Style::mutedText() + " font-family: 'Menlo', 'Courier New'; font-size: 11px;");
    hashLabel->setFixedWidth(470);

    auto* txIcon = new QLabel();
    txIcon->setPixmap(QIcon(":/icons/file-text.svg").pixmap(18, 18));

    auto* txCount = new QLabel(QString::number(block.transactions.size()));

    auto* timeLabel = new QLabel(block.timestamp.toString("yyyy-MM-dd hh:mm:ss UTC"));
    timeLabel->setStyleSheet(Style::mutedText());

    statusLabel->setFixedWidth(90);
    statusLabel->setAlignment(Qt::AlignCenter);

    row->addWidget(idLabel);
    row->addWidget(hashLabel);
    row->addStretch(1);
    row->addWidget(txIcon);
    row->addWidget(txCount);
    row->addWidget(timeLabel);
    row->addWidget(statusLabel);

    quint64 blockId = block.blockId;
    connect(frame, &ClickableFrame::clicked, this, [this, blockId]() {
        emit blockClicked(blockId);
    });

    layout->addWidget(frame);
}

void MainPage::addTransactionRow(QVBoxLayout* layout, const Transaction& tx)
{
    auto* frame = new ClickableFrame();
    frame->setFrameShape(QFrame::NoFrame);
    frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

    auto* row = new QHBoxLayout(frame);
    row->setSpacing(10);

    auto* icon = new QLabel();
    icon->setPixmap(QIcon(":/icons/file-text.svg").pixmap(20, 20));
    row->addWidget(icon);

    auto* hashLabel = new QLabel(tx.hash);
    QFont boldFont = hashLabel->font();
    boldFont.setBold(true);
    hashLabel->setFont(boldFont);

    QString typeStr = transactionTypeToString(tx.type);
    auto* typeLabel = new QLabel(typeStr);
    typeLabel->setStyleSheet(Style::badge(Style::txTypeColor(typeStr)));

    auto* metaLabel = new QLabel();
    switch (tx.type) {
    case TransactionType::Public:
        metaLabel->setText(QString("%1 accounts").arg(tx.accounts.size()));
        break;
    case TransactionType::PrivacyPreserving:
        metaLabel->setText(QString("%1 accounts, %2 commitments").arg(tx.accounts.size()).arg(tx.newCommitmentsCount));
        break;
    case TransactionType::ProgramDeployment:
        metaLabel->setText(QString("%1 bytes").arg(tx.bytecodeSizeBytes));
        break;
    }
    metaLabel->setStyleSheet(Style::mutedText());

    row->addWidget(hashLabel);
    row->addWidget(typeLabel);
    row->addWidget(metaLabel, 1);

    QString txHash = tx.hash;
    connect(frame, &ClickableFrame::clicked, this, [this, txHash]() {
        emit transactionClicked(txHash);
    });

    layout->addWidget(frame);
}

void MainPage::addAccountRow(QVBoxLayout* layout, const Account& account)
{
    auto* frame = new ClickableFrame();
    frame->setFrameShape(QFrame::NoFrame);
    frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

    auto* row = new QHBoxLayout(frame);
    row->setSpacing(10);

    auto* icon = new QLabel();
    icon->setPixmap(QIcon(":/icons/user.svg").pixmap(20, 20));
    row->addWidget(icon);

    auto* idLabel = new QLabel(account.accountId);
    QFont boldFont = idLabel->font();
    boldFont.setBold(true);
    idLabel->setFont(boldFont);

    auto* balanceLabel = new QLabel(QString("Balance: %1").arg(account.balance));
    auto* nonceLabel = new QLabel(QString("Nonce: %1").arg(account.nonce));
    nonceLabel->setStyleSheet(Style::mutedText());

    row->addWidget(idLabel);
    row->addWidget(balanceLabel, 1);
    row->addWidget(nonceLabel);

    QString accId = account.accountId;
    connect(frame, &ClickableFrame::clicked, this, [this, accId]() {
        emit accountClicked(accId);
    });

    layout->addWidget(frame);
}

void MainPage::refresh()
{
    clearSearchResults();

    quint64 latestId = m_indexer->getLatestBlockId();
    m_healthLabel->setText(QString("Chain height: %1").arg(latestId));
    m_healthLabel->setStyleSheet(Style::healthLabel());

    if (m_recentBlocksWidget) {
        m_contentLayout->removeWidget(m_recentBlocksWidget);
        delete m_recentBlocksWidget;
        m_recentBlocksWidget = nullptr;
        m_blocksLayout = nullptr;
        m_loadMoreBtn = nullptr;
    }

    m_oldestLoadedBlockId = std::nullopt;

    m_recentBlocksWidget = new QWidget();
    auto* outerLayout = new QVBoxLayout(m_recentBlocksWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(createSectionHeader("Recent Blocks", ":/icons/box.svg"));

    // Dedicated layout for block rows — button is appended after this in outerLayout
    auto* blockRowsWidget = new QWidget();
    m_blocksLayout = new QVBoxLayout(blockRowsWidget);
    m_blocksLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(blockRowsWidget);

    auto blocks = m_indexer->getBlocks(std::nullopt, 10);
    for (const auto& block : blocks) {
        addBlockRow(m_blocksLayout, block);
        if (!m_oldestLoadedBlockId || block.blockId < *m_oldestLoadedBlockId)
            m_oldestLoadedBlockId = block.blockId;
    }

    m_loadMoreBtn = new QPushButton("Load more");
    m_loadMoreBtn->setStyleSheet(Style::searchButton());
    m_loadMoreBtn->setVisible(m_oldestLoadedBlockId && *m_oldestLoadedBlockId > 1);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, &MainPage::loadMoreBlocks);
    outerLayout->addWidget(m_loadMoreBtn, 0, Qt::AlignHCenter);

    m_contentLayout->addWidget(m_recentBlocksWidget);
}

void MainPage::loadMoreBlocks()
{
    if (!m_oldestLoadedBlockId || *m_oldestLoadedBlockId <= 1)
        return;

    auto blocks = m_indexer->getBlocks(m_oldestLoadedBlockId, 10);
    for (const auto& block : blocks) {
        addBlockRow(m_blocksLayout, block);
        if (block.blockId < *m_oldestLoadedBlockId)
            m_oldestLoadedBlockId = block.blockId;
    }

    m_loadMoreBtn->setVisible(*m_oldestLoadedBlockId > 1 && !blocks.isEmpty());
}

void MainPage::showSearchResults(const SearchResults& results)
{
    clearSearchResults();

    if (m_recentBlocksWidget) {
        m_recentBlocksWidget->hide();
    }

    m_searchResultsWidget = new QWidget();
    auto* layout = new QVBoxLayout(m_searchResultsWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    bool hasResults = false;

    if (!results.blocks.isEmpty()) {
        layout->addWidget(createSectionHeader("Blocks", ":/icons/box.svg"));
        for (const auto& block : results.blocks) {
            addBlockRow(layout, block);
        }
        hasResults = true;
    }

    if (!results.transactions.isEmpty()) {
        layout->addWidget(createSectionHeader("Transactions", ":/icons/file-text.svg"));
        for (const auto& tx : results.transactions) {
            addTransactionRow(layout, tx);
        }
        hasResults = true;
    }

    if (!results.accounts.isEmpty()) {
        layout->addWidget(createSectionHeader("Accounts", ":/icons/user.svg"));
        for (const auto& acc : results.accounts) {
            addAccountRow(layout, acc);
        }
        hasResults = true;
    }

    if (!hasResults) {
        auto* noResults = new QLabel("No results found.");
        noResults->setAlignment(Qt::AlignCenter);
        noResults->setStyleSheet(Style::mutedText() + " padding: 20px; font-size: 14px;");
        layout->addWidget(noResults);
    }

    m_contentLayout->insertWidget(0, m_searchResultsWidget);
}

void MainPage::onNewBlock(const Block& block)
{
    if (!m_blocksLayout) return;

    m_healthLabel->setText(QString("Chain height: %1").arg(block.blockId));

    // Append to layout then move to top (index 0)
    addBlockRow(m_blocksLayout, block);
    auto* newRow = m_blocksLayout->itemAt(m_blocksLayout->count() - 1)->widget();
    m_blocksLayout->removeWidget(newRow);
    m_blocksLayout->insertWidget(0, newRow);
}

void MainPage::clearSearchResults()
{
    if (m_searchResultsWidget) {
        m_contentLayout->removeWidget(m_searchResultsWidget);
        delete m_searchResultsWidget;
        m_searchResultsWidget = nullptr;
    }
    if (m_recentBlocksWidget) {
        m_recentBlocksWidget->show();
    }
}
