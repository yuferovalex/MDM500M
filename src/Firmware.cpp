#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include "BootLoad.h"
#include "Types.h"
#include "Firmware.h"

using namespace std;

template <typename T1, typename T2>
inline void setIfNotNull(T1 *dst, const T2 &src)
{
    if (dst) {
        *dst = src;
    }
}

inline int my_log2(int x)
{
    int res = 0;
    for (; x > 1; ++res, x >>= 1);
    return res;
}

class FirmwareData : public QSharedData
{
public:
    QString errorString;
    QString filePath;
    int pageSize = 0;
    int pageSizeLog2 = 0;
    SoftwareVersion softVersion;
    vector<QByteArray> pages;
    vector<HardwareVersion> hardCompList;
    vector<SoftwareVersion> softCompList;
};

Firmware::Firmware()
    : d(new FirmwareData)
{}

Firmware::Firmware(QString fileName)
    : d(new FirmwareData)
{
    readFromFile(fileName);
}

Firmware::Firmware(const Firmware &rhs)
    : d(rhs.d)
{}

Firmware &Firmware::operator=(const Firmware &rhs)
{
    if (this != &rhs) {
        d = rhs.d;
    }
    return *this;
}

Firmware::~Firmware()
{}

bool Firmware::isError() const
{
    return !d->errorString.isEmpty();
}

QString Firmware::errorString() const
{
    return d->errorString;
}

void Firmware::clearError()
{
    d->errorString = QString();
}

bool Firmware::checkFile(QString filePath, QString *errorString)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        setIfNotNull(errorString, QObject::tr("Файл не существует"));
        return false;
    }
    if (fileInfo.suffix().toLower() != QLatin1String("bsk")) {
        setIfNotNull(errorString, QObject::tr("Файл не является файлом прошивки"));
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setIfNotNull(errorString, QObject::tr("Невозможно открыть файл: %1").arg(file.errorString()));
        return false;
    }
    if (file.size() < qint64(sizeof(TFileHeader))) {
        qDebug("%s: File size less than size of TFileHeader", "Firmware");
        setIfNotNull(errorString, QObject::tr("Файл не является файлом прошивки"));
        return false;
    }
    // Читаем заголовок
    TFileHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(TFileHeader));
    if (strcmp(header.ID, "BOOT_FILE") != 0) {
        qDebug("%s: TFileHeader::ID != BOOT_FILE", "Firmware");
        setIfNotNull(errorString, QObject::tr("Файл не является файлом прошивки"));
        return false;
    }
    if (header.CListLength > 20) {
        qDebug("%s: TFileHeader::CListLength > 20", "Firmware");
        setIfNotNull(errorString, QObject::tr("Файл не является файлом прошивки"));
        return false;
    }
    const auto theoreticalFileSize = fileSizeByHeader(header);
    if (theoreticalFileSize != fileInfo.size()) {
        qDebug("%s: Wrong file size", "Firmware");
        setIfNotNull(errorString, QObject::tr("Файл не является файлом прошивки"));
        return false;
    }
    return true;
}

bool Firmware::readFromFile(QString filePath)
{
    clear();
    if (!checkFile(filePath, &d->errorString)) {
        // qWarning("%s: Wrong firmware file", "Firmware");
        return false;
    }
    // Считываем заголовок
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    TFileHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(TFileHeader));
    // d->softVersion = header.SoftwareVersion; // Не реализовано в упаковщике, всегда 0
    d->pageSize = header.PageSize;
    uint16_t temp;
    // Считываем список совместимости железа
    d->hardCompList.reserve(header.CListLength);
    for (int i = 0; i < header.CListLength; ++i) {
        file.read(reinterpret_cast<char *>(&temp), sizeof(temp));
        header.HardwareVersion.modification = static_cast<uint8_t>(temp);
        d->hardCompList.push_back(header.HardwareVersion);
    }
    // Считываем список совместимости софта
    d->softCompList.reserve(header.SCListLength);
    for (int i = 0; i < header.SCListLength; ++i) {
        SoftwareVersion temp;
        file.read(reinterpret_cast<char *>(&temp), sizeof(temp));
        d->softVersion = std::max(d->softVersion, temp);
        d->softCompList.push_back(temp);
    }
    // Считываем контрольную сумму, не проверятся (не реализованно в упаковщике)
    file.read(reinterpret_cast<char *>(&temp), sizeof(temp));
    // Считываем страницы
    d->pages.reserve(header.NumPages);
    QByteArray data;
    for (uint page = 0; page < header.NumPages; ++page) {
        data = file.read(header.PageSize);
        Q_ASSERT(data.size() == header.PageSize);
        d->pages.push_back(data);
    }
    d->filePath = QDir::toNativeSeparators(QFileInfo(filePath).absoluteFilePath());
    d->pageSizeLog2 = my_log2(header.PageSize - sizeof(TPageHeader));
    return true;
}

void Firmware::clear()
{
    clearError();
    d->pageSize = 0;
    d->pages.clear();
    d->filePath.clear();
    d->hardCompList.clear();
    d->softCompList.clear();
}

bool Firmware::isCompatible(HardwareVersion hv) const
{
    auto iter = std::find(d->hardCompList.begin(), d->hardCompList.end(), hv);
    return iter != d->hardCompList.end();
}

QByteArray Firmware::page(int id) const
{
    Q_ASSERT(id >= 0 && id < pageCount());
    return d->pages.at(static_cast<size_t>(id));
}

int Firmware::pageCount() const
{
    return static_cast<int>(d->pages.size());
}

int Firmware::pageSize() const
{
    return d->pageSize;
}

SoftwareVersion Firmware::softwareVersion() const
{
    return d->softVersion;
}

QString Firmware::filePath() const
{
    return d->filePath;
}

bool Firmware::isNull() const
{
    return d->pages.empty();
}

int Firmware::pageSizeLog2() const
{
    return d->pageSizeLog2;
}

qint64 Firmware::fileSizeByHeader(const TFileHeader &header)
{
    return  sizeof(TFileHeader) +                      // Заголовок
            header.CListLength * sizeof(uint16_t) +    // Список совместимости железа
            header.SCListLength * sizeof(uint16_t) +   // Список совместимости софта
            sizeof(uint16_t) +                         // Контрольная сумма
            header.NumPages * header.PageSize;         // Страницы с заголовками
}
