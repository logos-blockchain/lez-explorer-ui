#include "TransactionPage.h"
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

TransactionPage::TransactionPage(const Transaction& tx, QWidget* parent)
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
    auto* title = new QLabel("Transaction Details");
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Transaction info grid
    auto* infoFrame = new QFrame();
    infoFrame->setFrameShape(QFrame::StyledPanel);
    infoFrame->setStyleSheet(Style::cardFrameWithLabels());

    auto* grid = new QGridLayout(infoFrame);
    grid->setColumnStretch(1, 1);
    int row = 0;

    grid->addWidget(makeFieldLabel("Hash"), row, 0);
    auto* hashVal = makeValueLabel(tx.hash);
    hashVal->setStyleSheet(Style::monoText());
    grid->addWidget(hashVal, row++, 1);

    grid->addWidget(makeFieldLabel("Type"), row, 0);
    QString typeColor;
    switch (tx.type) {
    case TransactionType::Public: typeColor = "#007bff"; break;
    case TransactionType::PrivacyPreserving: typeColor = "#6f42c1"; break;
    case TransactionType::ProgramDeployment: typeColor = "#fd7e14"; break;
    }
    auto* typeLabel = new QLabel(transactionTypeToString(tx.type));
    typeLabel->setStyleSheet(Style::badge(typeColor) + " max-width: 160px;");
    grid->addWidget(typeLabel, row++, 1);

    // Type-specific fields
    switch (tx.type) {
    case TransactionType::Public:
        grid->addWidget(makeFieldLabel("Program ID"), row, 0);
        grid->addWidget(makeValueLabel(tx.programId), row++, 1);

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
        auto* accHeader = new QLabel("Accounts");
        QFont headerFont = accHeader->font();
        headerFont.setPointSize(16);
        headerFont.setBold(true);
        accHeader->setFont(headerFont);
        accHeader->setStyleSheet("margin-top: 16px; margin-bottom: 4px;");
        layout->addWidget(accHeader);

        for (const auto& accRef : tx.accounts) {
            auto* frame = new ClickableFrame();
            frame->setFrameShape(QFrame::StyledPanel);
            frame->setStyleSheet(Style::clickableRowWithLabels("ClickableFrame"));

            auto* accRow = new QHBoxLayout(frame);

            auto* idLabel = new QLabel(accRef.accountId.left(20) + "...");
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
