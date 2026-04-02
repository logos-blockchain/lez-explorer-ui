#include "ExplorerWidget.h"
#include "Style.h"
#include "services/MockIndexerService.h"
#include "widgets/NavigationBar.h"
#include "widgets/SearchBar.h"
#include "pages/MainPage.h"
#include "pages/BlockPage.h"
#include "pages/TransactionPage.h"
#include "pages/AccountPage.h"

#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>

ExplorerWidget::ExplorerWidget(QWidget* parent)
    : QWidget(parent)
    , m_indexer(std::make_unique<MockIndexerService>())
{
    setStyleSheet(Style::appBackground());

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(8);

    // Navigation bar
    m_navBar = new NavigationBar(this);
    layout->addWidget(m_navBar);

    // Search bar
    m_searchBar = new SearchBar(this);
    layout->addWidget(m_searchBar);

    // Page stack
    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack, 1);

    // Create main page
    m_mainPage = new MainPage(m_indexer.get(), this);
    m_stack->addWidget(m_mainPage);
    connectPageSignals(m_mainPage);

    m_currentTarget = NavHome{};

    // Connect navigation
    connect(m_navBar, &NavigationBar::backClicked, this, &ExplorerWidget::navigateBack);
    connect(m_navBar, &NavigationBar::forwardClicked, this, &ExplorerWidget::navigateForward);
    connect(m_navBar, &NavigationBar::homeClicked, this, &ExplorerWidget::navigateHome);
    connect(m_searchBar, &SearchBar::searchRequested, this, &ExplorerWidget::onSearch);

    updateNavButtons();
}

ExplorerWidget::~ExplorerWidget() = default;

void ExplorerWidget::connectPageSignals(QWidget* page)
{
    if (auto* mainPage = qobject_cast<MainPage*>(page)) {
        connect(mainPage, &MainPage::blockClicked, this, [this](quint64 id) {
            navigateTo(NavBlock{id});
        });
        connect(mainPage, &MainPage::transactionClicked, this, [this](const QString& hash) {
            navigateTo(NavTransaction{hash});
        });
        connect(mainPage, &MainPage::accountClicked, this, [this](const QString& id) {
            navigateTo(NavAccount{id});
        });
    } else if (auto* blockPage = qobject_cast<BlockPage*>(page)) {
        connect(blockPage, &BlockPage::blockClicked, this, [this](quint64 id) {
            navigateTo(NavBlock{id});
        });
        connect(blockPage, &BlockPage::transactionClicked, this, [this](const QString& hash) {
            navigateTo(NavTransaction{hash});
        });
    } else if (auto* txPage = qobject_cast<TransactionPage*>(page)) {
        connect(txPage, &TransactionPage::accountClicked, this, [this](const QString& id) {
            navigateTo(NavAccount{id});
        });
    } else if (auto* accPage = qobject_cast<AccountPage*>(page)) {
        connect(accPage, &AccountPage::transactionClicked, this, [this](const QString& hash) {
            navigateTo(NavTransaction{hash});
        });
    }
}

void ExplorerWidget::showPage(QWidget* page)
{
    // Remove all pages except main page
    while (m_stack->count() > 1) {
        auto* w = m_stack->widget(1);
        m_stack->removeWidget(w);
        w->deleteLater();
    }

    if (page == m_mainPage) {
        m_stack->setCurrentWidget(m_mainPage);
    } else {
        m_stack->addWidget(page);
        m_stack->setCurrentWidget(page);
    }
}

void ExplorerWidget::navigateTo(const NavTarget& target, bool addToHistory)
{
    if (addToHistory) {
        m_backHistory.push(m_currentTarget);
        m_forwardHistory.clear();
    }

    m_currentTarget = target;

    std::visit([this](auto&& t) {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, NavHome>) {
            m_mainPage->clearSearchResults();
            m_mainPage->refresh();
            showPage(m_mainPage);
        } else if constexpr (std::is_same_v<T, NavBlock>) {
            auto block = m_indexer->getBlockById(t.blockId);
            if (block) {
                auto* page = new BlockPage(*block);
                connectPageSignals(page);
                showPage(page);
            }
        } else if constexpr (std::is_same_v<T, NavTransaction>) {
            auto tx = m_indexer->getTransaction(t.hash);
            if (tx) {
                auto* page = new TransactionPage(*tx);
                connectPageSignals(page);
                showPage(page);
            }
        } else if constexpr (std::is_same_v<T, NavAccount>) {
            auto account = m_indexer->getAccount(t.accountId);
            if (account) {
                auto* page = new AccountPage(*account, m_indexer.get());
                connectPageSignals(page);
                showPage(page);
            }
        }
    }, target);

    updateNavButtons();
}

void ExplorerWidget::navigateBack()
{
    if (m_backHistory.isEmpty()) return;

    m_forwardHistory.push(m_currentTarget);
    NavTarget target = m_backHistory.pop();
    navigateTo(target, false);
}

void ExplorerWidget::navigateForward()
{
    if (m_forwardHistory.isEmpty()) return;

    m_backHistory.push(m_currentTarget);
    NavTarget target = m_forwardHistory.pop();
    navigateTo(target, false);
}

void ExplorerWidget::navigateHome()
{
    navigateTo(NavHome{});
}

void ExplorerWidget::onSearch(const QString& query)
{
    if (query.trimmed().isEmpty()) return;

    auto results = m_indexer->search(query);

    // If exactly one result, navigate directly to it
    int totalResults = results.blocks.size() + results.transactions.size() + results.accounts.size();
    if (totalResults == 1) {
        if (!results.blocks.isEmpty()) {
            navigateTo(NavBlock{results.blocks.first().blockId});
            return;
        }
        if (!results.transactions.isEmpty()) {
            navigateTo(NavTransaction{results.transactions.first().hash});
            return;
        }
        if (!results.accounts.isEmpty()) {
            navigateTo(NavAccount{results.accounts.first().accountId});
            return;
        }
    }

    // Show results on main page
    navigateTo(NavHome{});
    m_mainPage->showSearchResults(results);
}

void ExplorerWidget::updateNavButtons()
{
    m_navBar->setBackEnabled(!m_backHistory.isEmpty());
    m_navBar->setForwardEnabled(!m_forwardHistory.isEmpty());
}
