#include "Device.h"
#include "Modules.h"

Device::Device(std::shared_ptr<Interfaces::ModuleFabric> moduleFabric, DeviceType type)
    : m_moduleFabric(moduleFabric)
{
    m_data.type = type;
    for (int slot = 0; slot < kSlotCount; ++slot) {
        m_modules[slot] = moduleFabric->createModule(ModuleInfo<EmptyModule>::typeIndex(),
                                                     slot,
                                                     m_data);
    }
}

Device::~Device()
{
    qDeleteAll(m_modules);
}

bool Device::isMDM500() const
{
    return m_data.type == DeviceType::MDM500;
}

QString Device::type() const
{
    return m_data.type == DeviceType::MDM500 ? tr("МДМ-500") : tr("МДМ-500М");
}

bool Device::isError() const
{
    return m_isError;
}

Module *Device::module(int slot) const
{
    Q_ASSERT(slot >= 0 && slot < moduleCount());
    return m_modules[slot];
}

int Device::moduleCount() const
{
    return kSlotCount;
}

const DeviceData &Device::data() const
{
    return m_data;
}

int Device::controlModule() const
{
    return m_data.config.control;
}

QString Device::name() const
{
    return m_name;
}

QString Device::serialNumber() const
{
    return m_data.info.serialNumber.toString(m_data.type);
}

SoftwareVersion Device::softwareVersion() const
{
    return m_data.info.softwareVersion;
}

void Device::setName(QString name)
{
    if (m_name == name) {
        return;
    }
    m_name = name;
    emit nameChanged(m_name);
}

void Device::setControlModule(int slot)
{
    Q_ASSERT(slot >= 0 && slot < moduleCount());
    if (m_data.config.control == slot) {
        return;
    }
    m_data.config.control = static_cast<uint8_t>(slot);
    emit controlModuleChanged(slot);
}

void Device::setInfo(const MDM500M::DeviceInfo &info)
{
    m_data.info = info;
    emit infoChanged();
}

void Device::setConfig(const MDM500M::DeviceConfig &config)
{
    m_data.config = config;
    for (int slot = 0; slot < moduleCount(); ++slot) {
        auto moduleConfig = config.modules[slot];
        auto &module = m_modules[slot];
        if (m_moduleFabric->mustBeReplaced(module, moduleConfig.type)) {
            delete module;
            module = m_moduleFabric->createModule(moduleConfig.type, slot, m_data);
        }
        emit moduleReplaced(module);
    }
    emit controlModuleChanged(m_data.config.control);
    updateErrorStatus();
}

void Device::setErrors(MDM500M::DeviceErrors errors)
{
    for (int slot = 0; slot < moduleCount(); ++slot) {
        bool isChanged = false;
        auto  e = errors[slot];
        auto &c = m_data.config.modules[slot];
        if (c.fault != e.fault) {
            c.fault = e.fault;
            isChanged = true;
        }
        if (c.lowLevel != e.lowLevel) {
            c.lowLevel = e.lowLevel;
            isChanged = true;
        }
        if (c.patf != e.patf) {
            c.patf = e.patf;
            isChanged = true;
        }
        if (isChanged) {
            emit m_modules[slot]->errorsChanged();
        }
    }
    updateErrorStatus();
}

void Device::setThresholdLevels(const MDM500M::SignalLevels &lvls)
{
    m_data.thresholdLevels = lvls;
    for (int slot = 0; slot < moduleCount(); ++slot) {
        auto &module = m_modules[slot];
        emit module->thresholdLevelChanged(module->thresholdLevel());
    }
}

void Device::setSignalLevels(const MDM500M::SignalLevels &lvls)
{
    for (int slot = 0; slot < moduleCount(); ++slot) {
        auto &module = m_modules[slot];
        if (m_data.signalLevels[slot] != lvls[slot]) {
            m_data.signalLevels[slot] = lvls[slot];
            emit module->signalLevelChanged(module->scaleLevel(), module->signalLevel());
        }
    }
    checkLowLevels();
}

void Device::setModuleStates(MDM500M::ModuleStates states)
{
    for (int slot = 0; slot < moduleCount(); ++slot) {
        m_modules[slot]->setModuleStates(states[slot]);
    }
}

void Device::resetChangedError()
{
    for (int slot = 0; slot < moduleCount(); ++slot) {
        auto &c = m_data.config.modules[slot];
        if (c.changed) {
            c.changed = false;
            emit m_modules[slot]->errorsChanged();
        }
    }
    updateErrorStatus();
}

void Device::updateErrorStatus()
{
    bool isErrors = false;
    for (auto &&module : m_modules) {
        if (module->isError()) {
            isErrors = true;
            break ;
        }
    }
    if (m_isError == isErrors) {
        return ;
    }
    m_isError = isErrors;
    emit errorsChanged(m_isError);
}

void Device::checkLowLevels()
{
    if (m_data.type == DeviceType::MDM500M) {
        return ;
    }
    for (int slot = 0; slot < kSlotCount; ++slot) {
        auto &config = m_data.config.modules[slot];
        bool lowLevel = m_data.signalLevels[slot] == 0;
        if (config.isModule && !config.blockDiagnostic && config.lowLevel != lowLevel) {
            config.lowLevel = lowLevel;
            emit module(slot)->errorsChanged();
        }
    }
    updateErrorStatus();
}
