#pragma once

#include <QSerialPort>

#include "Cancelation.h"
#include "Firmware.h"
#include "Types.h"

class Protocol;
class UpdaterProtocol;

namespace Interfaces {

class Transaction : public QObject
{
    Q_OBJECT

public:
    virtual void exec(QSerialPort &port, CancelToken cancelToken) = 0;

signals:
    void failure();
};

class GetAllDeviceInfo : public Transaction
{
    Q_OBJECT

public:
    struct Response
    {
        MDM500M::DeviceConfig config;
        MDM500M::DeviceErrors log;
        MDM500M::SignalLevels signalLevels;
        MDM500M::SignalLevels thresholdLevels;
        MDM500M::DeviceInfo   info;
    };

signals:
    void success(const Interfaces::GetAllDeviceInfo::Response &);
};

class UpdateDeviceInfo : public Transaction
{
    Q_OBJECT

public:
    struct Response
    {
        MDM500M::SignalLevels signalLevels;
        MDM500M::ModuleStates states;
        MDM500M::DeviceErrors errors;
    };

signals:
    void success(const Interfaces::UpdateDeviceInfo::Response &);
};

class SetControlModule : public Transaction
{
    Q_OBJECT

signals:
    void success();
};

class SetModuleConfig : public Transaction
{
    Q_OBJECT

signals:
    void success();
    void wrongParametersDetected();
};

class SetThresholdLevels : public Transaction
{
    Q_OBJECT

signals:
    void success();
};

class SaveConfigToEprom : public Transaction
{
    Q_OBJECT

signals:
    void success();
    void wrongParametersDetected();
    void deviceCorruptionDetected();
};

class UpdateFirmware : public Transaction
{
    Q_OBJECT

public:
    enum Status
    {
        Reboot,
        Flash,
        WaitForBoot
    };
    Q_ENUM(Status)

signals:
    void started();
    void success();
    void statusChanged(Interfaces::UpdateFirmware::Status status);
    void progressChanged(int progress);
    void progressMaxChanged(int progressMax);
};


class TransactionFabric
{
public:
    virtual ~TransactionFabric() = default;

    virtual GetAllDeviceInfo *getAllDeviceInfo() = 0;
    virtual UpdateDeviceInfo *updateDeviceInfo() = 0;
    virtual SetControlModule *setControlModule(int slot) = 0;
    virtual SetModuleConfig *setModuleConfig(int slot, MDM500M::ModuleConfig config) = 0;
    virtual SetThresholdLevels *setThresholdLevels(const MDM500M::SignalLevels &) = 0;
    virtual SaveConfigToEprom *saveConfigToEprom(const MDM500M::DeviceConfig &) = 0;
    virtual UpdateFirmware *updateFirmware(Firmware firmware) = 0;
};

} // namespace Interfaces
Q_DECLARE_METATYPE(Interfaces::GetAllDeviceInfo::Response)
Q_DECLARE_METATYPE(Interfaces::UpdateDeviceInfo::Response)

class SearchDevice : public Interfaces::Transaction
{
    Q_OBJECT

public:
    typedef ::DeviceType DeviceType;
    Q_ENUM(DeviceType)

    SearchDevice();
    void exec(QSerialPort &port, CancelToken iscancelled) override;

private:
    bool tryGetDeviceInfo(QSerialPort &port);

signals:
    void found(SearchDevice::DeviceType);
};

namespace MDM500M {

class GetAllDeviceInfo : public Interfaces::GetAllDeviceInfo
{
    Q_OBJECT

public:
    GetAllDeviceInfo();
    void exec(QSerialPort &port, CancelToken cancelled) override;
};

class UpdateDeviceInfo : public Interfaces::UpdateDeviceInfo
{
    Q_OBJECT

public:
    UpdateDeviceInfo();
    void exec(QSerialPort &port, CancelToken cancelled) override;
};

class SetControlModule : public Interfaces::SetControlModule
{
    Q_OBJECT

public:
    SetControlModule(int slot);
    void exec(QSerialPort &port, CancelToken cancelled) override;

private:
    uint8_t m_data;
};

class SetModuleConfig : public Interfaces::SetModuleConfig
{
    Q_OBJECT

public:
    SetModuleConfig(int slot, ModuleConfig config);
    void exec(QSerialPort &port, CancelToken cancelled) override;

private:
    ModuleConfigWithSlot m_data;
};

class SetThresholdLevels : public Interfaces::SetThresholdLevels
{
    Q_OBJECT

public:
    SetThresholdLevels(SignalLevels lvls);
    void exec(QSerialPort &port, CancelToken cancelled) override;

private:
    SignalLevels m_data;
};

class SaveConfigToEprom : public Interfaces::SaveConfigToEprom
{
    Q_OBJECT

public:
    SaveConfigToEprom(const DeviceConfig &config);
    void exec(QSerialPort &port, CancelToken cancelled) override;

private:
    DeviceConfig m_config;
};

class UpdateFirmware : public Interfaces::UpdateFirmware
{
    Q_OBJECT

public:
    UpdateFirmware(Firmware firmware);
    void exec(QSerialPort &port, CancelToken cancelled) override;

private:
    bool flash(UpdaterProtocol &boot);
    void waitForFirmware(Protocol &proto);
    void reboot(Protocol &proto);

    Firmware m_firmware;
};

class TransactionFabric : public Interfaces::TransactionFabric
{
public:
    GetAllDeviceInfo *getAllDeviceInfo() override;
    UpdateDeviceInfo *updateDeviceInfo() override;
    SetControlModule *setControlModule(int slot) override;
    SetModuleConfig *setModuleConfig(int slot, ModuleConfig config) override;
    SetThresholdLevels *setThresholdLevels(const SignalLevels &) override;
    SaveConfigToEprom *saveConfigToEprom(const DeviceConfig &) override;
    UpdateFirmware *updateFirmware(Firmware firmware) override;
};

} // namespace MDM500M

namespace MDM500 {

class GetAllDeviceInfo : public Interfaces::GetAllDeviceInfo
{
    Q_OBJECT

public:
    GetAllDeviceInfo();
    void exec(QSerialPort &port, CancelToken cancelled) override;
};

class UpdateDeviceInfo : public Interfaces::UpdateDeviceInfo
{
    Q_OBJECT

public:
    UpdateDeviceInfo();
    void exec(QSerialPort &port, CancelToken cancelled) override;
};

class SaveConfigToEprom : public Interfaces::SaveConfigToEprom
{
    Q_OBJECT

public:
    SaveConfigToEprom(const MDM500M::DeviceConfig &config);
    void exec(QSerialPort &port, CancelToken cancelled) override;

private:
    DeviceConfig m_config;
};

class TransactionFabric : public Interfaces::TransactionFabric
{
public:
    GetAllDeviceInfo *getAllDeviceInfo() override;
    UpdateDeviceInfo *updateDeviceInfo() override;
    SaveConfigToEprom *saveConfigToEprom(const MDM500M::DeviceConfig &) override;

    Interfaces::SetControlModule *setControlModule(int slot) override;
    Interfaces::SetModuleConfig *setModuleConfig(int slot, MDM500M::ModuleConfig config) override;
    Interfaces::SetThresholdLevels *setThresholdLevels(const SignalLevels &) override;
    Interfaces::UpdateFirmware *updateFirmware(Firmware firmware) override;
};

} // namespace MDM500
