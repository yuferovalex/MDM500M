#include <QFile>
#include <QDateTime>
#include <QMessageBox>

#include "Device.h"
#include "Modules.h"
#include "EventLog.h"

EventLog::EventLog(Device &device, QObject *parent)
    : QObject(parent)
    , m_device(device)
    , m_file(new QFile(this))
{
}

void EventLog::initialMessage(DeviceErrors log)
{
    open();
    out() << tr("Начало работы с устройством \"%1\"").arg(m_device.name()) << endl;
    out() << tr("Серийный номер:  %1").arg(m_device.serialNumber().toString()) << endl;
    out() << tr("Версия прошивки: %1").arg(m_device.softwareVersion().toString()) << endl;
    if (log) {
        out() << tr("Со времени последнего подключения произошли следующие ошибки:") << endl;
        reportOldErrors(log);
    }
    else {
        out() << tr("Со времени последнего подключения ошибок не обнаружено") << endl;
    }
    out() << tr("Текущее состояние модулей:") << endl;
    reportCurrentErrors();
    subscribe();
}

void EventLog::subscribe()
{
    for (int slot = 0; slot < m_device.moduleCount(); ++slot) {
        connect(m_device.module(slot), &Module::errorsChanged,
                this, &EventLog::onModuleErrorsChanged);
    }
}

void EventLog::reportOldErrors(DeviceErrors log)
{
    for (int slot = 0; slot < m_device.moduleCount(); ++slot) {
        auto e = log[slot];
        if (e.fault) {
            out() << tr("Модуль в слоте %1 (%2): не отвечал")
                     .arg(slot)
                     .arg(m_device.module(slot)->type())
                  << endl;
        }
        if (e.lowLevel) {
            out() << tr("Модуль в слоте %1 (%2): наблюдался низкий уровень сигнала")
                     .arg(slot)
                     .arg(m_device.module(slot)->type())
                  << endl;
        }
        if (e.patf) {
            out() << tr("Модуль в слоте %1 (%2): была обнаружена авария ФАПЧ")
                     .arg(slot)
                     .arg(m_device.module(slot)->type())
                  << endl;
        }
    }
}

void EventLog::reportCurrentErrors()
{
    for (int slot = 0; slot < m_device.moduleCount(); ++slot) {
        if (typeid (*m_device.module(slot)) == typeid (EmptyModule &)) {
            continue ;
        }
        out() << tr("Модуль в слоте %1 (%2): %3")
                 .arg(slot)
                 .arg(m_device.module(slot)->type())
                 .arg(m_device.module(slot)->error().toString())
              << endl;
    }
}

void EventLog::onModuleErrorsChanged()
{
    auto module = static_cast<Module *>(sender());
    out() << tr("Состояние модуля в слоте %1 (%2) изменилось: %3")
             .arg(module->slot())
             .arg(module->type())
             .arg(module->error().toString())
          << endl;
}

QTextStream &EventLog::out()
{
    return m_out << "[" << date() << "]: ";
}

QString EventLog::date() const
{
    return QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss");
}

void EventLog::open()
{
    if (!m_file->isOpen()) {
        m_file->setFileName(QString("%1_%2.log")
                            .arg(m_device.serialNumber().toString())
                            .arg(QDate::currentDate().toString("dd.MM.yyyy")));
        if (!m_file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            return;
        }
        m_out.setDevice(m_file);
    }
}
