#include <QDeadlineTimer>

#include "Protocol.h"

Protocol::Protocol(QSerialPort &port)
    : m_port(port)
{
}

bool Protocol::get(Protocol::Command cmd, void *data, size_t size, std::chrono::milliseconds timeout)
{
    if (!configure()) return false;
    for (int attempt = 0; attempt < attemptsMaxCount; ++attempt) {
        if (write(cmd, nullptr, 0) && read(cmd, data, size, timeout)) {
            return true;
        }
    }
    return false;
}

bool Protocol::set(Protocol::Command cmd, Protocol::Error &error, const void *data, size_t size, std::chrono::milliseconds timeout)
{
    if (!configure()) return false;
    for (int attempt = 0; attempt < attemptsMaxCount; ++attempt) {
        if (write(cmd, data, size) && read(cmd, &error, sizeof(Error), timeout)) {
            return true;
        }
    }
    return false;
}

bool Protocol::configure()
{
    return m_port.setBaudRate(QSerialPort::Baud19200)
            && m_port.setDataBits(QSerialPort::Data8)
            && m_port.setFlowControl(QSerialPort::NoFlowControl)
            && m_port.setParity(QSerialPort::NoParity)
            && m_port.setRequestToSend(true)
            && m_port.setDataTerminalReady(true);
}

uint8_t Protocol::getch()
{
    uint8_t retval;
    m_port.getChar(reinterpret_cast<char *>(&retval));
    return retval;
}

void Protocol::putch(uint8_t ch)
{
    m_port.putChar(static_cast<char>(ch));
}

uint8_t Protocol::calcCrc(Protocol::Command cmd, const void *data, size_t size)
{
    uint8_t sizeField = minSize + static_cast<uint8_t>(size);
    uint8_t headerCrc = sizeField + static_cast<uint8_t>(cmd);
    auto dataBegin = reinterpret_cast<const uint8_t *>(data);
    auto dataEnd   = dataBegin + size;
    return std::accumulate(dataBegin, dataEnd, headerCrc);
}

bool Protocol::wait(size_t size, std::chrono::milliseconds timeout)
{
    QDeadlineTimer timer(timeout);

    while(!timer.hasExpired()) {
        if ((size_t) m_port.bytesAvailable() >= size) {
            return true;
        }
        m_port.waitForReadyRead(timer.remainingTime());
    }
    return false;
}

bool Protocol::read(Protocol::Command cmd, void *data, size_t size, std::chrono::milliseconds timeout)
{
    auto readPackage = [=]
    {
        // Header
        bool res = getch() == marker
                && getch() == size + minSize
                && getch() == (uint8_t) cmd;
        // Body
        if (res) {
            m_port.read(reinterpret_cast<char *>(data), (qint64) size);
            uint8_t expectedCrc = getch();
            uint8_t actualCrc = calcCrc(cmd, data, size);
            res = expectedCrc == actualCrc;
            if (!res) qDebug("Protocol::read(): wrong crc");
        }
        return res;
    };

    do {
        if (!wait(size + minFullSize, timeout)) {
            return false;
        }
    }
    while (!readPackage());
    return true;
}

bool Protocol::write(Protocol::Command cmd, const void *data, size_t size)
{
    putch(marker);
    putch((uint8_t) size + minSize);
    putch((uint8_t) cmd);
    m_port.write((const char *) data, (quint64) size);
    putch(calcCrc(cmd, data, size));
    return m_port.waitForBytesWritten(writeTimeout.count());
}
