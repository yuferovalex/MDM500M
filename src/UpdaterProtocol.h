#pragma once

#include <chrono>

#include <QByteArray>
#include <QDeadlineTimer>

#include "Types.h"
#include "BootLoad.h"

class QSerialPort;

class UpdaterProtocol
{
public:
    UpdaterProtocol(QSerialPort &device);
    bool configure();
    bool writePage(int pageNumber, int pageLogSize, QByteArray pageData);
    int waitForRequest();

private:
    bool wait(size_t size, QDeadlineTimer timer);
    uint8_t getChar();
    bool readPackage(ETypeBoot &type, int &page, QDeadlineTimer timer);
    bool writePackage(int pageNumber, int pageSizeLog, QByteArray pageData);

    QSerialPort &m_port;
};




