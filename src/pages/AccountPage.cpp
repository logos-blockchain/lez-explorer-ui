#include "AccountPage.h"
#include "Style.h"
#include "widgets/ClickableFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>

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

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget();
    auto* layout = new QVBoxLayout(scrollContent);
    layout->setAlignment(Qt::AlignTop);

    // Title
    auto* title = new QLabel("Account Details");
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Account info grid
    auto* infoFrame = new QFrame();
    infoFrame->setFrameShape(QFrame::StyledPanel);
    infoFrame->setStyleSheet(Style::cardFrameWithLabels());

    auto* grid = new QGridLayout(infoFrame);
    grid->setColumnStretch(1, 1);
    int row = 0;

    grid->addWidget(makeFieldLabel("Account ID"), row, 0);
    auto* idVal = makeValueLabel(account.accountId);
    idVal->setStyleSheet(Style::monoText());
    grid->addWidget(idVal, row++, 1);

    grid->addWidget(makeFieldLabel("Balance"), row, 0);
    grid->addWidget(makeValueLabel(account.balance), row++, 1);

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
        auto* txHeader = new QLabel("Transaction History");
        QFont headerFont = txHeader->font();
        headerFont.setPointSize(16);
        headerFont.setBold(true);
        txHeader->setFont(headerFont);
        txHeader->setStyleSheet("margin-top: 16px; margin-bottom: 4px;");
        layout->addWidget(txHeader);

        for (const auto& tx : transactions) {
            auto* frame = new ClickableFrame();
            frame->setFrameShape(QFrame::StyledPanel);
            frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

            auto* txRow = new QHBoxLayout(frame);

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
