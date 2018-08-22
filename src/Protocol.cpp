#include "Protocol.h"

Protocol::Protocol(QSerialPort &port)
    : m_port(port)
{}

bool Protocol::configure()
{
    return m_port.setBaudRate(QSerialPort::Baud19200)
        && m_port.setDataBits(QSerialPort::Data8)
        && m_port.setFlowControl(QSerialPort::NoFlowControl)
        && m_port.setParity(QSerialPort::NoParity)
        && m_port.setRequestToSend(true)
        && m_port.setDataTerminalReady(true);
}

uint8_t Protocol::calcCrc(Protocol::Command cmd, const void *data, int size)
{
    uint8_t sizeField = kMinValueOfSizeField + static_cast<uint8_t>(size);
    uint8_t headerCrc = sizeField + static_cast<uint8_t>(cmd);
    auto dataBegin = reinterpret_cast<const uint8_t *>(data);
    auto dataEnd   = dataBegin + size;
    return std::accumulate(dataBegin, dataEnd, headerCrc);
}

bool Protocol::get(Command request, void *readBuffer, int readBufferSize, std::chrono::milliseconds timeout)
{
    return performDefaultDialog(request, nullptr, 0, readBuffer, readBufferSize, timeout);
}

bool Protocol::set(Command cmd, Error &error, const void *cmdParams, int cmdParamsSize, std::chrono::milliseconds timeout)
{
    return performDefaultDialog(cmd, cmdParams, cmdParamsSize, &error, sizeof(error), timeout);
}

bool Protocol::write(Command cmd, const void *data, int size)
{
    m_port.putChar(static_cast<char>(kPackageMarker             ));
    m_port.putChar(static_cast<char>(size + kMinValueOfSizeField));
    m_port.putChar(static_cast<char>(cmd                        ));
    m_port.write(reinterpret_cast<const char *>(data), size);
    m_port.putChar(static_cast<char>(calcCrc(cmd, data, size)));
    return m_port.waitForBytesWritten(kWriteTimeout.count());
}

bool Protocol::performDefaultDialog(Command cmd, const void *writeBuffer, int writeBufferSize, void *readBuffer, int readBufferSize, std::chrono::milliseconds timeout)
{
    for (int attempt = 0; attempt < kMaxAttemptsCount; ++attempt) {
        if (!write(cmd, writeBuffer, writeBufferSize)) {
            return false;
        }
        if (read(DefaultReader(cmd, readBuffer, readBufferSize), timeout)) {
            return true;
        }
    }
    return false;
}

DefaultReader::DefaultReader(Protocol::Command cmd, void *buffer, int size)
    : m_buffer(buffer)
    , m_size(size)
    , m_cmd(cmd)
{}

bool DefaultReader::isDataEnough(int size) const
{
    return size >= m_size;
}

bool DefaultReader::checkPackage(Protocol::Command cmd, const void *data, int size)
{
    if (m_cmd != cmd || m_size != size) {
        return false;
    }
    memcpy(m_buffer, data, size);
    return true;
}

