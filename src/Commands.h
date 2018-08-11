#pragma once

#include <QSerialPort>

#include "Cancelation.h"
#include "Types.h"

class Command : public QObject
{
    Q_OBJECT

public:
    virtual void exec(QSerialPort &port, CancelToken cancelToken) = 0;
};

class SearchDevice : public Command
{
    Q_OBJECT

public:
    void exec(QSerialPort &port, CancelToken isCanceled) override;

private:
    bool isFTDI(const QSerialPortInfo &info) const;
    bool check(QSerialPort &port);

signals:
    void found();
};

class GetAllDeviceInfo : public Command
{
    Q_OBJECT

public:
    struct Response
    {
        DeviceConfig config;
        DeviceErrors log;
        SignalLevels signalLevels;
        SignalLevels thresholdLevels;
        DeviceInfo   info;
    };

    GetAllDeviceInfo();
    void exec(QSerialPort &port, CancelToken canceled) override;

signals:
    void success(const GetAllDeviceInfo::Response &);
    void failure();
};
Q_DECLARE_METATYPE(GetAllDeviceInfo::Response)

class UpdateDeviceInfo : public Command
{
    Q_OBJECT

public:
    struct Response
    {
        SignalLevels signalLevels;
        ModuleStates states;
        DeviceErrors errors;
    };

    UpdateDeviceInfo();
    void exec(QSerialPort &port, CancelToken canceled) override;

signals:
    void success(const UpdateDeviceInfo::Response &);
    void failure();
};
Q_DECLARE_METATYPE(UpdateDeviceInfo::Response)

class SetControlModule : public Command
{
    Q_OBJECT

public:
    SetControlModule(int slot);
    void exec(QSerialPort &port, CancelToken canceled) override;

signals:
    void success();
    void failure();

private:
    uint8_t m_data;
};

class SetModuleConfig : public Command
{
    Q_OBJECT

public:
    SetModuleConfig(int slot, ModuleConfig config);
    void exec(QSerialPort &port, CancelToken canceled) override;

signals:
    void success();
    void wrongParametersDetected();
    void failure();

private:
    ModuleConfigWithSlot m_data;
};

class SetThresholdLevels : public Command
{
    Q_OBJECT

public:
    SetThresholdLevels(SignalLevels lvls);
    void exec(QSerialPort &port, CancelToken canceled) override;

signals:
    void success();
    void failure();

private:
    SignalLevels m_data;
};

class SaveConfigToEprom : public Command
{
    Q_OBJECT

public:
    SaveConfigToEprom(const DeviceConfig &config);
    void exec(QSerialPort &port, CancelToken canceled) override;

signals:
    void success();
    void wrongParametersDetected();
    void deviceCorruptionDetected();
    void failure();

private:
    DeviceConfig m_config;
};
