#pragma once

#include "services/IndexerService.h"

#include <QWidget>
#include <QStack>
#include <memory>
#include <variant>

class QStackedWidget;
class NavigationBar;
class SearchBar;
class MainPage;
class LogosAPI;

struct NavHome {};
struct NavBlock { quint64 blockId; };
struct NavTransaction { QString hash; };
struct NavAccount { QString accountId; };

using NavTarget = std::variant<NavHome, NavBlock, NavTransaction, NavAccount>;

class ExplorerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ExplorerWidget(LogosAPI* logosAPI = nullptr, QWidget* parent = nullptr);
    ~ExplorerWidget() override;

private:
    void navigateTo(const NavTarget& target, bool addToHistory = true);
    void navigateBack();
    void navigateForward();
    void navigateHome();
    void onSearch(const QString& query);
    void updateNavButtons();

    void showPage(QWidget* page);
    void connectPageSignals(QWidget* page);

    std::unique_ptr<IndexerService> m_indexer;
    NavigationBar* m_navBar = nullptr;
    SearchBar* m_searchBar = nullptr;
    QStackedWidget* m_stack = nullptr;
    MainPage* m_mainPage = nullptr;

    QStack<NavTarget> m_backHistory;
    QStack<NavTarget> m_forwardHistory;
    NavTarget m_currentTarget;
};
