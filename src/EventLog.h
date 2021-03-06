#pragma once

#include <QDir>
#include <QObject>
#include <QTextStream>

#include "Types.h"

class Device;
class Module;
class QFile;

class EventLog : public QObject
{
    Q_OBJECT

public:
    EventLog(Device &device, QObject *parent = nullptr);
    void initialMessage(MDM500M::DeviceErrors log);

private slots:
    void onModuleErrorsChanged();

private:
    QTextStream &out();
    QString date() const;

    bool open();
    void subscribe();
    void reportOldErrors(MDM500M::DeviceErrors log);
    void reportCurrentErrors();
    QDir getLogPath() const;
    QString getFileName() const;

    QTextStream m_out;
    Device &m_device;
    QFile *m_file;
};
