#pragma once
#include <QSlider>
#include <QMouseEvent>

class ClickableSlider : public QSlider {
    Q_OBJECT
public:
    explicit ClickableSlider(Qt::Orientation o, QWidget *parent = nullptr)
        : QSlider(o, parent) {}

protected:
    void mousePressEvent(QMouseEvent *e) override {
        // Calculate value at click position and jump directly
        if (e->button() == Qt::LeftButton) {
            int val;
            if (orientation() == Qt::Horizontal) {
                double ratio = static_cast<double>(e->pos().x()) / width();
                val = minimum() + static_cast<int>(ratio * (maximum() - minimum()));
            } else {
                double ratio = 1.0 - static_cast<double>(e->pos().y()) / height();
                val = minimum() + static_cast<int>(ratio * (maximum() - minimum()));
            }
            setValue(val);
            emit sliderMoved(val);
            e->accept();
            return;
        }
        QSlider::mousePressEvent(e);
    }
};
