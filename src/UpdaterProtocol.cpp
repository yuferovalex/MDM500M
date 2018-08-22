#include <QSerialPort>
#include <QThread>

#include "BootLoad.h"
#include "UpdaterProtocol.h"

static constexpr uint8_t marker = 0x55;

struct WriteParametrs
{
    ushort pageId;
    ushort pageSize;
    QByteArray pageData;
};

uint16_t slow_crc16RAM(uint16_t sum, const void *data, uint16_t len)
{
    auto p = reinterpret_cast<const uint8_t *>(data);
    if(!len){
        return sum;
    }
    do {
        uint8_t i;
        uint8_t byte = *p++;
        for (i = 0; i < 8; ++i) {
            WORD osum = sum;
            sum <<= 1;
            if(byte & 0x80){
                sum |= 1 ;
            }
            if(osum & 0x8000){
                sum ^= 0x1021;
            }
            byte <<= 1;
        }
    } while(--len);
    return sum;
}

inline bool checkCrc(const TBootPack &package)
{
    auto begin = reinterpret_cast<const char *>(&package.type_boot);
    auto end   = reinterpret_cast<const char *>(&package.crc);
    auto size  = static_cast<uint16_t>(std::distance(begin, end));
    return slow_crc16RAM(0, begin, size) == package.crc;
}

UpdaterProtocol::UpdaterProtocol(QSerialPort &port)
    : m_port(port)
{

}

bool UpdaterProtocol::configure()
{
    return m_port.setBaudRate(QSerialPort::Baud38400)
        && m_port.setDataBits(QSerialPort::Data8)
        && m_port.setFlowControl(QSerialPort::NoFlowControl)
        && m_port.setParity(QSerialPort::NoParity)
        && m_port.setRequestToSend(true)
        && m_port.setDataTerminalReady(true);
}

bool UpdaterProtocol::wait(size_t size, QDeadlineTimer timer)
{
    while (!timer.hasExpired()) {
        if (static_cast<size_t>(m_port.bytesAvailable()) >= size) {
            return true;
        }
        m_port.waitForReadyRead(static_cast<int>(timer.remainingTime()));
    }
    return false;
}

uint8_t UpdaterProtocol::getChar()
{
    uint8_t retval;
    m_port.getChar(reinterpret_cast<char *>(&retval));
    return retval;
}

bool UpdaterProtocol::readPackage(ETypeBoot &type, int &page, QDeadlineTimer timer)
{
    TBootPack pack;
    auto begin = reinterpret_cast<char *>(&pack) + sizeof(TBootPack::start_byte);
    int  size  = sizeof(TBootPack) - sizeof(TBootPack::start_byte);

    do {
        if (!wait(sizeof(TBootPack), timer)) {
            return false;
        }
        if (getChar() == marker) {
            m_port.read(begin, size);
            if (checkCrc(pack)) {
                type = static_cast<ETypeBoot>(pack.type_boot);
                page = pack.nmb_page;
                return true;
            }
        }
    }
    while (!timer.hasExpired());

    return false;
}

bool UpdaterProtocol::writePackage(int pageNumber, int pageSizeLog, QByteArray pageData)
{
    Q_ASSERT(pageSizeLog > 0 && pageSizeLog < 16);

    // Заполнение заголовка пакета
    TBootHeader header;
    header.start_byte    = marker;
    header.type_boot     = tbWritePage;
    header.nmb_page      = static_cast<uint16_t>(pageNumber);
    header.log_page_size = static_cast<uint16_t>(pageSizeLog);

    // Вычисление контрольной суммы
    uint16_t crc;
    {
        const void *begin = &header.type_boot;
        uint16_t size = sizeof(TBootHeader) - sizeof(TBootHeader::start_byte);
        crc = slow_crc16RAM(0, begin, size);
    }
    {
        const void *begin = pageData.data();
        uint16_t size = static_cast<uint16_t>(pageData.size());
        crc = slow_crc16RAM(crc, begin, size);
    }

    // Запись в порт
    {
        auto begin = reinterpret_cast<const char *>(&header);
        qint64 size = sizeof(TBootHeader);
        m_port.write(begin, size);
    }
    m_port.write(pageData);
    {
        auto begin = reinterpret_cast<const char *>(&crc);
        qint64 size = sizeof(crc);
        m_port.write(begin, size);
    }

    // Ожидание записи
    return m_port.waitForBytesWritten();
}

bool UpdaterProtocol::writePage(int pageNumber, int pageLogSize, QByteArray pageData)
{
    QDeadlineTimer timer(5000);
    forever {
        if (!writePackage(pageNumber, pageLogSize, pageData)) {
            return false;
        }
        ETypeBoot ansType;
        int ansPage;
        if (!readPackage(ansType, ansPage, timer)) {
            return false;
        }
        if (ansType == tbAcknowledg && ansPage == pageNumber) {
            return true;
        }
        if (ansType == tbBootQuery && ansPage == pageNumber) {
            continue ;
        }
        if (ansType == tbError) {
            qWarning("UpdaterProtocol::writePage(): устройство сообщило о фатальной ошибке");
            return false;
        }
    }
}

int UpdaterProtocol::waitForRequest()
{
    QDeadlineTimer timer(5000);
    forever {
        ETypeBoot ansType;
        int ansPage;
        if (!readPackage(ansType, ansPage, timer)) {
            return -1;
        }
        if (ansType == tbBootQuery) {
            return ansPage;
        }
    }
}
