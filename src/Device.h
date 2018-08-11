#pragma once

#include <array>
#include <memory>

#include <QObject>

#include "Types.h"

class Module;

struct DeviceData
{
    DeviceInfo   info {};
    DeviceConfig config {};
    SignalLevels signalLevels {};
    SignalLevels thresholdLevels {};
};

class ModuleFabric
{
public:
    enum ModuleTypeIndex
    {
        Empty = 0,
        DM500 = 1,
        DM500M = 2,
        DM500FM = 3,
        Unknown = 15,
    };

    virtual ~ModuleFabric() = default;
    virtual bool mustBeReplaced(Module *module, int typeIndex) const = 0;
    virtual Module *createModule(int typeIndex, int slot, DeviceData &data) = 0;
};

class Device : public QObject
{
    Q_OBJECT

public:
    Device(ModuleFabric *moduleFabric, QObject *parent = nullptr);
    ~Device();
    bool isError() const;
    Module *module(int slot) const;
    int moduleCount() const;
    const DeviceData &data() const;
    int controlModule() const;
    QString name() const;
    SerialNumber serialNumber() const;
    SoftwareVersion softwareVersion() const;
    void setName(QString name);
    void setControlModule(int slot);
    void setInfo(DeviceInfo info);
    void setConfig(const DeviceConfig &config);
    void setErrors(DeviceErrors errs);
    void setThresholdLevels(SignalLevels lvls);
    void setSignalLevels(SignalLevels lvls);
    void setModuleStates(ModuleStates states);
    void resetChangedError();

signals:
    void errorsChanged(bool);
    void infoChanged();
    void controlModuleChanged(int);
    void nameChanged(QString);
    void moduleReplaced(Module *);

private:
    void updateErrorStatus();

    DeviceData m_data;
    std::array<Module *, kMDM500MSlotCount> m_modules;
    QString m_name;
    std::unique_ptr<ModuleFabric> m_moduleFabric;
    bool m_isError = false;
};
