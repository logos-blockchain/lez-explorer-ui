#include "ExplorerWidget.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>

ExplorerWidget::ExplorerWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel("LEZ Explorer", this);
    label->setAlignment(Qt::AlignCenter);
    QFont font = label->font();
    font.setPointSize(24);
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet("color: white;");

    layout->addWidget(label);
}

void ExplorerWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a blue rectangle filling the widget
    painter.setBrush(QColor(30, 60, 120));
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());

    // Draw a lighter inner rectangle
    int margin = 40;
    QRect inner = rect().adjusted(margin, margin, -margin, -margin);
    painter.setBrush(QColor(40, 80, 160));
    painter.drawRoundedRect(inner, 16, 16);
}
