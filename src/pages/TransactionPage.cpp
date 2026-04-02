#include "TransactionPage.h"
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

TransactionPage::TransactionPage(const Transaction& tx, QWidget* parent)
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
    titleIcon->setPixmap(QIcon(":/icons/file-text.svg").pixmap(30, 30));
    auto* title = new QLabel("Transaction Details");
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

    // Transaction info grid
    auto* infoFrame = new QFrame();
    infoFrame->setFrameShape(QFrame::NoFrame);
    infoFrame->setStyleSheet(Style::cardFrameWithLabels());

    auto* grid = new QGridLayout(infoFrame);
    grid->setColumnStretch(1, 1);
    grid->setVerticalSpacing(10);
    grid->setHorizontalSpacing(20);
    int row = 0;

    grid->addWidget(makeFieldLabel("Hash"), row, 0);
    auto* hashVal = makeValueLabel(tx.hash);
    hashVal->setStyleSheet(Style::monoText());
    grid->addWidget(hashVal, row++, 1);

    grid->addWidget(makeFieldLabel("Type"), row, 0);
    QString typeStr = transactionTypeToString(tx.type);
    auto* typeLabel = new QLabel(typeStr);
    typeLabel->setStyleSheet(Style::badge(Style::txTypeColor(typeStr)));
    typeLabel->setMaximumWidth(180);
    grid->addWidget(typeLabel, row++, 1);

    // Type-specific fields
    switch (tx.type) {
    case TransactionType::Public:
        grid->addWidget(makeFieldLabel("Program ID"), row, 0);
        { auto* v = makeValueLabel(tx.programId); v->setStyleSheet(Style::monoText()); grid->addWidget(v, row++, 1); }

        grid->addWidget(makeFieldLabel("Instruction Data"), row, 0);
        grid->addWidget(makeValueLabel(QString("%1 items").arg(tx.instructionData.size())), row++, 1);

        grid->addWidget(makeFieldLabel("Proof Size"), row, 0);
        grid->addWidget(makeValueLabel(QString("%1 bytes").arg(tx.proofSizeBytes)), row++, 1);

        grid->addWidget(makeFieldLabel("Signatures"), row, 0);
        grid->addWidget(makeValueLabel(QString::number(tx.signatureCount)), row++, 1);
        break;

    case TransactionType::PrivacyPreserving:
        grid->addWidget(makeFieldLabel("Public Accounts"), row, 0);
        grid->addWidget(makeValueLabel(QString::number(tx.accounts.size())), row++, 1);

        grid->addWidget(makeFieldLabel("New Commitments"), row, 0);
        grid->addWidget(makeValueLabel(QString::number(tx.newCommitmentsCount)), row++, 1);

        grid->addWidget(makeFieldLabel("Nullifiers"), row, 0);
        grid->addWidget(makeValueLabel(QString::number(tx.nullifiersCount)), row++, 1);

        grid->addWidget(makeFieldLabel("Encrypted States"), row, 0);
        grid->addWidget(makeValueLabel(QString::number(tx.encryptedStatesCount)), row++, 1);

        grid->addWidget(makeFieldLabel("Proof Size"), row, 0);
        grid->addWidget(makeValueLabel(QString("%1 bytes").arg(tx.proofSizeBytes)), row++, 1);

        grid->addWidget(makeFieldLabel("Validity Window"), row, 0);
        grid->addWidget(makeValueLabel(QString("[%1, %2)").arg(tx.validityWindowStart).arg(tx.validityWindowEnd)), row++, 1);
        break;

    case TransactionType::ProgramDeployment:
        grid->addWidget(makeFieldLabel("Bytecode Size"), row, 0);
        grid->addWidget(makeValueLabel(QString("%1 bytes").arg(tx.bytecodeSizeBytes)), row++, 1);
        break;
    }

    layout->addWidget(infoFrame);

    // Accounts section
    if (!tx.accounts.isEmpty()) {
        auto* headerRow = new QHBoxLayout();
        headerRow->setContentsMargins(0, 16, 0, 6);
        headerRow->setSpacing(8);
        auto* headerIcon = new QLabel();
        headerIcon->setPixmap(QIcon(":/icons/user.svg").pixmap(24, 24));
        auto* accHeader = new QLabel("Accounts");
        QFont headerFont = accHeader->font();
        headerFont.setPointSize(20);
        headerFont.setBold(true);
        accHeader->setFont(headerFont);
        accHeader->setStyleSheet(Style::sectionHeader());
        headerRow->addWidget(headerIcon);
        headerRow->addWidget(accHeader);
        headerRow->addStretch();
        layout->addLayout(headerRow);

        for (const auto& accRef : tx.accounts) {
            auto* frame = new ClickableFrame();
            frame->setFrameShape(QFrame::NoFrame);
            frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

            auto* accRow = new QHBoxLayout(frame);
            accRow->setSpacing(10);

            auto* icon = new QLabel();
            icon->setPixmap(QIcon(":/icons/user.svg").pixmap(20, 20));
            accRow->addWidget(icon);

            auto* idLabel = new QLabel(accRef.accountId);
            QFont boldFont = idLabel->font();
            boldFont.setBold(true);
            idLabel->setFont(boldFont);

            auto* nonceLabel = new QLabel(QString("Nonce: %1").arg(accRef.nonce));
            nonceLabel->setStyleSheet(Style::mutedText());

            accRow->addWidget(idLabel, 1);
            accRow->addWidget(nonceLabel);

            QString accId = accRef.accountId;
            connect(frame, &ClickableFrame::clicked, this, [this, accId]() {
                emit accountClicked(accId);
            });

            layout->addWidget(frame);
        }
    }

    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);
}
