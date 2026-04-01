#include "BlockPage.h"
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

BlockPage::BlockPage(const Block& block, QWidget* parent)
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
    auto* title = new QLabel(QString("Block #%1").arg(block.blockId));
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Block info grid
    auto* infoFrame = new QFrame();
    infoFrame->setFrameShape(QFrame::StyledPanel);
    infoFrame->setStyleSheet(Style::cardFrameWithLabels());

    auto* grid = new QGridLayout(infoFrame);
    grid->setColumnStretch(1, 1);
    int row = 0;

    grid->addWidget(makeFieldLabel("Block ID"), row, 0);
    grid->addWidget(makeValueLabel(QString::number(block.blockId)), row++, 1);

    grid->addWidget(makeFieldLabel("Hash"), row, 0);
    grid->addWidget(makeValueLabel(block.hash), row++, 1);

    grid->addWidget(makeFieldLabel("Previous Hash"), row, 0);
    auto* prevHashLabel = new QLabel(QString("<a href='#' style='color: #007bff;'>%1</a>").arg(block.prevBlockHash));
    prevHashLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    if (block.blockId > 1) {
        quint64 prevBlockId = block.blockId - 1;
        connect(prevHashLabel, &QLabel::linkActivated, this, [this, prevBlockId]() {
            emit blockClicked(prevBlockId);
        });
    }
    grid->addWidget(prevHashLabel, row++, 1);

    grid->addWidget(makeFieldLabel("Timestamp"), row, 0);
    grid->addWidget(makeValueLabel(block.timestamp.toString("yyyy-MM-dd hh:mm:ss UTC")), row++, 1);

    grid->addWidget(makeFieldLabel("Status"), row, 0);
    QString statusColor;
    switch (block.bedrockStatus) {
    case BedrockStatus::Finalized: statusColor = "#28a745"; break;
    case BedrockStatus::Safe: statusColor = "#ffc107"; break;
    case BedrockStatus::Pending: statusColor = "#6c757d"; break;
    }
    auto* statusLabel = new QLabel(bedrockStatusToString(block.bedrockStatus));
    statusLabel->setStyleSheet(Style::badge(statusColor) + " max-width: 100px;");
    grid->addWidget(statusLabel, row++, 1);

    grid->addWidget(makeFieldLabel("Signature"), row, 0);
    auto* sigLabel = makeValueLabel(block.signature);
    sigLabel->setStyleSheet(Style::monoText());
    grid->addWidget(sigLabel, row++, 1);

    grid->addWidget(makeFieldLabel("Transactions"), row, 0);
    grid->addWidget(makeValueLabel(QString::number(block.transactions.size())), row++, 1);

    layout->addWidget(infoFrame);

    // Transactions section
    if (!block.transactions.isEmpty()) {
        auto* txHeader = new QLabel("Transactions");
        QFont headerFont = txHeader->font();
        headerFont.setPointSize(16);
        headerFont.setBold(true);
        txHeader->setFont(headerFont);
        txHeader->setStyleSheet("margin-top: 16px; margin-bottom: 4px;");
        layout->addWidget(txHeader);

        for (const auto& tx : block.transactions) {
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

            txRow->addWidget(hashLabel);
            txRow->addWidget(typeLabel);
            txRow->addWidget(metaLabel, 1);

            QString txHash = tx.hash;
            connect(frame, &ClickableFrame::clicked, this, [this, txHash]() {
                emit transactionClicked(txHash);
            });

            layout->addWidget(frame);
        }
    }

    scrollArea->setWidget(scrollContent);
    outerLayout->addWidget(scrollArea);
}
