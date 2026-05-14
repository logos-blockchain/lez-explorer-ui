#include "BlockPage.h"
#include "Style.h"
#include "widgets/ClickableFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QEvent>
#include <QMouseEvent>

namespace {

class PrevHashClickFilter : public QObject {
public:
    PrevHashClickFilter(quint64 blockId, BlockPage* page, QObject* parent)
        : QObject(parent), m_blockId(blockId), m_page(page) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                emit m_page->blockClicked(m_blockId);
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    quint64 m_blockId;
    BlockPage* m_page;
};

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
    titleIcon->setPixmap(QIcon(":/icons/box.svg").pixmap(30, 30));
    auto* title = new QLabel(QString("Block #%1").arg(block.blockId));
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

    // Block info grid
    auto* infoFrame = new QFrame();
    infoFrame->setFrameShape(QFrame::NoFrame);
    infoFrame->setStyleSheet(Style::cardFrameWithLabels());

    auto* grid = new QGridLayout(infoFrame);
    grid->setColumnStretch(1, 1);
    grid->setVerticalSpacing(10);
    grid->setHorizontalSpacing(20);
    int row = 0;

    grid->addWidget(makeFieldLabel("Block ID"), row, 0);
    grid->addWidget(makeValueLabel(QString::number(block.blockId)), row++, 1);

    grid->addWidget(makeFieldLabel("Hash"), row, 0);
    auto* hashVal = makeValueLabel(block.hash);
    hashVal->setStyleSheet(Style::monoText());
    grid->addWidget(hashVal, row++, 1);

    grid->addWidget(makeFieldLabel("Previous Hash"), row, 0);
    if (block.blockId > 1) {
        auto* prevHashLabel = new QLabel(block.prevBlockHash);
        prevHashLabel->setStyleSheet(QString("color: %1; text-decoration: underline;").arg(Style::Color::accent()));
        prevHashLabel->setCursor(Qt::PointingHandCursor);
        prevHashLabel->setWordWrap(true);
        prevHashLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        quint64 prevBlockId = block.blockId - 1;
        prevHashLabel->installEventFilter(new PrevHashClickFilter(prevBlockId, this, this));
        grid->addWidget(prevHashLabel, row++, 1);
    } else {
        grid->addWidget(makeValueLabel(block.prevBlockHash), row++, 1);
    }

    grid->addWidget(makeFieldLabel("Timestamp"), row, 0);
    grid->addWidget(makeValueLabel(block.timestamp.toString("yyyy-MM-dd hh:mm:ss UTC")), row++, 1);

    grid->addWidget(makeFieldLabel("Status"), row, 0);
    QString statusStr = bedrockStatusToString(block.bedrockStatus);
    auto* statusLabel = new QLabel(statusStr);
    statusLabel->setStyleSheet(Style::badge(Style::statusColor(statusStr)));
    statusLabel->setMaximumWidth(120);
    grid->addWidget(statusLabel, row++, 1);

    grid->addWidget(makeFieldLabel("Signature"), row, 0);
    // Insert zero-width spaces every 32 chars so QLabel can word-wrap the long hex string
    QString wrappableSig = block.signature;
    for (int i = 32; i < wrappableSig.size(); i += 33) {
        wrappableSig.insert(i, QChar(0x200B));
    }
    auto* sigLabel = new QLabel(wrappableSig);
    sigLabel->setStyleSheet(Style::monoText());
    sigLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    sigLabel->setWordWrap(true);
    grid->addWidget(sigLabel, row++, 1);

    grid->addWidget(makeFieldLabel("Transactions"), row, 0);
    grid->addWidget(makeValueLabel(QString::number(block.transactions.size())), row++, 1);

    layout->addWidget(infoFrame);

    // Transactions section
    if (!block.transactions.isEmpty()) {
        auto* headerRow = new QHBoxLayout();
        headerRow->setContentsMargins(0, 16, 0, 6);
        headerRow->setSpacing(8);
        auto* headerIcon = new QLabel();
        headerIcon->setPixmap(QIcon(":/icons/file-text.svg").pixmap(24, 24));
        auto* txHeader = new QLabel("Transactions");
        QFont headerFont = txHeader->font();
        headerFont.setPointSize(20);
        headerFont.setBold(true);
        txHeader->setFont(headerFont);
        txHeader->setStyleSheet(Style::sectionHeader());
        headerRow->addWidget(headerIcon);
        headerRow->addWidget(txHeader);
        headerRow->addStretch();
        layout->addLayout(headerRow);

        for (const auto& tx : block.transactions) {
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
