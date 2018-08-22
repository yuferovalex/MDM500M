#include <QSerialPortInfo>
#include <QThread>

#include "Transactions.h"
#include "Protocol.h"
#include "Firmware.h"
#include "UpdaterProtocol.h"

#define CHECK(received)        \
    do {                       \
        if (!(received)) {     \
            emit failure();    \
            return;            \
        }                      \
        if (cancelled) return; \
    } while(false)

SearchDevice::SearchDevice()
{
    qRegisterMetaType<SearchDevice::DeviceType>();
}

void SearchDevice::exec(QSerialPort &port, CancelToken cancelled)
{
    while (!cancelled) {
        for (auto &&info : QSerialPortInfo::availablePorts()) {
            if (cancelled) {
                return ;
            }
            port.setPort(info);
            if (port.open(QIODevice::ReadWrite)) {
                if (tryGetDeviceInfo(port)) {
                    return ;
                }
                port.close();
            }
        }
        QThread::sleep(1);
    }
}

bool SearchDevice::tryGetDeviceInfo(QSerialPort &port)
{
    Protocol proto(port);
    if (!proto.configure()) {
        return false;
    }
    MDM500M::DeviceInfo info;
    bool received = proto.get(Protocol::Command::ReadInfo, info);
    if (received && info.hardwareVersion == MDM500M::kHardwareVersion) {
        emit found(DeviceType::MDM500M);
        return true;
    }
    return false;
}

