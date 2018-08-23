#pragma once

#include <array>
#include <memory>

#include <QObject>

#include "Types.h"

class Module;

struct DeviceData
{
    MDM500M::DeviceConfig config {};
    MDM500M::SignalLevels signalLevels {};
    MDM500M::SignalLevels thresholdLevels {};
    MDM500M::DeviceInfo   info {};
    DeviceType            type;
};

namespace Interfaces {

class ModuleFabric
{
public:
    virtual ~ModuleFabric() = default;
    virtual bool mustBeReplaced(Module *module, int typeIndex) const = 0;
    virtual Module *createModule(int typeIndex, int slot, DeviceData &data) = 0;
};

} // namespace Interfaces

class Device : public QObject
{
    Q_OBJECT

public:
    Device(std::shared_ptr<Interfaces::ModuleFabric> moduleFabric, DeviceType type);
    ~Device();
    bool isMDM500() const;
    QString type() const;
    bool isError() const;
    Module *module(int slot) const;
    int moduleCount() const;
    const DeviceData &data() const;
    int controlModule() const;
    QString name() const;
    QString serialNumber() const;
    SoftwareVersion softwareVersion() const;
    void setName(QString name);
    void setControlModule(int slot);
    void setInfo(const MDM500M::DeviceInfo &info);
    void setConfig(const MDM500M::DeviceConfig &config);
    void setErrors(MDM500M::DeviceErrors errs);
    void setThresholdLevels(const MDM500M::SignalLevels &lvls);
    void setSignalLevels(const MDM500M::SignalLevels &lvls);
    void setModuleStates(MDM500M::ModuleStates states);
    void resetChangedError();

signals:
    void errorsChanged(bool);
    void infoChanged();
    void controlModuleChanged(int);
    void nameChanged(QString);
    void moduleReplaced(Module *);

private:
    void updateErrorStatus();
    void checkLowLevels();

    DeviceData m_data;
    std::array<Module *, MDM500M::kSlotCount> m_modules;
    std::shared_ptr<Interfaces::ModuleFabric> m_moduleFabric;
    QString m_name;
    bool m_isError = false;
};
