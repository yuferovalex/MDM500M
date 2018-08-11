#include <QThread>

#include "Commands.h"
#include "Driver.h"

Driver::Driver()
{
    m_exit = false;
    m_thread = QThread::create([this] { loop(); });
    m_thread->start();
}

Driver::~Driver()
{
    m_exit.store(true, std::memory_order_relaxed);
    m_cancellationSource.cancel();
    m_cv.notify_all();
    m_thread->wait();
    clear();
}

void Driver::exec(Command *cmd)
{
    if (!cmd) return;
    std::unique_lock<std::mutex> lock(m_mutex);
    cmd->moveToThread(m_thread);
    m_queue.push(cmd);
    m_cv.notify_all();
}

void Driver::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        delete m_queue.front();
        m_queue.pop();
    }
}

Command *Driver::nextCmd()
{
    if (m_exit.load(std::memory_order_relaxed)) {
        return nullptr;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        m_cv.wait(lock, [this] {
            return m_exit.load(std::memory_order_relaxed) || !m_queue.empty();
        });
        if (m_exit.load(std::memory_order_relaxed)) {
            return nullptr;
        }
    }
    auto cmd = m_queue.front();
    m_queue.pop();
    return cmd;
}

void Driver::loop()
{
    QSerialPort port;
    while (auto cmd = nextCmd()) {
        try {
            m_cancellationSource = CancellationSource();
            cmd->exec(port, m_cancellationSource.token());
        }
        catch (CancelledException &) {
        }
        catch (...) {
            std::terminate();
        }
        delete cmd;
    }
}
