#include "ChannelTable.h"
#include "Modules.h"

Module::Module(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable)
    : m_data(data)
    , m_channel(chTable->channel(KiloHertz(m_data.config.modules[slot].frequency)))
    , m_chTable(chTable)
    , m_slot(slot)
{
}

bool Module::isEmpty() const
{
    return m_isEmpty;
}

QString Module::type() const
{
    return m_type;
}

int Module::slot() const
{
    return m_slot;
}

QString Module::channel() const
{
    return m_channel;
}

QStringList Module::allChannels() const
{
    return m_chTable->allChannels();
}

void Module::setChannel(QString channel)
{
    if (m_channel == channel) {
        return ;
    }
    auto f = m_chTable->frequency(channel);
    m_data.config.modules[m_slot].frequency = static_cast<uint32_t>(f.count());
    m_channel = channel;
    emit frequencyChanged();
    emit channelChanged(channel);
}

KiloHertz Module::frequency() const
{
    return KiloHertz(m_data.config.modules[m_slot].frequency);
}

bool Module::setFrequency(KiloHertz f)
{
    if (!validateFrequency(f)) {
        return false;
    }
    if (frequency() == f) {
        return true;
    }
    m_data.config.modules[m_slot].frequency = static_cast<uint32_t>(f.count());
    auto channel = m_chTable->channel(f);
    if (m_channel != channel) {
        m_channel = channel;
        emit channelChanged(m_channel);
    }
    emit frequencyChanged();
    return true;
}

bool Module::isDiagnosticEnabled() const
{
    return !m_data.config.modules[m_slot].blockDiagnostic;
}

void Module::setDiagnostic(bool on)
{
    if (isDiagnosticEnabled() == on) {
        return ;
    }
    m_data.config.modules[m_slot].blockDiagnostic = !on;
    emit diagnosticChanged(on);
}

bool Module::isError() const
{
    return error().isError();
}

ModuleError Module::error() const
{
    ModuleError retval;
    auto config = m_data.config.modules[m_slot];

    retval.setFlag(ModuleError::Error::Changed,        config.changed );
    retval.setFlag(ModuleError::Error::LowSignalLevel, config.lowLevel);
    retval.setFlag(ModuleError::Error::PatfFault,      config.patf    );
    retval.setFlag(ModuleError::Error::SlotFault,      config.i2c     );
    retval.setFlag(ModuleError::Error::NotResponding,  config.fault   );

    return retval;
}

ScaleLevel Module::scaleLevel() const
{
    return ScaleLevel(scaleLevelImpl(m_data.signalLevels[m_slot]));
}

bool Module::isSupportSignalLevel() const
{
    return m_isSupportSignalLevel;
}

int Module::signalLevel() const
{
    return signalLevelImpl(m_data.signalLevels[m_slot]);
}

int Module::thresholdLevel() const
{
    return thresholdLevelsImpl(m_data.thresholdLevels[m_slot]);
}

void Module::setThresholdLevel(int lvl)
{
    qDebug("Module::setThresholdLevel()");
    Q_ASSERT(lvl >= 1 && lvl <= 9);
    if (lvl == thresholdLevel()) {
        return ;
    }
    m_data.thresholdLevels[m_slot] = convertThresholdLevels(lvl);
    Q_ASSERT(thresholdLevel() == lvl);
    emit thresholdLevelChanged(lvl);
}

void Module::accept(Interfaces::ModuleVisitor &visitor)
{
    visitor.visit(*this);
}

bool Module::validateFrequency(KiloHertz frequency) const
{
    Q_UNUSED(frequency);
    return true;
}

void Module::setModuleStates(MDM500M::ModuleStates::States states)
{
    Q_UNUSED(states);
}

EmptyModule::EmptyModule(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable)
    : Module(slot, data, chTable)
{
    m_type = tr("-");
    m_isEmpty = true;
    m_isSupportSignalLevel = false;
}

void EmptyModule::accept(Interfaces::ModuleVisitor &visitor)
{
    visitor.visit(*this);
}

int EmptyModule::convertScaleLevelToSignalLevel(int) const
{
    return 0;
}

int EmptyModule::scaleLevelImpl(int8_t) const
{
    return 0;
}

int EmptyModule::signalLevelImpl(int8_t) const
{
    return 0;
}

int EmptyModule::thresholdLevelsImpl(int8_t) const
{
    return 0;
}

int8_t EmptyModule::convertThresholdLevels(int) const
{
    return 0;
}

bool EmptyModule::validateFrequency(KiloHertz) const
{
    return false;
}

UnknownModule::UnknownModule(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable)
    : Module(slot, data, chTable)
{
    m_type = tr("Неизвестный");
    m_isEmpty = true;
    m_isSupportSignalLevel = false;
}

void UnknownModule::accept(Interfaces::ModuleVisitor &visitor)
{
    visitor.visit(*this);
}

int UnknownModule::convertScaleLevelToSignalLevel(int) const
{
    return 0;
}

int UnknownModule::scaleLevelImpl(int8_t ) const
{
    return 0;
}

