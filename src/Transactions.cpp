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
                port.clear();
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
    using namespace std::chrono_literals;

    Protocol proto(port);
    if (!proto.configure()) {
        return false;
    }

    struct Reader
    {
        bool isDataEnough(int size) const
        {
            constexpr int min = std::min<int>(sizeof(MDM500::DeviceInfo),
                                              sizeof(MDM500M::DeviceInfo));
            return size >= min;
        }

        bool checkPackage(Protocol::Command cmd, const void *data, int size)
        {
            if (cmd != Protocol::Command::ReadInfo) {
                return false;
            }
            if (size == sizeof(MDM500::DeviceInfo)) {
                type = DeviceType::MDM500;
                return true;
            }
            if (size == sizeof(MDM500M::DeviceInfo)) {
                type = DeviceType::MDM500M;
                auto &&info = *reinterpret_cast<const MDM500M::DeviceInfo *>(data);
                return info.hardwareVersion == MDM500M::kHardwareVersion;
            }
            return false;
        }

        DeviceType type;
    } reader;

    bool received = proto.get(Protocol::Command::ReadInfo, reader, 1s, Protocol::ReaderTag());
    if (received) {
        emit found(reader.type);
    }
    return received;
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
    bool result = reboot(proto)        // Перезагружаем устройство
            && boot.configure()        // Настраиваем порт на работу с загрузчиком
            && flash(boot)             // Передаем прошивку на устройство
            && proto.configure()       // Настраиваем порт на работу с прошивкой
            && waitForFirmware(proto)  // Ждем загрузки прошивки
            && reboot(proto)           // Перезагружаем прошивку
            && waitForFirmware(proto); // Ждем загрузки прошивки
    // Сообщаем о результате
    if (result) {
        emit success();
    } else {
        emit failure();
    }
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
            emit error(Flash);
            return false;
        }
        qDebug("UpdateFirmware::flash(): запрошена страница %d", (int) requestedPage);
        if (!boot.writePage(requestedPage, m_firmware.pageSizeLog2(),
                            m_firmware.page(requestedPage))) {
            emit error(Flash);
            return false;
        }
        qDebug("UpdateFirmware::flash(): страница %d передана", (int) requestedPage);
        emit progressChanged(requestedPage);
    }
    return true;
}

bool UpdateFirmware::waitForFirmware(Protocol &proto)
{
    emit statusChanged(WaitForBoot);
    emit progressMaxChanged(0);
    emit progressChanged(-1);

    DeviceInfo info;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (proto.get(Protocol::Command::ReadInfo, info)) {
            qDebug("UpdateFirmware::reboot(): устройство загружено");
            return true;
        }
        QThread::msleep(2500);
    }
    emit error(WaitForBoot);
    return false;
}

bool UpdateFirmware::reboot(Protocol &proto)
{
    emit statusChanged(Reboot);
    emit progressMaxChanged(0);
    emit progressChanged(-1);

    Protocol::Error err;
    if (proto.set(Protocol::Command::Reboot, err)) {
        Q_ASSERT(err == Protocol::Error::Ok);
        qDebug("UpdateFirmware::reboot(): устройство перезагружено");
        return true;
    }
    emit error(Reboot);
    return false;
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

namespace MDM500 {

GetAllDeviceInfo::GetAllDeviceInfo()
{
    qRegisterMetaType<Interfaces::GetAllDeviceInfo::Response>();
}

void GetAllDeviceInfo::exec(QSerialPort &port, CancelToken cancelled)
{
    Protocol proto(port);

    DeviceInfo info;
    DeviceConfig config;
    SignalLevels signalLevels;

    CHECK(proto.get(Protocol::Command::ReadInfo, info));
    CHECK(proto.get(Protocol::Command::ReadConfig, config));
    CHECK(proto.get(Protocol::Command::ReadSignalLevels, signalLevels));

    Response response;
    memset(&response, 0, sizeof(Response));
    response.info.serialNumber.value = info.serialNumber;
    response.config = config.convertToMDM500M();
    response.signalLevels = signalLevels;
    response.log = MDM500M::DeviceErrors {};
    emit success(std::move(response));
}

UpdateDeviceInfo::UpdateDeviceInfo()
{
    qRegisterMetaType<Interfaces::UpdateDeviceInfo::Response>();
}

void UpdateDeviceInfo::exec(QSerialPort &port, CancelToken cancelled)
{
    Protocol proto(port);
    Response response;
    memset(&response, 0, sizeof(Response));

    CHECK(proto.get(Protocol::Command::ReadSignalLevels, response.signalLevels));

    emit success(std::move(response));
}

SaveConfigToEprom::SaveConfigToEprom(const MDM500M::DeviceConfig &config)
    : m_config(DeviceConfig::fromMDM500M(config))
{
}

void SaveConfigToEprom::exec(QSerialPort &port, CancelToken cancelled)
{
    using namespace std::chrono_literals;

    struct Reader
    {
        bool isDataEnough(int size) const
        {
            return size >= 0;
        }

        bool checkPackage(Protocol::Command cmd, const void *, int size)
        {
            if (size != 0) {
                return false;
            }
            if (cmd == Protocol::Command::Ok) {
                deviceCorruptionDetected = false;
                return true;
            }
            if (cmd == Protocol::Command::Error) {
                deviceCorruptionDetected = true;
                return true;
            }
            return false;
        }

        bool deviceCorruptionDetected = false;
    } reader;

    Protocol proto(port);

    CHECK(proto.set(Protocol::Command::WriteConfig, m_config, reader, 2s, Protocol::ReaderTag()));

    if (reader.deviceCorruptionDetected) {
        emit deviceCorruptionDetected();
        return ;
    }
    emit success();
}

GetAllDeviceInfo *TransactionFabric::getAllDeviceInfo()
{
    return new GetAllDeviceInfo();
}

UpdateDeviceInfo *TransactionFabric::updateDeviceInfo()
{
    return new UpdateDeviceInfo();
}

SaveConfigToEprom *TransactionFabric::saveConfigToEprom(const MDM500M::DeviceConfig &config)
{
    return new SaveConfigToEprom(config);
}

Interfaces::SetControlModule *TransactionFabric::setControlModule(int)
{
    return nullptr;
}

Interfaces::SetModuleConfig *TransactionFabric::setModuleConfig(int, MDM500M::ModuleConfig)
{
    return nullptr;
}

Interfaces::SetThresholdLevels *TransactionFabric::setThresholdLevels(const SignalLevels &)
{
    return nullptr;
}

Interfaces::UpdateFirmware *TransactionFabric::updateFirmware(Firmware)
{
    return nullptr;
}

} // namespace MDM500
