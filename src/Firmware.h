#pragma once

#include <QSharedDataPointer>

#include "Types.h"

class FirmwareData;

/**
 * @brief Прошивка
 */
class Firmware
{
public:
    Firmware();
    Firmware(QString fileName);
    Firmware(const Firmware &);
    Firmware &operator=(const Firmware &);
    ~Firmware();

    /**
     * @brief Ошибка при чтении файла
     * @return Вернет истину, если прошизошла ошибка при чтении, ложь - иначе.
     */
    bool isError() const;
    /**
     * @brief Описание ошибки
     * @return Возвращает описание произошедшей ошибки
     */
    QString errorString() const;
    /**
     * @brief Сброс ошибки
     */
    void clearError();
    /**
     * @brief Этот метод проверяет, является ли файл файлом прошивки
     * @param[in]  filePath - Путь до файла
     * @param[out] errorString - Описание ошибки
     * @return Вернет истину, если filePath - файл прошивки, ложь - иначе.
     */
    static bool checkFile(QString filePath, QString *errorString = nullptr);
    /**
     * @brief Этот метод загружает данные из файла.
     * @param[in] filePath - Путь до файла
     * @return Вернет истину, если операция успешна, ложь - иначе.
     */
    bool readFromFile(QString filePath);
    /**
     * @brief Очистить от текущих данных.
     */
    void clear();
    /**
     * @brief Этот метод проверяет совместимость прошивки по версии аппаратного обеспечения.
     * @param[in] hv - Версия АО
     */
    bool isCompatible(HardwareVersion hv) const;
    /**
     * @brief Этот метод возвращает страницу по её номеру.
     * @param[in] id - Номер страницы
     */
    QByteArray page(int id) const;
    /**
     * @brief Этот метод возвращает количество страниц загруженного файла
     */
    int pageCount() const;
    /**
     * @brief Этот метод возвращает степень двойки, соответствующую размеру страницы
     */
    int pageSize() const;
    /**
     * @brief Этот метод возвращает версию ПО загруженного файла.
     */
    SoftwareVersion softwareVersion() const;
    /**
     * @brief Этот метод возвращает полный путь до файла прошивки.
     */
    QString filePath() const;
    /**
     * @brief "Проверка на ноль"
     */
    bool isNull() const;
    /**
     * @brief Этот метод возвращает двоичный логарифм размера страницы.
     */
    int pageSizeLog2() const;

private:
    /**
     * @brief Этот метод возвращает теоретический размер файла, исходя из
     * заголовочной информации.
     */
    static qint64 fileSizeByHeader(const class TFileHeader &header);

    QSharedDataPointer<FirmwareData> d; /**< Данные файла */
};
