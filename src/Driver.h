#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

#include <QSerialPort>

#include "Cancelation.h"

class Command;

class Driver
{
public:
    Driver();
    ~Driver();
    void exec(Command *cmd);
    void clear();

private:
    std::unique_ptr<Command> nextCmd();
    void loop();

    std::atomic_bool m_exit;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<Command *> m_queue;
    CancellationSource m_cancellationSource;
    QThread *m_thread;
};
