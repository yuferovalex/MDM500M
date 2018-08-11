#include "Device.h"
#include "Modules.h"

Device::Device(ModuleFabric *moduleFabric, QObject *parent)
    : QObject(parent)
    , m_moduleFabric(moduleFabric)
{
    for (int slot = 0; slot < kMDM500MSlotCount; ++slot) {
        m_modules[slot] = moduleFabric->createModule(ModuleFabric::Empty, slot, m_data);
    }
}

Device::~Device()
{
    qDeleteAll(m_modules);
}

bool Device::isError() const
{
    return m_isError;
}

Module *Device::module(int slot) const
{
    Q_ASSERT(slot >= 0 && slot < kMDM500MSlotCount);
    return m_modules[slot];
}

int Device::moduleCount() const
{
    return kMDM500MSlotCount;
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

SerialNumber Device::serialNumber() const
{
    return m_data.info.serialNumber;
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
    Q_ASSERT(slot >= 0 && slot < kMDM500MSlotCount);
    if (m_data.config.control == slot) {
        return;
    }
    m_data.config.control = static_cast<uint8_t>(slot);
    emit controlModuleChanged(slot);
}

void Device::setInfo(DeviceInfo info)
{
    m_data.info = info;
    emit infoChanged();
}

void Device::setConfig(const DeviceConfig &config)
{
    m_data.config = config;
    for (int slot = 0; slot < kMDM500MSlotCount; ++slot) {
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

void Device::setErrors(DeviceErrors errors)
{
    for (int slot = 0; slot < kMDM500MSlotCount; ++slot) {
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

void Device::setThresholdLevels(SignalLevels lvls)
{
    m_data.thresholdLevels = lvls;
    for (int slot = 0; slot < kMDM500MSlotCount; ++slot) {
        auto &module = m_modules[slot];
        emit module->thresholdLevelChanged(module->thresholdLevel());
    }
}

void Device::setSignalLevels(SignalLevels lvls)
{

    for (int slot = 0; slot < kMDM500MSlotCount; ++slot) {
        auto &module = m_modules[slot];
        if (m_data.signalLevels[slot] != lvls[slot]) {
            m_data.signalLevels[slot] = lvls[slot];
            emit module->signalLevelChanged(module->scaleLevel(), module->signalLevel());
        }
    }
}

void Device::setModuleStates(ModuleStates states)
{
    for (int slot = 0; slot < kMDM500MSlotCount; ++slot) {
        m_modules[slot]->setModuleStates(states[slot]);
    }
}

void Device::resetChangedError()
{
    for (int slot = 0; slot < kMDM500MSlotCount; ++slot) {
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
