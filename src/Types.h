#pragma once

#include <assert.h>
#include <stdint.h>

#include <QString>
#include <QVersionNumber>

#include "Frequency.h"

#pragma pack(push, 1)

enum class DeviceType : int
{
    MDM500,
    MDM500M
};

/**
 * @brief Версия программного обеспечения
 */
struct SoftwareVersion
{
    SoftwareVersion()
        : build  (0)
        , release(0)
        , minor  (0)
        , major  (1)
    {}

    SoftwareVersion(uint16_t data)
    {
        memcpy(this, &data, sizeof(uint16_t));
    }

    SoftwareVersion(uint16_t major, uint16_t minor, uint16_t release, uint16_t build)
        : build(build), release(release), minor(minor), major(major)
    {}

    uint16_t build   : 4;
    uint16_t release : 4;
    uint16_t minor   : 4;
    uint16_t major   : 4;
    
    bool operator == (SoftwareVersion rhs) const
    {
        return major   == rhs.major
            && minor   == rhs.minor
            && release == rhs.release
            && build   == rhs.build;
    }
    
    bool operator <  (SoftwareVersion r) const
    {
        QVersionNumber lhs { major, minor, release, build };
        QVersionNumber rhs { r.major, r.minor, r.release, r.build };
        return lhs < rhs;
    }
    
    bool operator != (SoftwareVersion rhs) const
    {
        return !(*this == rhs);
    }
    
    bool operator <= (SoftwareVersion rhs) const
    {
        return (*this < rhs) || (*this == rhs);
    }
    
    bool operator >  (SoftwareVersion rhs) const
    {
        return !(*this <= rhs);
    }
    
    bool operator >= (SoftwareVersion rhs) const
    {
        return !(*this < rhs);
    }
    
    QString toString() const
    {
        return QString("%1.%2.%3.%4")
                .arg(major  )
                .arg(minor  )
                .arg(release)
                .arg(build  );
    }
};
static_assert(sizeof(SoftwareVersion) == 2, "");

/**
 * @brief Версия аппаратного обеспечения
 */
struct HardwareVersion
{
    uint16_t type;
    uint8_t  modification;
    uint8_t  group;
    
    bool operator == (HardwareVersion rhs) const
    {
        return type         == rhs.type
            && modification == rhs.modification
            && group        == rhs.group;
    }
};
static_assert(sizeof(HardwareVersion) == 4, "");

struct SerialNumber
{
    QString toString(DeviceType format) const
    {
        switch (format) {
        case DeviceType::MDM500:
            return QString("%1").arg(value, 5, 10, QChar('0'));
        case DeviceType::MDM500M:
            return QString("PSN%1").arg(value, 9, 10, QChar('0'));
        default:
            Q_ASSERT(false);
            return QString();
        }
    }

    uint32_t value;
};

/**
 * @brief Кол-во слотов в МДМ-500 и МДМ-500М
 */
constexpr int kSlotCount = 16;

/**
 * @brief Уровни сигналов
 */
struct SignalLevels
{
    inline int8_t &operator [] (ptrdiff_t i)
    {
        return values[i];
    }

    inline int8_t operator [] (ptrdiff_t i) const
    {
        return values[i];
    }

    int8_t values[kSlotCount];
};
static_assert(sizeof(SignalLevels) == 16, "");

namespace MDM500M {

/**
 * @brief Версия аппаратного обеспечения МДМ-500М
 */
constexpr HardwareVersion kHardwareVersion = { 5, 1, 0 };

/**
 * @brief Информация об устройстве
 */
struct DeviceInfo
{
    SerialNumber serialNumber;
    SoftwareVersion softwareVersion;
    HardwareVersion hardwareVersion;
};
static_assert(sizeof(DeviceInfo) == 10, "");

/**
 * @brief Структура конфигурации модулей МДМ-500М
 */
struct ModuleConfig
{
    uint32_t frequency       : 20;   /**< Частота в кГц                      */
    uint32_t type            : 4;    /**< Тип модуля                         */
    uint32_t blockDiagnostic : 1;    /**< Блокировка диагностики             */
    uint32_t isModule        : 1;    /**< В слоте есть модуль                */
    uint32_t i2c             : 1;    /**< Ошибка I2C (слот поврежден)        */
    uint32_t changed         : 1;    /**< Было изменение конфигурации демодулятора */
    uint32_t fault           : 1;    /**< Модуль неисправен                  */
    uint32_t lowLevel        : 1;    /**< Низкий уровень сигнала             */
    uint32_t                 : 2;
    union 
    {
        struct // Общее
        {
            uint8_t          : 7;
            uint8_t patf     : 1;    /**< Авария ФАПЧ                        */
        };

