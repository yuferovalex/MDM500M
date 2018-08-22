#include <QColor>
#include <QBrush>
#include <QPainter>
#include <QPen>

#include "Scale.h"

const QBrush Scale::kUnitsColors[10]
{
    QBrush("#FF0066"),
    QBrush("#FF0000"),
    QBrush("#FFCC99"),
    QBrush("#FFFF00"),
    QBrush(Qt::green),
    QBrush("#99CC00"),
    QBrush("#99CC00"),
    QBrush("#99CC00"),
    QBrush("#B2CC6B"),
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
    static const QPen   border(Qt::gray, 1);
    static const QBrush inactive(Qt::lightGray);
    QPainter painter(this);
    painter.setPen(border);
    QRect unitRect(1, 1, width() - 2, 8);
    QPoint delta(0, 13);
    int invertedLvl = kUnitsCount - (m_signalLevel + !m_empty);
    for (int unit = 0; unit < kUnitsCount; ++unit) {
        painter.setBrush(invertedLvl <= unit ? kUnitsColors[unit] : inactive);
        painter.drawRect(unitRect);
        unitRect.translate(delta);
    }
}
