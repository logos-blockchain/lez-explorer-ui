#pragma once

#include "services/IndexerService.h"

#include <QWidget>
#include <QSet>
#include <optional>

class QVBoxLayout;
class QLabel;
class QPushButton;
class QLineEdit;

class MainPage : public QWidget {
    Q_OBJECT

public:
    explicit MainPage(IndexerService* indexer, QWidget* parent = nullptr);

    void refresh();
    void showSearchResults(const SearchResults& results);
    void clearSearchResults();
    void onNewBlock(const Block& block);

signals:
    void blockClicked(quint64 blockId);
    void transactionClicked(const QString& hash);
    void accountClicked(const QString& accountId);

private:
    void addBlockRow(QVBoxLayout* layout, const Block& block, int insertIndex = -1);
    void addTransactionRow(QVBoxLayout* layout, const Transaction& tx);
    void addAccountRow(QVBoxLayout* layout, const Account& account);
    QWidget* createSectionHeader(const QString& title, const QString& iconPath = {});
    QVector<Block> fetchRecentBlocks(int limit);
    void insertRecentBlock(const Block& block);
    void loadMoreBlocks();
    void applyIndexerEndpoint();

    IndexerService* m_indexer = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QWidget* m_searchResultsWidget = nullptr;
    QWidget* m_recentBlocksWidget = nullptr;
    QVBoxLayout* m_blocksLayout = nullptr;
    QPushButton* m_loadMoreBtn = nullptr;
    QLabel* m_healthLabel = nullptr;
    QLineEdit* m_indexerEndpointInput = nullptr;
    std::optional<quint64> m_newestLoadedBlockId;
    std::optional<quint64> m_oldestLoadedBlockId;
    QSet<quint64> m_displayedBlockIds;
    quint64 m_latestKnownBlockId = 0;
};
