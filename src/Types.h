#pragma once

#include <stdint.h>

#include <QString>
#include <QVersionNumber>

#pragma pack(push, 1)

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
    QString toString() const
    {
        return QString("PSN%1").arg(value, 9, 10, QChar('0'));
    }

    uint32_t value;
};

/**
 * @brief Версия аппаратного обеспечения МДМ-500М
 */
constexpr HardwareVersion kMDM500MHardwareVersion = { 5, 1, 0 };

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
constexpr int kMDM500MSlotCount = 16;

/**
 * @brief Структура конфигурации устройства МДМ-500М
 */
struct DeviceConfig
{
    ModuleConfig modules[kMDM500MSlotCount]; /**< Конфигурация всех модулей */
    uint8_t control : 4;                     /**< Номер контрольного модуля */
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
    
    int8_t values[kMDM500MSlotCount];
};
static_assert(sizeof(SignalLevels) == 16, "");

#pragma pack(pop)