        struct // ДМ-500M
        {
            uint8_t videoStandart: 1; /**< Стандарт видео сигнала            */
            uint8_t soundStandart: 1; /**< Стандарт аудио сигнала            */
            uint8_t              : 6;
        };
        
        struct // ДМ-500FM
        {
            uint8_t volume   : 4;    /**< Громкость                          */
            uint8_t rds      : 1;    /**< Присутсвие RDS                     */
            uint8_t stereo   : 1;    /**< Присутсвие стереомодуляции         */
            uint8_t          : 2;
        };
    };
};
static_assert(sizeof(ModuleConfig) == 5, "");

/**
 * @brief Кол-во слотов в МДМ-500М
 */
constexpr int kSlotCount = ::kSlotCount;

/**
 * @brief Структура конфигурации устройства МДМ-500М
 */
struct DeviceConfig
{
    ModuleConfig modules[kSlotCount]; /**< Конфигурация всех модулей */
    uint8_t control : 4;              /**< Номер контрольного модуля */
    uint8_t         : 4;
};
static_assert(sizeof(DeviceConfig) == 81, "");

/**
 * @brief Структура ошибок устройства МДМ-500М
 *
 * Каждый i-ый бит в членах этой структуры отвечает за i-ый модуль.
 */
struct DeviceErrors
{
    struct Errors 
    {
        bool lowLevel;
        bool fault;
        bool patf;
    };
    
    inline Errors operator [] (ptrdiff_t i) const 
    {
        uint16_t mask = 1 << i;
        return Errors { !!(lowLevel & mask), !!(fault & mask), !!(patf & mask) };
    }
    
    operator bool () const
    {
        return !!lowLevel || !!fault || !!patf;
    }

    uint16_t lowLevel;       /**< Нет сигнала                             */
    uint16_t fault;          /**< Неисправность I2C или модуль не ответил */
    uint16_t patf;           /**< Авария ФАПЧ                             */
};
static_assert(sizeof(DeviceErrors) == 6, "");

/**
 * @brief Структура пакета с ошибками устройства МДМ-500М
 */
struct ErrorsPackage
{
    bool isResetRequired() const 
    {
        bool retval = false;
        auto checkForReset = [](uint16_t log, uint16_t curr) -> bool {
            return ~curr & log; // Ошибка была, но сейчас отсутствует
        };
        retval = retval || checkForReset(log.lowLevel, current.lowLevel);
        retval = retval || checkForReset(log.fault, current.fault);
        retval = retval || checkForReset(log.patf, current.patf);
        return retval;
    }

    DeviceErrors current; /**< Текущие ошибки */
    DeviceErrors log;     /**< Ошибки, происходившие с момента последнего подключения */
};
static_assert(sizeof(ErrorsPackage) == 12, "");

/**
 * @brief Дополнительные состояния модулей, которые могут меняться со временем
 */
struct ModuleStates
{
    struct States 
    {
        bool rds;
        bool stereo;
    };
    
    inline States operator [] (ptrdiff_t i) const 
    {
        uint16_t mask = 1 << i;
        return States { !!(rds & mask), !!(stereo & mask) };
    }
    