int UnknownModule::signalLevelImpl(int8_t) const
{
    return 0;
}

int UnknownModule::thresholdLevelsImpl(int8_t) const
{
    return 0;
}

int8_t UnknownModule::convertThresholdLevels(int) const
{
    return 0;
}

bool UnknownModule::validateFrequency(KiloHertz) const
{
    return false;
}

DM500::DM500(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable)
    : Module(slot, data, chTable)
{
    m_type = tr("ДМ-500");
    m_isSupportSignalLevel = ModuleInfo<DM500>::signalLevelSupport();
}

bool DM500::isThresholdLevelsSupported() const
{
    return m_data.type == DeviceType::MDM500M;
}

void DM500::accept(Interfaces::ModuleVisitor &visitor)
{
    visitor.visit(*this);
}

int DM500::convertScaleLevelToSignalLevel(int) const
{
    return 0;
}

int DM500::scaleLevelImpl(int8_t data) const
{
    switch (data) {
    case 0: return 0;
    case 1: return 3;
    case 2: return 5;
    case 3: return 7;
    case 4: return 9;
    default:
        Q_ASSERT(false);
        return 0;
    }
}

int DM500::signalLevelImpl(int8_t data) const
{
    Q_UNUSED(data);
    return 0;
}

int DM500::thresholdLevelsImpl(int8_t data) const
{
    if (data == 0) return 1;
    return scaleLevelImpl(data);
}

int8_t DM500::convertThresholdLevels(int lvl) const
{
    return ModuleInfo<DM500>::serializeThresholdLevel(lvl);
}

bool DM500::validateFrequency(KiloHertz f) const
{
    return ModuleInfo<DM500>::validateFrequency(f);
}

DM500M::DM500M(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable)
    : Module(slot, data, chTable)
{
    m_type = tr("ДМ-500М");
    m_isSupportSignalLevel = ModuleInfo<DM500M>::signalLevelSupport();
}

DM500M::VideoStandart DM500M::videoStandart() const
{
    return VideoStandart(m_data.config.modules[slot()].videoStandart);
}

DM500M::SoundStandart DM500M::soundStandart() const
{
    return SoundStandart(m_data.config.modules[slot()].soundStandart);
}

int DM500M::convertScaleLevelToSignalLevel(int value) const
{
    return ModuleInfo<DM500M>::convertScaleLevelToSignalLevel(value);
}

void DM500M::accept(Interfaces::ModuleVisitor &visitor)
{
    visitor.visit(*this);
}

void DM500M::setSoundStandart(DM500M::SoundStandart ss)
{
    if (soundStandart() == ss) {
        return ;
    }
    m_data.config.modules[slot()].soundStandart = static_cast<uint8_t>(ss);
    emit soundStandartChanged(ss);
}

void DM500M::setVideoStandart(DM500M::VideoStandart vs)
{
    if (videoStandart() == vs) {
        return ;
    }
    m_data.config.modules[slot()].videoStandart = static_cast<uint8_t>(vs);
    emit videoStandartChanged(vs);
}

bool DM500M::validateFrequency(KiloHertz f) const
{
    return ModuleInfo<DM500M>::validateFrequency(f);
}

int DM500M::scaleLevelImpl(int8_t data) const
{
    return ModuleInfo<DM500M>::deserializeScaleLevel(data);
}

int DM500M::signalLevelImpl(int8_t data) const
{
    return ModuleInfo<DM500M>::deserializeSignalLevel(data);
}

int DM500M::thresholdLevelsImpl(int8_t data) const
{
    return ModuleInfo<DM500M>::deserializeThresholdLevel(data);
}

int8_t DM500M::convertThresholdLevels(int lvl) const
{
    return ModuleInfo<DM500M>::serializeThresholdLevel(lvl);
}

DM500FM::DM500FM(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable)
    : Module(slot, data, chTable)
{
    m_type = tr("ДМ-500FM");
    m_isSupportSignalLevel = ModuleInfo<DM500FM>::signalLevelSupport();
}

bool DM500FM::isRds() const
{
    return m_data.config.modules[slot()].rds;
}

bool DM500FM::isStereo() const
{
    return m_data.config.modules[slot()].stereo;
}

int DM500FM::volume() const
{
    return m_data.config.modules[slot()].volume;
}

void DM500FM::setVolume(int v)
{
    Q_ASSERT(v >= 1 && v <= 15);
    if (volume() == v) {
        return ;
    }
    m_data.config.modules[slot()].volume = static_cast<uint8_t>(v);
    emit volumeChanged(v);
}

void DM500FM::accept(Interfaces::ModuleVisitor &visitor)
{
    visitor.visit(*this);
}

bool DM500FM::validateFrequency(KiloHertz f) const
{
    return ModuleInfo<DM500FM>::validateFrequency(f);
}

int DM500FM::convertScaleLevelToSignalLevel(int value) const
{
    return ModuleInfo<DM500FM>::convertScaleLevelToSignalLevel(value);
}