namespace MDM500M {

GetAllDeviceInfo::GetAllDeviceInfo()
{
    qRegisterMetaType<Interfaces::GetAllDeviceInfo::Response>();
}

void GetAllDeviceInfo::exec(QSerialPort &port, CancelToken cancelled)
{
    Response response;
    ErrorsPackage errors;
    Protocol proto(port);

    CHECK(proto.get(Protocol::Command::ReadInfo, response.info));
    CHECK(proto.get(Protocol::Command::ReadConfig, response.config));
    CHECK(proto.get(Protocol::Command::ReadErrors, errors));
    CHECK(proto.get(Protocol::Command::ReadThresholdLevels, response.thresholdLevels));
    CHECK(proto.get(Protocol::Command::ReadSignalLevels, response.signalLevels));
    if (errors.isResetRequired()) {
        Protocol::Error err;
        CHECK(proto.set(Protocol::Command::ResetErrors, err));
        Q_ASSERT(err == Protocol::Error::Ok);
    }
    response.log = errors.log;

    emit success(response);
}

UpdateDeviceInfo::UpdateDeviceInfo()
{
    qRegisterMetaType<Interfaces::UpdateDeviceInfo::Response>();
}

void UpdateDeviceInfo::exec(QSerialPort &port, CancelToken cancelled)
{
    Response response;
    ErrorsPackage errors;
    Protocol proto(port);

    CHECK(proto.get(Protocol::Command::ReadErrors, errors));
    CHECK(proto.get(Protocol::Command::ReadSignalLevels, response.signalLevels));
    CHECK(proto.get(Protocol::Command::ReadModuleStates, response.states));
    if (errors.isResetRequired()) {
        Protocol::Error err;
        CHECK(proto.set(Protocol::Command::ResetErrors, err));
        Q_ASSERT(err == Protocol::Error::Ok);
    }

    response.errors = errors.current;

    emit success(response);
}

SetControlModule::SetControlModule(int slot)
    : m_data(static_cast<uint8_t>(slot))
{
    Q_ASSERT(slot >= 0 && slot < kSlotCount);
}

void SetControlModule::exec(QSerialPort &port, CancelToken cancelled)
{
    Protocol proto(port);
    Protocol::Error err;

    CHECK(proto.set(Protocol::Command::WriteTempControlModule, err, m_data));

    Q_ASSERT(err == Protocol::Error::Ok);
    emit success();
}

SetModuleConfig::SetModuleConfig(int slot, ModuleConfig config)
    : m_data { static_cast<uint8_t>(slot), std::move(config) }
{
    Q_ASSERT(slot >= 0 && slot < kSlotCount);
}

void SetModuleConfig::exec(QSerialPort &port, CancelToken cancelled)
{
    Protocol proto(port);
    Protocol::Error err;

    CHECK(proto.set(Protocol::Command::WriteTempModuleConfig, err, m_data));

    switch (err) {
    case Protocol::Error::Ok             : return emit success();
    case Protocol::Error::BadParamNumber :
    case Protocol::Error::WrongParam     : return emit wrongParametersDetected();
    case Protocol::Error::CantWrite      : Q_ASSERT(false);
    }
}

SetThresholdLevels::SetThresholdLevels(SignalLevels lvls)
    : m_data(lvls)
{
}

void SetThresholdLevels::exec(QSerialPort &port, CancelToken cancelled)
{
    Protocol proto(port);
    Protocol::Error err;

    CHECK(proto.set(Protocol::Command::WriteThresholdLevels, err, m_data));
    Q_ASSERT(err == Protocol::Error::Ok);

    emit success();
}

SaveConfigToEprom::SaveConfigToEprom(const DeviceConfig &config)
    : m_config(config)
{
}

void SaveConfigToEprom::exec(QSerialPort &port, CancelToken cancelled)
{
    using namespace std::chrono_literals;
    Protocol proto(port);
    Protocol::Error err;

    CHECK(proto.set(Protocol::Command::WriteConfig, err, m_config, 2s));

    switch (err) {
    case Protocol::Error::Ok             : return emit success();
    case Protocol::Error::BadParamNumber : return Q_ASSERT(false);
    case Protocol::Error::WrongParam     : return emit wrongParametersDetected();
    case Protocol::Error::CantWrite      : return emit deviceCorruptionDetected();
    }
}

UpdateFirmware::UpdateFirmware(Firmware firmware)
    : m_firmware(firmware)
{
    qRegisterMetaType<Interfaces::UpdateFirmware::Status>();
}

void UpdateFirmware::exec(QSerialPort &port, CancelToken)
{
    Protocol proto(port);
    UpdaterProtocol boot(port);

    emit started();

    reboot(proto);

    boot.configure();
    if (!flash(boot)) {
        return emit failure();
    }

    proto.configure();
    waitForFirmware(proto);
    reboot(proto);
    waitForFirmware(proto);

    emit success();
}

bool UpdateFirmware::flash(UpdaterProtocol &boot)
{
    emit statusChanged(Flash);
    emit progressMaxChanged(m_firmware.pageCount() - 1);
    emit progressChanged(0);

    int requestedPage = 0;
    while (requestedPage < m_firmware.pageCount() - 1) {
        requestedPage = boot.waitForRequest();
        if (requestedPage == -1) {
            return false;
        }
        qDebug("UpdateFirmware::flash(): запрошена страница %d", (int) requestedPage);
        if (!boot.writePage(requestedPage, m_firmware.pageSizeLog2(),
                            m_firmware.page(requestedPage))) {
            return false;
        }
        qDebug("UpdateFirmware::flash(): страница %d передана", (int) requestedPage);
        emit progressChanged(requestedPage);
    }
    return true;
}

void UpdateFirmware::waitForFirmware(Protocol &proto)
{
    emit statusChanged(WaitForBoot);
    emit progressMaxChanged(0);
    emit progressChanged(-1);

    DeviceInfo info;
    do {
        QThread::msleep(2500);
    }
    while (!proto.get(Protocol::Command::ReadInfo, info));
    qDebug("UpdateFirmware::reboot(): устройство загружено");
}

void UpdateFirmware::reboot(Protocol &proto)
{
    emit statusChanged(Reboot);
    emit progressMaxChanged(0);
    emit progressChanged(-1);

    Protocol::Error err;
    proto.set(Protocol::Command::Reboot, err);
    Q_ASSERT(err == Protocol::Error::Ok);
    qDebug("UpdateFirmware::reboot(): устройство перезагружено");
}

GetAllDeviceInfo *TransactionFabric::getAllDeviceInfo()
{
    return new GetAllDeviceInfo();
}

UpdateDeviceInfo *TransactionFabric::updateDeviceInfo()
{
    return new UpdateDeviceInfo();
}

SetControlModule *TransactionFabric::setControlModule(int slot)
{
    return new SetControlModule(slot);
}

SetModuleConfig *TransactionFabric::setModuleConfig(int slot, ModuleConfig config)
{
    return new SetModuleConfig(slot, config);
}

SetThresholdLevels *TransactionFabric::setThresholdLevels(const SignalLevels &lvls)
{
    return new SetThresholdLevels(lvls);
}

SaveConfigToEprom *TransactionFabric::saveConfigToEprom(const DeviceConfig &config)
{
    return new SaveConfigToEprom(config);
}

UpdateFirmware *TransactionFabric::updateFirmware(Firmware firmware)
{
    return new UpdateFirmware(firmware);
}

} // namespace MDM500M

