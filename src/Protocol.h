#pragma once

#include <QDeadlineTimer>
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

    struct ReaderTag {};

    // struct Reader
    // {
    //     typedef Protocol::ReaderTag ReaderTag;
    //     bool isDataEnough(int size);
    //     bool checkPackage(Protocol::Command cmd, const void *data, int size);
    // };

    Protocol(QSerialPort &port);
    bool configure();

    template <typename AnswerData>
    bool get(Command cmd, AnswerData &data);

    template <typename AnswerData, typename T, typename Q>
    bool get(Command cmd, AnswerData &data, std::chrono::duration<T, Q> timeout);

    template <typename Params>
    bool set(Command cmd, Error &error, Params &&data);

    template <typename Params, typename T, typename Q>
    bool set(Command cmd, Error &error, Params &&data,
             std::chrono::duration<T, Q> timeout);

    template <typename Reader>
    bool get(Command request, Reader &&reader, ReaderTag);

    template <typename Reader, typename T, typename Q>
    bool get(Command request, Reader &&reader, std::chrono::duration<T, Q> timeout, ReaderTag);

    template <typename Reader>
    bool set(Command cmd, const void *data, int size, Reader &&reader, ReaderTag);

    template <typename Params, typename Reader>
    bool set(Command cmd, Params &&data, Reader &&reader, ReaderTag);

    template <typename Params, typename Reader, typename T, typename Q>
    bool set(Command cmd, Params &&data, Reader &&reader,
             std::chrono::duration<T, Q> timeout, ReaderTag);

    template <typename Reader, typename T, typename Q>
    bool set(Command cmd, const void *data, int size, Reader &&reader,
             std::chrono::duration<T, Q> timeout, ReaderTag);


    bool get(Command request, void *readBuffer, int readBufferSize,
             std::chrono::milliseconds timeout = kDefaultReadTimeout);

    bool set(Command cmd, Error &error, const void *cmdParams = nullptr,
             int cmdParamsSize = 0,
             std::chrono::milliseconds timeout = kDefaultReadTimeout);

private:
    static constexpr int     kMaxAttemptsCount    = 2;
    static constexpr size_t  kMinPackageSize      = 4;
    static constexpr uint8_t kMinValueOfSizeField = 2;
    static constexpr uint8_t kPackageMarker       = 0xA5;
    static constexpr std::chrono::milliseconds kWriteTimeout { 500 };
    static constexpr std::chrono::milliseconds kDefaultReadTimeout { 1000 };

    bool performDefaultDialog(Command cmd, const void *writeBuffer, int writeBufferSize,
                              void *readBuffer, int readBufferSize, std::chrono::milliseconds timeout);

    template <typename Reader>
    bool wait(Reader &&reader, QDeadlineTimer timer);

    bool write(Command cmd, const void *data, int size);

    template <typename Reader>
    bool read(Reader &&reader, std::chrono::milliseconds timeout);

    template <typename Reader>
    bool readPackage(Reader &&reader);

    uint8_t calcCrc(Command cmd, const void *data, int size);

    QSerialPort &m_port;
};

class DefaultReader
{
public:
    DefaultReader(Protocol::Command cmd, void *buffer, int size);
    bool isDataEnough(int size) const;
    bool checkPackage(Protocol::Command cmd, const void *data, int size);

private:
    void *m_buffer;
    int m_size;
    Protocol::Command m_cmd;
};

inline uint8_t getch(QIODevice &device)
{
    uint8_t retval;
    device.getChar(reinterpret_cast<char *>(&retval));
    return retval;
}

template <typename Reader>
bool Protocol::wait(Reader &&reader, QDeadlineTimer timer)
{
    while (!timer.hasExpired()) {
        auto bytesAvailable = m_port.bytesAvailable();
        if (bytesAvailable >= kMinValueOfSizeField) {
            if (reader.isDataEnough(bytesAvailable - kMinValueOfSizeField)) {
                return true;
            }
        }
        m_port.waitForReadyRead(timer.remainingTime());
    }
    return false;
}

template <typename Reader>
bool Protocol::read(Reader &&reader, std::chrono::milliseconds timeout)
{
    QDeadlineTimer timer(timeout);

    do {
        if (!wait(reader, timer)) {
            return false;
        }
    }
    while (!readPackage(reader));

    return true;
}

template <typename Reader>
bool Protocol::readPackage(Reader &&reader)
{
    if (getch(m_port) != kPackageMarker) {
        return false;
    }
    auto sizeField   = getch(m_port);
    if (sizeField < kMinValueOfSizeField) {
        return false;
    }
    auto cmdField    = static_cast<Command>(getch(m_port));
    auto dataField   = m_port.read(sizeField - kMinValueOfSizeField);
    auto expectedCrc = getch(m_port);
    auto actualCrc   = calcCrc(cmdField, dataField.data(), dataField.size());
    if (expectedCrc != actualCrc) {
        return false;
    }
    return reader.checkPackage(cmdField, dataField.data(), dataField.size());
}

template <typename Data>
bool Protocol::get(Command cmd, Data &data)
{
    return get(cmd, &data, sizeof(Data));
}

template <typename Data, typename T, typename Q>
bool Protocol::get(Command cmd, Data &data, std::chrono::duration<T, Q> timeout)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return get(cmd, &data, sizeof(Data),
               duration_cast<milliseconds>(timeout));
}

template <typename Data>
bool Protocol::set(Command cmd, Error &error, Data &&data)
{
    return set(cmd, error, &data, sizeof(Data));
}

template <typename Data, typename T, typename Q>
bool Protocol::set(Command cmd, Error &error, Data &&data, std::chrono::duration<T, Q> timeout)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return set(cmd, error, &data, sizeof(Data),
               duration_cast<milliseconds>(timeout));
}

template <typename Reader>
bool Protocol::get(Command request, Reader &&reader, ReaderTag)
{
    return get(request, std::forward<Reader>(reader), kDefaultReadTimeout, ReaderTag());
}

template <typename Reader, typename T, typename Q>
bool Protocol::get(Command request, Reader &&reader, std::chrono::duration<T, Q> timeout, ReaderTag)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return write(request, nullptr, 0)
        && read(std::forward<Reader>(reader), duration_cast<milliseconds>(timeout));
}

template <typename Params, typename Reader>
bool Protocol::set(Command cmd, Params &&data, Reader &&reader, ReaderTag)
{
    return set(cmd, &data, sizeof(Params), reader, kDefaultReadTimeout, ReaderTag());
}

template <typename Params, typename Reader, typename T, typename Q>
bool Protocol::set(Command cmd, Params &&data, Reader &&reader, std::chrono::duration<T, Q> timeout, ReaderTag)
{
    return set(cmd, &data, sizeof(Params), reader, timeout, ReaderTag());
}

template <typename Reader>
bool Protocol::set(Command cmd, const void *data, int size, Reader &&reader, ReaderTag)
{
    return set(cmd, data, size, reader, kDefaultReadTimeout, ReaderTag());
}

template <typename Reader, typename T, typename Q>
bool Protocol::set(Command cmd, const void *data, int size, Reader &&reader,
         std::chrono::duration<T, Q> timeout, ReaderTag)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return write(cmd, data, size)
        && read(std::forward(reader), duration_cast<milliseconds>(timeout));
}