int DM500FM::scaleLevelImpl(int8_t data) const
{
    return ModuleInfo<DM500FM>::deserializeScaleLevel(data);
}

int DM500FM::signalLevelImpl(int8_t data) const
{
    return ModuleInfo<DM500FM>::deserializeSignalLevel(data);
}

int DM500FM::thresholdLevelsImpl(int8_t data) const
{
    return ModuleInfo<DM500FM>::deserializeThresholdLevel(data);
}

int8_t DM500FM::convertThresholdLevels(int lvl) const
{
    return ModuleInfo<DM500FM>::serializeThresholdLevel(lvl);
}

void DM500FM::setModuleStates(MDM500M::ModuleStates::States states)
{
    auto &config = m_data.config.modules[slot()];
    if (config.rds != states.rds) {
        config.rds = states.rds;
        emit rdsChanged(isRds());
    }
    if (config.stereo != states.stereo) {
        config.stereo = states.stereo;
        emit stereoChanged(isStereo());
    }
}

bool ModuleFabric::mustBeReplaced(Module *module, int typeIndex) const
{
    switch (typeIndex) {
    case ModuleInfo<EmptyModule>::typeIndex():
        return typeid(*module) != typeid(EmptyModule &);
    case ModuleInfo<DM500>::typeIndex():
        return typeid(*module) != typeid(DM500 &);
    case ModuleInfo<DM500M>::typeIndex():
        return typeid(*module) != typeid(DM500M &);
    case ModuleInfo<DM500FM>::typeIndex():
        return typeid(*module) != typeid(DM500FM &);
    default:
        return typeid(*module) != typeid(UnknownModule &);
    }
}

Module *ModuleFabric::createModule(int typeIndex, int slot, DeviceData &data)
{
    switch (typeIndex) {
    case ModuleInfo<EmptyModule>::typeIndex():
        return new EmptyModule(slot, data, NullChannelTable::globalInstance());
    case ModuleInfo<DM500>::typeIndex():
        return new DM500(slot, data, TvChannelTable::globalInstance());
    case ModuleInfo<DM500M>::typeIndex():
        return new DM500M(slot, data, TvChannelTable::globalInstance());
    case ModuleInfo<DM500FM>::typeIndex():
        return new DM500FM(slot, data, NullChannelTable::globalInstance());
    default:
        return new UnknownModule(slot, data, NullChannelTable::globalInstance());
    }
}

ModuleError::ModuleError(Errors errors)
    : m_errors(errors)
{
}

bool ModuleError::isError() const
{
    return m_errors != NoErrors;
}

QString ModuleError::toString() const
{
    if (m_errors == NoErrors)              return QObject::tr(QT_TR_NOOP("Норма"));
    if (m_errors.testFlag(SlotFault     )) return QObject::tr(QT_TR_NOOP("Слот неисправен"));
    if (m_errors.testFlag(NotResponding )) return QObject::tr(QT_TR_NOOP("Не отвечает"));
    if (m_errors.testFlag(PatfFault     )) return QObject::tr(QT_TR_NOOP("Авария ФАПЧ"));
    if (m_errors.testFlag(Changed       )) return QObject::tr(QT_TR_NOOP("Заменен"));
    if (m_errors.testFlag(LowSignalLevel)) return QObject::tr(QT_TR_NOOP("Низкий уровень сигнала"));
    return QObject::tr(QT_TR_NOOP("Неизвестная ошибка"));
}

ModuleError::Errors ModuleError::flags() const
{
    return m_errors;
}

bool ModuleError::testFlag(ModuleError::Error e) const
{
    return m_errors.testFlag(e);
}

void ModuleError::setFlags(Errors errors)
{
    m_errors = errors;
}

void ModuleError::setFlag(ModuleError::Error e, bool on)
{
    m_errors.setFlag(e, on);
}

bool ModuleError::operator==(ModuleError e) const
{
    return m_errors == e.m_errors;
}

bool ModuleError::operator!=(ModuleError e) const
{
    return !(*this == e);
}

static const char *m_lvlNames[10]
{
    QT_TR_NOOP("Нет сигнала (0)"),
    QT_TR_NOOP("Слабый уровень (1)"),
    QT_TR_NOOP("Нормальный уровень (2)"),
    QT_TR_NOOP("Нормальный уровень (3)"),
    QT_TR_NOOP("Нормальный уровень (4)"),
    QT_TR_NOOP("Высокий уровень (5)"),
    QT_TR_NOOP("Слишком высокий уровень (6)"),
    QT_TR_NOOP("Перегруз (7)"),
    QT_TR_NOOP("Перегруз (8)"),
    QT_TR_NOOP("Перегруз (9)")
};

ScaleLevel::ScaleLevel(int lvl)
{
    set(lvl);
}

QString ScaleLevel::toString() const
{
    return m_lvlNames[m_lvl];
}

ScaleLevel::operator int() const
{
    return m_lvl;
}

void ScaleLevel::set(int lvl)
{
    m_lvl = qBound(0, lvl, 9);
}
