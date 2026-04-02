#include "AccountPage.h"
#include "Style.h"
#include "widgets/ClickableFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>

namespace {

QLabel* makeFieldLabel(const QString& text)
{
    auto* label = new QLabel(text);
    label->setStyleSheet(Style::mutedText() + " font-weight: bold;");
    return label;
}

QLabel* makeValueLabel(const QString& text)
{
    auto* label = new QLabel(text);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);
    return label;
}

} // namespace

AccountPage::AccountPage(const Account& account, IndexerService* indexer, QWidget* parent)
    : QWidget(parent)
{
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; } QWidget#scrollContent { background: transparent; }");

    auto* scrollContent = new QWidget();
    scrollContent->setObjectName("scrollContent");
    auto* layout = new QVBoxLayout(scrollContent);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);

    // Title with icon
    auto* titleRow = new QHBoxLayout();
    titleRow->setSpacing(10);
    auto* titleIcon = new QLabel();
    titleIcon->setPixmap(QIcon(":/icons/user.svg").pixmap(30, 30));
    auto* title = new QLabel("Account Details");
    QFont titleFont = title->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setStyleSheet(QString("color: %1;").arg(Style::Color::text()));
    titleRow->addWidget(titleIcon);
    titleRow->addWidget(title);
    titleRow->addStretch();
    layout->addLayout(titleRow);
    layout->addSpacing(8);

    // Account info grid
    auto* infoFrame = new QFrame();
    infoFrame->setFrameShape(QFrame::NoFrame);
    infoFrame->setStyleSheet(Style::cardFrameWithLabels());

    auto* grid = new QGridLayout(infoFrame);
    grid->setColumnStretch(1, 1);
    grid->setVerticalSpacing(10);
    grid->setHorizontalSpacing(20);
    int row = 0;

    grid->addWidget(makeFieldLabel("Account ID"), row, 0);
    auto* idVal = makeValueLabel(account.accountId);
    idVal->setStyleSheet(Style::monoText());
    grid->addWidget(idVal, row++, 1);

    grid->addWidget(makeFieldLabel("Balance"), row, 0);
    auto* balVal = makeValueLabel(account.balance);
    balVal->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Style::Color::green()));
    grid->addWidget(balVal, row++, 1);

    grid->addWidget(makeFieldLabel("Program Owner"), row, 0);
    auto* ownerVal = makeValueLabel(account.programOwner);
    ownerVal->setStyleSheet(Style::monoText());
    grid->addWidget(ownerVal, row++, 1);

    grid->addWidget(makeFieldLabel("Nonce"), row, 0);
    grid->addWidget(makeValueLabel(account.nonce), row++, 1);

    grid->addWidget(makeFieldLabel("Data Size"), row, 0);
    grid->addWidget(makeValueLabel(QString("%1 bytes").arg(account.dataSizeBytes)), row++, 1);

    layout->addWidget(infoFrame);

    // Transaction history
    auto transactions = indexer->getTransactionsByAccount(account.accountId, 0, 10);

    if (!transactions.isEmpty()) {
        auto* headerRow = new QHBoxLayout();
        headerRow->setContentsMargins(0, 16, 0, 6);
        headerRow->setSpacing(8);
        auto* headerIcon = new QLabel();
        headerIcon->setPixmap(QIcon(":/icons/file-text.svg").pixmap(24, 24));
        auto* txHeader = new QLabel("Transaction History");
        QFont headerFont = txHeader->font();
        headerFont.setPointSize(20);
        headerFont.setBold(true);
        txHeader->setFont(headerFont);
        txHeader->setStyleSheet(Style::sectionHeader());
        headerRow->addWidget(headerIcon);
        headerRow->addWidget(txHeader);
        headerRow->addStretch();
        layout->addLayout(headerRow);

        for (const auto& tx : transactions) {
            auto* frame = new ClickableFrame();
            frame->setFrameShape(QFrame::NoFrame);
            frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

            auto* txRow = new QHBoxLayout(frame);
            txRow->setSpacing(10);

            auto* icon = new QLabel();
            icon->setPixmap(QIcon(":/icons/file-text.svg").pixmap(20, 20));
            txRow->addWidget(icon);

            auto* hashLabel = new QLabel(tx.hash);
            QFont boldFont = hashLabel->font();
            boldFont.setBold(true);
            hashLabel->setFont(boldFont);

            QString typeStr = transactionTypeToString(tx.type);
            auto* typeLabel = new QLabel(typeStr);
            typeLabel->setStyleSheet(Style::badge(Style::txTypeColor(typeStr)));

            txRow->addWidget(hashLabel);
            txRow->addWidget(typeLabel);
            txRow->addStretch();

            QString txHash = tx.hash;
            connect(frame, &ClickableFrame::clicked, this, [this, txHash]() {
                emit transactionClicked(txHash);
            });

            layout->addWidget(frame);
        }
    } else {
        auto* noTx = new QLabel("No transactions found for this account.");
        noTx->setStyleSheet(Style::mutedText() + " margin-top: 16px;");
        layout->addWidget(noTx);
    }

    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);
}
