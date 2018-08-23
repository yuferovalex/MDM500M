#include <QColor>
#include <QBrush>
#include <QPainter>
#include <QPen>

#include "Scale.h"

const QBrush Scale::kUnitsColors[10]
{
    QBrush("#cc0000"),
    QBrush("#d93802"),
    QBrush("#e67403"),
    QBrush("#f2b705"),
    QBrush("#83cc04"),
    QBrush("#09a603"),
    QBrush("#09a603"),
    QBrush("#09a603"),
    QBrush("#83cc04"),
    QBrush(Qt::white)
};

Scale::Scale(QWidget *parent)
    : QWidget(parent)
    , m_signalLevel(0)
    , m_empty(true)
{
    setMinimumSize(QSize(30, 127));
}

void Scale::setEmpty(bool value)
{
    if (m_empty == value) {
        return;
    }
    m_empty = value;
    update();
}

void Scale::setSignalLevel(int value)
{
    if (m_empty) {
        value = 0;
    }
    if (m_signalLevel == value) {
        return;
    }
    m_signalLevel = value;
    update();
}

void Scale::paintEvent(QPaintEvent *)
{
    static const QPen   border(QColor("#333333"), 1);
    static const QBrush inactive("#D8D8D8");
    QPainter painter(this);
    painter.setPen(border);
    QRect unitRect(1, 1, width() - 2, 8);
    QPoint delta(0, 13);
    int invertedLvl = kUnitsCount - (m_signalLevel + !m_empty);
    for (int unit = 0; unit < kUnitsCount; ++unit) {
        painter.setBrush(invertedLvl <= unit ? kUnitsColors[unit] : inactive); // (kUnitsColors[unit]); //
        painter.drawRect(unitRect);
        unitRect.translate(delta);
    }
}
