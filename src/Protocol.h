#pragma once

#include <QSerialPort>

class Protocol
{
public:
    /**
     * @brief Команды управления устройствами МДМ-500 и МДМ-500М
     */
    enum class Command : uint8_t
    {
        /* Общие команды */
        Ok,   	                /**< Ответ МДМ-500 на команду WriteConfig при успешной записи */
        ReadConfig,             /**< Прочитать конфигурацию устройства            */
        WriteConfig,            /**< Записать конфигурацию устройства в постоянную память */
        ReadSignalLevels,       /**< Прочитать уровни сигналов модулей            */
        ReadInfo,               /**< Прочитать базовую информацию об устройстве   */
        Error,   	            /**< Ответ МДМ-500 на команду WriteConfig при ошибке */

        /* Команды, поддерживаемые только МДМ-500М */
        WriteTempModuleConfig,  /**< Временная установка настроек модуля.
                                        Если минуту нет активности оператора
                                        - возврат к сохраненному перед этим         */
        WriteTempControlModule, /**< Временная смена контрольного канала.
                                        Если минуту нет активности оператора
                                        - возврат к сохраненным                     */
        ReadErrors,             /**< Прочитать ошибки                             */
        WriteTemplateConfig,    /**< Не реализовано: Для записи шаблона в EEPROM (пока может быть 4 шаблона)  */
        ReadTemplateConfig,     /**< Не реализовано: Для чтения шаблона из EEPROM (пока может быть 4 шаблона) */
        ReadThresholdLevels,    /**< Прочитать пороговые уровни сигналов          */
        WriteThresholdLevels,   /**< Записать пороговые уровни сигналов           */
        Reboot,                 /**< Перезагрузка устройства                      */
        ResetErrors,            /**< Очистить ошибки                              */
        ReadModuleStates        /**< Чтение состояний модулей                     */
    };

    /**
     * @brief Коды ошибок, возникающие при записи данных на устройство
     */
    enum class Error : uint8_t
    {
        Ok = 0,
        BadParamNumber = 1, /**< WriteTempModuleConfig неверное значение параметра slot                       */
        WrongParam = 2,     /**< WriteConfig, WriteTempModuleConfig некорректные данные (тип модуля, частота) */
        CantWrite = 5,      /**< WriteConfig - не записалось, WriteTempModuleConfig - не загрузился модуль    */
    };

    Protocol(QSerialPort &port);
    bool get(Command cmd, void *data, size_t size, std::chrono::milliseconds timeout = readTimeout);
    bool set(Command cmd, Error &error, const void *data = nullptr, size_t size = 0u, std::chrono::milliseconds timeout = readTimeout);

    template <typename Data>
    bool get(Command cmd, Data &data)
    {
        return get(cmd, &data, sizeof(Data));
    }

    template <typename Data, typename T, typename Q>
    bool get(Command cmd, Data &data, std::chrono::duration<T, Q> timeout)
    {
        return get(cmd, &data, sizeof(Data),
                   std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
    }

    template <typename Data>
    bool set(Command cmd, Error &error, Data &&data)
    {
        return set(cmd, error, &data, sizeof(Data));
    }

    template <typename Data, typename T, typename Q>
    bool set(Command cmd, Error &error, Data &&data, std::chrono::duration<T, Q> timeout)
    {
        return set(cmd, error, &data, sizeof(Data),
                   std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
    }

private:
    static constexpr int attemptsMaxCount = 2;
    static constexpr size_t  minFullSize = 4;
    static constexpr uint8_t minSize = 2;
    static constexpr uint8_t marker  = 0xA5;
    static constexpr std::chrono::milliseconds writeTimeout { 500 };
    static constexpr std::chrono::milliseconds readTimeout { 1000 };

    bool configure();
    uint8_t getch();
    void putch(uint8_t ch);
    uint8_t calcCrc(Command cmd, const void *data, size_t size);
    bool wait(size_t size, std::chrono::milliseconds timeout);
    bool read(Command cmd, void *data, size_t size, std::chrono::milliseconds timeout);
    bool write(Command cmd, const void *data, size_t size);

    QSerialPort &m_port;
};
