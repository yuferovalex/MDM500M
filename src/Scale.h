#ifndef SCALE_H
#define SCALE_H

#include <QWidget>

class Scale : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(Scale)

public:
    explicit Scale(QWidget *parent = nullptr);

public slots:
    void setEmpty(bool value);
    void setSignalLevel(int value);

private:
    static const int m_unitsCount = 10;
    static const char *m_colors[];
    static const char *m_disabledColors[];
    void invalidate();

    int m_signalLevel = 0;
    bool m_empty = true;
    QWidget *m_widgets[m_unitsCount];
};

#endif // SCALE_H
