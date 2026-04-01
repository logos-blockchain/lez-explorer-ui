#pragma once

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
};