    uint16_t rds;          /**< Состояние RDS     */
    uint16_t stereo;       /**< Присутсвие стерео */
    uint16_t reserved_1;   /**< Резерв            */
    uint16_t reserved_2;   /**< Резерв            */
};
static_assert(sizeof(ModuleStates) == 8, "");

/**
 * @brief Конфигурация модуля с номером слота
 */
struct ModuleConfigWithSlot
{
    uint8_t slot;          /**< Слот         */
    ModuleConfig config;   /**< Конфигурация */
};
static_assert(sizeof(ModuleConfigWithSlot) == 6, "");

typedef ::SignalLevels SignalLevels;

} // namespace MDM500M

namespace MDM500 {

/**
 * @brief Кол-во слотов в МДМ-500
 */
constexpr int kSlotCount = ::kSlotCount;

/**
 * @brief Информация об устройстве
 */
struct DeviceInfo
{
    uint16_t serialNumber;
};

/**
 * @brief Структура конфигурации модулей МДМ-500
 */
struct ModuleConfig
{
    MDM500M::ModuleConfig convertToMDM500M() const
    {
        MDM500M::ModuleConfig retval {};
        retval.type = isModule;
        retval.isModule = isModule;
        retval.frequency = frequencyMhz * 1000 + frequencyKhz * 10;
        retval.blockDiagnostic = blockDiagnostic;
        return retval;
    }

    static ModuleConfig fromMDM500M(MDM500M::ModuleConfig config)
    {
        ModuleConfig retval;
        retval.frequencyMhz = config.frequency / 1000;
        retval.frequencyKhz = (config.frequency / 10) % 100;
        retval.blockDiagnostic = config.blockDiagnostic;
        retval.isModule = config.isModule;
        return retval;
    }

    uint16_t frequencyMhz;        /**< Частота, значение до запятой    */
    uint8_t  frequencyKhz;        /**< Частота, значение после запятой */
    uint8_t  isModule        : 1; /**< Модуль присутствует             */
    uint8_t                  : 1;
    uint8_t  blockDiagnostic : 1; /**< Блокировка диагностики          */
};

/**
 * @brief Структура конфигурации МДМ-500
 */
struct DeviceConfig
{
    MDM500M::DeviceConfig convertToMDM500M() const
    {
        MDM500M::DeviceConfig retval {};
        for (int slot = 0; slot < kSlotCount; ++slot) {
            retval.modules[slot] = modules[slot].convertToMDM500M();
        }
        retval.control = control;
        return retval;
    }

    static DeviceConfig fromMDM500M(const MDM500M::DeviceConfig &config)
    {
        DeviceConfig retval {};
        for (int slot = 0; slot < kSlotCount; ++slot) {
            retval.modules[slot] = ModuleConfig::fromMDM500M(config.modules[slot]);
        }
        retval.control = config.control;
        return retval;
    }

    ModuleConfig modules[kSlotCount]; /**< Конфигурация всех модулей   */
    uint8_t control;                  /**< Номер контрольного модуля   */
};

typedef ::SignalLevels SignalLevels;

} // namespace MDM500

class DM500;
class DM500M;
class DM500FM;
class EmptyModule;
class UnknownModule;

template <typename T>
struct ModuleInfo
{
};

template <>
struct ModuleInfo<EmptyModule>
{
    constexpr static int typeIndex()
    {
        return 0;
    }
};

template <>
struct ModuleInfo<UnknownModule>
{
    constexpr static int typeIndex()
    {
        return 15;
    }
};

template <>
struct ModuleInfo<DM500>
{
    constexpr static int typeIndex()
    {
        return 1;
    }

    constexpr static KiloHertz minFrequency()
    {
        return 48_MHz;
    }

    constexpr static KiloHertz maxFrequency()
    {
        return 862_MHz;
    }

    template<typename T, typename Q>
    constexpr static bool validateFrequency(Frequency<T, Q> frequency)
    {
        return minFrequency() <= frequency && frequency <= maxFrequency();
    }

    constexpr static bool signalLevelSupport()
    {
        return false;
    }

    constexpr static bool validateThresholdLevel(int value)
    {
        switch (value) {
        case 1: case 3: case 5: case 7: case 9: return true;
        default: return false;
        }
    }

    constexpr static int deserializeScaleLevel(uint8_t data)
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

