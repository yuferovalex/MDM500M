#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>

#include "Device.h"
#include "Modules.h"
#include "EventLog.h"

struct CannotCreateLogsDir {};
struct CannotCdToLogsDir {};
struct CannotOpenFile
{
public:
    CannotOpenFile(QString desc) noexcept
        : m_what(desc)
    {}
    QString qWhat() const noexcept { return m_what; }

private:
    QString m_what;
};

EventLog::EventLog(Device &device, QObject *parent)
    : QObject(parent)
    , m_device(device)
    , m_file(new QFile(this))
{
}

void EventLog::initialMessage(MDM500M::DeviceErrors log)
{
    if (!open()) {
        return ;
    }
    out() << tr("Начало работы с устройством \"%1\"").arg(m_device.name()) << endl;
    out() << tr("Модель устройства:") << ' ' << m_device.type() << endl;
    out() << tr("Серийный номер:") << ' ' << m_device.serialNumber() << endl;
    // Старое устройство не поддерживает перепрошивку и журналирование ошибок
    if (!m_device.isMDM500()) {
        out() << tr("Версия прошивки:") << ' ' << m_device.softwareVersion().toString() << endl;
        if (log) {
            out() << tr("Со времени последнего подключения произошли следующие ошибки:") << endl;
            reportOldErrors(log);
        }
        else {
            out() << tr("Со времени последнего подключения ошибок не обнаружено") << endl;
        }
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

void EventLog::reportOldErrors(MDM500M::DeviceErrors log)
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

QDir EventLog::getLogPath() const
{
    QDir path { QFileInfo(QCoreApplication::applicationFilePath()).path() };
    if (!path.exists("logs")) {
        if (!path.mkdir("logs")) {
            throw CannotCreateLogsDir();
        }
    }
    if (!path.cd("logs")) {
        throw CannotCdToLogsDir();
    }
    return path;
}

QString EventLog::getFileName() const
{
    return QString("%1_%2.log")
            .arg(m_device.serialNumber())
            .arg(QDate::currentDate().toString("dd.MM.yyyy"));
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

bool EventLog::open()
{
    if (m_file->isOpen()) {
        return true;
    }
    static const auto errorMsgTemplate = tr("Во время создания файла журнала "
                                            "устройства произошла ошибка: %1.");

    QString caption;
    try {
        QString fileName = getLogPath().absoluteFilePath(getFileName());
        m_file->setFileName(fileName);
        if (!m_file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            throw CannotOpenFile(m_file->errorString());
        }
        m_out.setDevice(m_file);
        return true;
    }
    catch (CannotCreateLogsDir &) {
        caption = errorMsgTemplate
                .arg(tr("не удалось создать директорию \"logs\""));
    }
    catch (CannotCdToLogsDir &) {
        caption = errorMsgTemplate
                .arg(tr("не удалось открыть директорию \"logs\""));
    }
    catch (CannotOpenFile &e) {
        caption = errorMsgTemplate.arg(e.qWhat());
    }
    QMessageBox::warning(
                nullptr,
                tr("Ошибка создания файла журнала"),
                caption);
    return false;
}
