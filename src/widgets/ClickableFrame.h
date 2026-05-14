#pragma once

#include "Style.h"

#include <QFrame>
#include <QMouseEvent>

class ClickableFrame : public QFrame {
    Q_OBJECT

public:
    explicit ClickableFrame(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setCursor(Qt::PointingHandCursor);
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit clicked();
        }
        QFrame::mousePressEvent(event);
    }

    void enterEvent(QEnterEvent* event) override
    {
        m_baseStyleSheet = styleSheet();
        setStyleSheet(m_baseStyleSheet +
            QString(" ClickableFrame { background: %1 !important; border-color: %2 !important; }")
                .arg(Style::Color::surfaceHover(), Style::Color::accent()));
        QFrame::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        setStyleSheet(m_baseStyleSheet);
        QFrame::leaveEvent(event);
    }

private:
    QString m_baseStyleSheet;
};