    constexpr static int deserializeThresholdLevel(uint8_t data)
    {
        switch (data) {
        case 0: return 1;
        case 1: return 3;
        case 2: return 5;
        case 3: return 7;
        case 4: return 9;
        default:
            Q_ASSERT(false);
            return 0;
        }
    }

    constexpr static int8_t serializeThresholdLevel(int lvl)
    {
        switch (lvl) {
        case 1: return 0;
        case 3: return 1;
        case 5: return 2;
        case 7: return 3;
        case 9: return 4;
        default:
            Q_ASSERT(false);
            return 0;
        }
    }
};

template <>
struct ModuleInfo<DM500M>
{
    constexpr static int typeIndex()
    {
        return 2;
    }

    constexpr static KiloHertz minFrequency()
    {
        return 48_MHz;
    }

    constexpr static KiloHertz maxFrequency()
    {
        return 862_MHz;
    }

    template<typename T, typename Q>
    constexpr static bool validateFrequency(Frequency<T, Q> frequency)
    {
        return minFrequency() <= frequency && frequency <= maxFrequency();
    }

    constexpr static bool validateThresholdLevel(int value)
    {
        return 0 <= value && value <= 9;
    }

    constexpr static bool signalLevelSupport()
    {
        return true;
    }

    constexpr static int8_t signalCorrection()
    {
        return 6;
    }

    constexpr static int8_t signalMinLevel()
    {
        return 30;
    }

    constexpr static int8_t signalDivider()
    {
        return 10;
    }

    constexpr static int deserializeScaleLevel(int8_t data)
    {
        return (data - signalMinLevel()) / signalDivider();
    }

    constexpr static int deserializeThresholdLevel(int8_t data)
    {
        return deserializeScaleLevel(data);
    }

    constexpr static int deserializeSignalLevel(int8_t data)
    {
        return data + signalCorrection();
    }

    constexpr static int8_t serializeThresholdLevel(int lvl)
    {
        return static_cast<int8_t>(lvl * signalDivider() + signalMinLevel());
    }

    constexpr static int convertScaleLevelToSignalLevel(int scale)
    {
        return scale * signalDivider() + signalMinLevel() + signalCorrection();
    }
};

template <>
struct ModuleInfo<DM500FM>
{
    constexpr static int typeIndex()
    {
        return 3;
    }

    constexpr static KiloHertz minFrequency()
    {
        return 62_MHz;
    }

    constexpr static KiloHertz maxFrequency()
    {
        return 108_MHz;
    }

    template<typename T, typename Q>
    constexpr static bool validateFrequency(Frequency<T, Q> frequency)
    {
        return (62_MHz <= frequency && frequency <= 74_MHz)
            || (76_MHz <= frequency && frequency <= 108_MHz);
    }

    constexpr static bool validateThresholdLevel(int value)
    {
        return 0 <= value && value <= 9;
    }

    constexpr static bool signalLevelSupport()
    {
        return true;
    }

    constexpr static int8_t signalCorrection()
    {
        return -10;
    }

    constexpr static int8_t signalMinLevel()
    {
        return 25;
    }

    constexpr static int8_t signalDivider()
    {
        return 8;
    }

    constexpr static int deserializeScaleLevel(int8_t data)
    {
        return (data - signalMinLevel()) / signalDivider();
    }

    constexpr static int deserializeThresholdLevel(int8_t data)
    {
        return deserializeScaleLevel(data);
    }

    constexpr static int deserializeSignalLevel(int8_t data)
    {
        return data + signalCorrection();
    }

    constexpr static int8_t serializeThresholdLevel(int lvl)
    {
        return static_cast<int8_t>(lvl * signalDivider() + signalMinLevel());
    }

    constexpr static int convertScaleLevelToSignalLevel(int scale)
    {
        return scale * signalDivider() + signalMinLevel() + signalCorrection();
    }

    constexpr static int minVolume()
    {
        return 1;
    }

    constexpr static int maxVolume()
    {
        return 15;
    }

    constexpr static bool validateVolume(int value)
    {
        return minVolume() <= value && value <= maxVolume();
    }
};

#pragma pack(pop)
