#include <QColor>
#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>

#include "Scale.h"

const char *Scale::m_colors[10]
{
    "#CC0000",
    "#CC0000",
    "#F2B705",
    "#F2B705",
    "#09A504",
    "#09A504",
    "#09A504",
    "#09A504",
    "#09A504",
    "white"
};

const char *Scale::m_disabledColors[10]
{
    "#4D0000",
    "#4D0000",
    "#332701",
    "#332701",
    "#033301",
    "#033301",
    "#033301",
    "#033301",
    "#033301",
    "lightgray"
};

Scale::Scale(QWidget *parent)
    : QWidget(parent)
    , m_signalLevel(0)
    , m_empty(true)
{
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(3);

    for (int i = 0; i < m_unitsCount; ++i) {
        auto widget = new QWidget(this);
        auto palette = widget->palette();
        widget->setPalette(palette);
        widget->setMinimumHeight(10);
        layout->addWidget(widget);
        m_widgets[i] = widget;
    }
    invalidate();
}

void Scale::setEmpty(bool value)
{
    if (m_empty == value) {
        return;
    }
    m_empty = value;
    invalidate();
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
    invalidate();
}

void Scale::invalidate()
{
    int lvl = m_empty ? 0 : m_signalLevel + 2;

    for (int i = 0; i < m_unitsCount; ++i) {
        auto color = m_unitsCount - lvl < i
                   ? m_colors[i]
                   : m_disabledColors[i];
        QString style = "QWidget {"
                        "   background-color: %1;"
                        "   border: 1px solid gray;"
                        "}";
        m_widgets[i]->setStyleSheet(style.arg(color));
    }
}
