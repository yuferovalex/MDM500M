#pragma once

#include <QWidget>

class Scale : public QWidget
{
    Q_OBJECT

public:
    Scale(QWidget *parent = nullptr);

public slots:
    void setEmpty(bool value);
    void setSignalLevel(int value);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    static const int kUnitsCount = 10;
    static const QBrush kUnitsColors[];

    int m_signalLevel = 0;
    bool m_empty = true;
};
