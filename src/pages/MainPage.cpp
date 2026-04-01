#include "MainPage.h"
#include "Style.h"
#include "widgets/ClickableFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>

MainPage::MainPage(IndexerService* indexer, QWidget* parent)
    : QWidget(parent)
    , m_indexer(indexer)
{
    auto* outerLayout = new QVBoxLayout(this);

    // Health indicator
    auto* healthRow = new QHBoxLayout();
    m_healthLabel = new QLabel(this);
    healthRow->addWidget(m_healthLabel);
    healthRow->addStretch();
    outerLayout->addLayout(healthRow);

    // Scroll area for content
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget();
    m_contentLayout = new QVBoxLayout(scrollContent);
    m_contentLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);

    refresh();
}

QWidget* MainPage::createSectionHeader(const QString& title)
{
    auto* label = new QLabel(title);
    QFont font = label->font();
    font.setPointSize(16);
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet("margin-top: 12px; margin-bottom: 4px;");
    return label;
}

void MainPage::addBlockRow(QVBoxLayout* layout, const Block& block)
{
    auto* frame = new ClickableFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

    auto* row = new QHBoxLayout(frame);

    auto* idLabel = new QLabel(QString("Block #%1").arg(block.blockId));
    QFont boldFont = idLabel->font();
    boldFont.setBold(true);
    idLabel->setFont(boldFont);

    QString statusColor;
    switch (block.bedrockStatus) {
    case BedrockStatus::Finalized: statusColor = "#28a745"; break;
    case BedrockStatus::Safe: statusColor = "#ffc107"; break;
    case BedrockStatus::Pending: statusColor = "#6c757d"; break;
    }
    auto* statusLabel = new QLabel(bedrockStatusToString(block.bedrockStatus));
    statusLabel->setStyleSheet(Style::badge(statusColor));

    auto* hashLabel = new QLabel(block.hash.left(16) + "...");
    hashLabel->setStyleSheet(Style::mutedText());

    auto* txCount = new QLabel(QString("%1 tx").arg(block.transactions.size()));

    auto* timeLabel = new QLabel(block.timestamp.toString("yyyy-MM-dd hh:mm:ss UTC"));
    timeLabel->setStyleSheet(Style::mutedText());

    row->addWidget(idLabel);
    row->addWidget(statusLabel);
    row->addWidget(hashLabel, 1);
    row->addWidget(txCount);
    row->addWidget(timeLabel);

    quint64 blockId = block.blockId;
    connect(frame, &ClickableFrame::clicked, this, [this, blockId]() {
        emit blockClicked(blockId);
    });

    layout->addWidget(frame);
}

void MainPage::addTransactionRow(QVBoxLayout* layout, const Transaction& tx)
{
    auto* frame = new ClickableFrame();
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

    auto* row = new QHBoxLayout(frame);

    auto* hashLabel = new QLabel(tx.hash.left(16) + "...");
    QFont boldFont = hashLabel->font();
    boldFont.setBold(true);
    hashLabel->setFont(boldFont);

    QString typeColor;
    switch (tx.type) {
    case TransactionType::Public: typeColor = "#007bff"; break;
    case TransactionType::PrivacyPreserving: typeColor = "#6f42c1"; break;
    case TransactionType::ProgramDeployment: typeColor = "#fd7e14"; break;
    }
    auto* typeLabel = new QLabel(transactionTypeToString(tx.type));
    typeLabel->setStyleSheet(Style::badge(typeColor));

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
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

    auto* row = new QHBoxLayout(frame);

    auto* idLabel = new QLabel(account.accountId.left(16) + "...");
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
    m_healthLabel->setStyleSheet("color: #28a745; font-weight: bold; font-size: 14px;");

    if (m_recentBlocksWidget) {
        m_contentLayout->removeWidget(m_recentBlocksWidget);
        delete m_recentBlocksWidget;
    }

    m_recentBlocksWidget = new QWidget();
    auto* blocksLayout = new QVBoxLayout(m_recentBlocksWidget);
    blocksLayout->setContentsMargins(0, 0, 0, 0);

    blocksLayout->addWidget(createSectionHeader("Recent Blocks"));

    auto blocks = m_indexer->getBlocks(std::nullopt, 10);
    for (const auto& block : blocks) {
        addBlockRow(blocksLayout, block);
    }

    m_contentLayout->addWidget(m_recentBlocksWidget);
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
        layout->addWidget(createSectionHeader("Blocks"));
        for (const auto& block : results.blocks) {
            addBlockRow(layout, block);
        }
        hasResults = true;
    }

    if (!results.transactions.isEmpty()) {
        layout->addWidget(createSectionHeader("Transactions"));
        for (const auto& tx : results.transactions) {
            addTransactionRow(layout, tx);
        }
        hasResults = true;
    }

    if (!results.accounts.isEmpty()) {
        layout->addWidget(createSectionHeader("Accounts"));
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
