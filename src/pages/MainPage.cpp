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
#include <QLineEdit>

#include <algorithm>

MainPage::MainPage(IndexerService* indexer, QWidget* parent)
    : QWidget(parent)
    , m_indexer(indexer)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // Indexer endpoint selector
    auto* endpointRow = new QHBoxLayout();
    endpointRow->setSpacing(8);

    auto* endpointLabel = new QLabel("Indexer config", this);
    endpointLabel->setStyleSheet(Style::mutedText() + " font-weight: bold;");

    m_indexerEndpointInput = new QLineEdit(this);
    m_indexerEndpointInput->setText(m_indexer->endpoint());
    m_indexerEndpointInput->setPlaceholderText("absolute path to indexer_config.json (blank = already running)");
    m_indexerEndpointInput->setMinimumHeight(34);
    m_indexerEndpointInput->setStyleSheet(Style::searchInput());

    auto* applyEndpointBtn = new QPushButton("Apply", this);
    applyEndpointBtn->setMinimumHeight(34);
    applyEndpointBtn->setStyleSheet(Style::searchButton());

    endpointRow->addWidget(endpointLabel);
    endpointRow->addWidget(m_indexerEndpointInput, 1);
    endpointRow->addWidget(applyEndpointBtn);
    outerLayout->addLayout(endpointRow);

    connect(applyEndpointBtn, &QPushButton::clicked, this, &MainPage::applyIndexerEndpoint);
    connect(m_indexerEndpointInput, &QLineEdit::returnPressed, this, &MainPage::applyIndexerEndpoint);

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

void MainPage::addBlockRow(QVBoxLayout* layout, const Block& block, int insertIndex)
{
    auto* frame = new ClickableFrame();
    frame->setFrameShape(QFrame::NoFrame);
    frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));
    frame->setProperty("blockId", QVariant::fromValue<qulonglong>(block.blockId));

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

    if (insertIndex >= 0) {
        layout->insertWidget(insertIndex, frame);
    } else {
        layout->addWidget(frame);
    }
}

void MainPage::insertRecentBlock(const Block& block)
{
    if (!m_blocksLayout || m_displayedBlockIds.contains(block.blockId)) {
        return;
    }

    int insertIndex = m_blocksLayout->count();
    for (int i = 0; i < m_blocksLayout->count(); ++i) {
        QLayoutItem* item = m_blocksLayout->itemAt(i);
        if (!item || !item->widget()) {
            continue;
        }

        bool ok = false;
        const quint64 existingId = item->widget()->property("blockId").toULongLong(&ok);
        if (ok && block.blockId > existingId) {
            insertIndex = i;
            break;
        }
    }

    addBlockRow(m_blocksLayout, block, insertIndex);
    m_displayedBlockIds.insert(block.blockId);

    if (!m_newestLoadedBlockId || block.blockId > *m_newestLoadedBlockId) {
        m_newestLoadedBlockId = block.blockId;
    }

    if (!m_oldestLoadedBlockId || block.blockId < *m_oldestLoadedBlockId) {
        m_oldestLoadedBlockId = block.blockId;
    }
}

QVector<Block> MainPage::fetchRecentBlocks(int limit)
{
    if (limit <= 0) {
        return {};
    }

    auto blocks = m_indexer->getBlocks(std::nullopt, limit);

    std::sort(blocks.begin(), blocks.end(), [](const Block& lhs, const Block& rhs) {
        return lhs.blockId > rhs.blockId;
    });

    if (blocks.size() > limit) {
        blocks.resize(limit);
    }

    return blocks;
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

    quint64 latestId = m_indexer->getLastFinalizedBlockId();
    m_latestKnownBlockId = latestId;
    m_healthLabel->setText(QString("Chain height: %1").arg(m_latestKnownBlockId));
    m_healthLabel->setStyleSheet(Style::healthLabel());

    if (m_recentBlocksWidget) {
        m_contentLayout->removeWidget(m_recentBlocksWidget);
        m_recentBlocksWidget->deleteLater();
        m_recentBlocksWidget = nullptr;
        m_blocksLayout = nullptr;
        m_loadMoreBtn = nullptr;
    }

    m_newestLoadedBlockId = std::nullopt;
    m_oldestLoadedBlockId = std::nullopt;
    m_displayedBlockIds.clear();

    m_recentBlocksWidget = new QWidget();
    auto* outerLayout = new QVBoxLayout(m_recentBlocksWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(createSectionHeader("Recent Blocks", ":/icons/box.svg"));

    // Dedicated layout for block rows — button is appended after this in outerLayout
    auto* blockRowsWidget = new QWidget();
    m_blocksLayout = new QVBoxLayout(blockRowsWidget);
    m_blocksLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(blockRowsWidget);

    auto blocks = fetchRecentBlocks(10);

    for (const auto& block : blocks) {
        insertRecentBlock(block);
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
    std::sort(blocks.begin(), blocks.end(), [](const Block& lhs, const Block& rhs) {
        return lhs.blockId > rhs.blockId;
    });

    for (const auto& block : blocks) {
        insertRecentBlock(block);
    }

    m_loadMoreBtn->setVisible(m_oldestLoadedBlockId && *m_oldestLoadedBlockId > 1 && !blocks.isEmpty());
}

void MainPage::applyIndexerEndpoint()
{
    // The "endpoint" is the indexer config path now; blank is valid and means
    // "the indexer is already running" (start is idempotent provider-side).
    const QString configPath = m_indexerEndpointInput->text().trimmed();
    m_indexer->setEndpoint(configPath);
    clearSearchResults();
    refresh();
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

    m_latestKnownBlockId = std::max(m_latestKnownBlockId, block.blockId);
    m_healthLabel->setText(QString("Chain height: %1").arg(m_latestKnownBlockId));

    // If notifications jump ahead, rebuild recent list from backend
    // so we do not mix one fresh head block with stale rows.
    if (!m_newestLoadedBlockId) {
        refresh();
        return;
    }

    if (block.blockId > *m_newestLoadedBlockId && (block.blockId - *m_newestLoadedBlockId) > 1) {
        refresh();
        return;
    }

    insertRecentBlock(block);

    if (m_loadMoreBtn) {
        m_loadMoreBtn->setVisible(m_oldestLoadedBlockId && *m_oldestLoadedBlockId > 1);
    }
}

void MainPage::clearSearchResults()
{
    if (m_searchResultsWidget) {
        m_contentLayout->removeWidget(m_searchResultsWidget);
        m_searchResultsWidget->deleteLater();
        m_searchResultsWidget = nullptr;
    }
    if (m_recentBlocksWidget) {
        m_recentBlocksWidget->show();
    }
}
