#pragma once

#include <memory>

#include "Device.h"
#include "Frequency.h"

class DM500;
class DM500M;
class DM500FM;
class EmptyModule;
class Module;
class UnknownModule;

namespace Interfaces {

class ChannelTable;

class ModuleVisitor
{
public:
    virtual ~ModuleVisitor() = default;
    virtual void visit(Module &) = 0;
    virtual void visit(DM500 &) = 0;
    virtual void visit(DM500M &) = 0;
    virtual void visit(DM500FM &) = 0;
    virtual void visit(EmptyModule &) = 0;
    virtual void visit(UnknownModule &) = 0;
};

} // namespace Interfaces

class ScaleLevel
{
public:
    explicit ScaleLevel(int lvl = 0);

    operator int() const;
    QString toString() const;
    void set(int lvl);

private:
    int m_lvl;
};

class ModuleError
{
public:
    enum Error
    {
        NoErrors        = 0,  // Нет ошибок
        LowSignalLevel  = 1,  // Низкий уровень сигнала
        Changed         = 2,  // Заменен
        PatfFault       = 4,  // Авария ФАПЧ
        NotResponding   = 8,  // Не отвечает
        SlotFault       = 16  // Слот не исправен
    };
    Q_DECLARE_FLAGS(Errors, Error)

    explicit ModuleError(Errors errors = NoErrors);
    bool isError() const;
    QString toString() const;
    Errors flags() const;
    bool testFlag(Error e) const;
    void setFlags(Errors errors);
    void setFlag(Error e, bool on = true);
    bool operator==(ModuleError e) const;
    bool operator!=(ModuleError e) const;

private:
    Errors m_errors;
};

class Module : public QObject
{
    Q_OBJECT

public:
    Module(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable);
    bool isEmpty() const;
    QString type() const;
    int slot() const;

    QString channel() const;
    QStringList allChannels() const;
    void setChannel(QString channel);

    KiloHertz frequency() const;
    bool setFrequency(KiloHertz frequency);
    template <typename T, typename Q>
    bool setFrequency(Frequency<T, Q> f);

    bool isDiagnosticEnabled() const;
    void setDiagnostic(bool on);

    bool isError() const;
    ModuleError error() const;
    ScaleLevel scaleLevel() const;
    bool isSupportSignalLevel() const;
    int signalLevel() const;
    int thresholdLevel() const;
    virtual int convertScaleLevelToSignalLevel(int value) const = 0;
    void setThresholdLevel(int lvl);
    virtual void accept(Interfaces::ModuleVisitor &visitor);

signals:
    void frequencyChanged();
    void channelChanged(QString);
    void diagnosticChanged(bool);
    void signalLevelChanged(int scaleLevel, int signalLevel);
    void thresholdLevelChanged(int);
    void errorsChanged();

protected:
    virtual int scaleLevelImpl(int8_t data) const = 0;
    virtual int signalLevelImpl(int8_t data) const = 0;
    virtual int thresholdLevelsImpl(int8_t data) const = 0;
    virtual int8_t convertThresholdLevels(int lvl) const = 0;
    virtual bool validateFrequency(KiloHertz frequency) const = 0;
    virtual void setModuleStates(MDM500M::ModuleStates::States states);

    QString m_type;
    DeviceData &m_data;
    bool m_isSupportSignalLevel = true;
    bool m_isEmpty = false;

private:
    friend class Device;

    QString m_channel;
    std::shared_ptr<Interfaces::ChannelTable> m_chTable;
    int m_slot;
};

class EmptyModule : public Module
{
    Q_OBJECT

public:
    EmptyModule(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable);
    void accept(Interfaces::ModuleVisitor &visitor) override;
    int convertScaleLevelToSignalLevel(int value) const override;

protected:
    int scaleLevelImpl(int8_t data) const override;
    int signalLevelImpl(int8_t data) const override;
    int thresholdLevelsImpl(int8_t data) const override;
    int8_t convertThresholdLevels(int lvl) const override;
    bool validateFrequency(KiloHertz frequency) const override;
};

class UnknownModule : public Module
{
    Q_OBJECT

public:
    UnknownModule(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable);
    void accept(Interfaces::ModuleVisitor &visitor) override;
    int convertScaleLevelToSignalLevel(int value) const override;

protected:
    int scaleLevelImpl(int8_t data) const override;
    int signalLevelImpl(int8_t data) const override;
    int thresholdLevelsImpl(int8_t data) const override;
    int8_t convertThresholdLevels(int lvl) const override;
    bool validateFrequency(KiloHertz frequency) const override;
};

class DM500 : public Module
{
    Q_OBJECT

public:
    DM500(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable);
    bool isThresholdLevelsSupported() const;
    void accept(Interfaces::ModuleVisitor &visitor) override;
    int convertScaleLevelToSignalLevel(int value) const override;

protected:
    int scaleLevelImpl(int8_t data) const override;
    int signalLevelImpl(int8_t data) const override;
    int thresholdLevelsImpl(int8_t data) const override;
    int8_t convertThresholdLevels(int lvl) const override;
    bool validateFrequency(KiloHertz khz) const override;
};

class DM500M : public Module
{
    Q_OBJECT

public:
    enum SoundStandart
    {
        NICAM = 0,
        A2    = 1
    };
    Q_ENUM(SoundStandart)

    enum VideoStandart
    {
        DK = 0,
        BG = 1
    };
    Q_ENUM(VideoStandart)

    DM500M(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable);
    VideoStandart videoStandart() const;
    SoundStandart soundStandart() const;

signals:
    void videoStandartChanged(VideoStandart);
    void soundStandartChanged(SoundStandart);

public:
    int convertScaleLevelToSignalLevel(int value) const override;
    void accept(Interfaces::ModuleVisitor &visitor) override;
    void setSoundStandart(SoundStandart soundStandart);
    void setVideoStandart(VideoStandart videoStandart);

protected:
    bool validateFrequency(KiloHertz f) const override;
    int scaleLevelImpl(int8_t data) const override;
    int signalLevelImpl(int8_t data) const override;
    int thresholdLevelsImpl(int8_t data) const override;
    int8_t convertThresholdLevels(int lvl) const override;
};

class DM500FM : public Module
{
    Q_OBJECT

public:
    DM500FM(int slot, DeviceData &data, std::shared_ptr<Interfaces::ChannelTable> chTable);
    bool isRds() const;
    bool isStereo() const;
    int volume() const;
    void setVolume(int volume);
    void accept(Interfaces::ModuleVisitor &visitor) override;

signals:
    void rdsChanged(bool);
    void stereoChanged(bool);
    void volumeChanged(int);

protected:
    bool validateFrequency(KiloHertz f) const override;
    void setModuleStates(MDM500M::ModuleStates::States states) override;

    // Module interface
public:
    int convertScaleLevelToSignalLevel(int value) const override;

protected:
    int scaleLevelImpl(int8_t data) const override;
    int signalLevelImpl(int8_t data) const override;
    int thresholdLevelsImpl(int8_t data) const override;
    int8_t convertThresholdLevels(int lvl) const override;
};

class ModuleFabric : public Interfaces::ModuleFabric
{
public:
    bool mustBeReplaced(Module *module, int typeIndex) const override;
    Module *createModule(int typeIndex, int slot, DeviceData &data) override;
};

template <typename T, typename Q>
bool Module::setFrequency(Frequency<T, Q> f)
{
    return setFrequency(frequency_cast<KiloHertz>(f));
}
