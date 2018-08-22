#include <QThread>

#include "Transactions.h"
#include "TransactionInvoker.h"

TransactionInvoker::TransactionInvoker()
{
    m_exit = false;
    m_thread = QThread::create([this] { loop(); });
    m_thread->start();
}

TransactionInvoker::~TransactionInvoker()
{
    m_exit.store(true, std::memory_order_release);
    m_cancellationSource.cancel();
    m_cv.notify_all();
    m_thread->wait();
    clear();
}

void TransactionInvoker::exec(Interfaces::Transaction *transaction)
{
    if (!transaction) return;
    std::unique_lock<std::mutex> lock(m_mutex);
    transaction->moveToThread(m_thread);
    m_queue.push(transaction);
    m_cv.notify_all();
}

void TransactionInvoker::clear()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        delete m_queue.front();
        m_queue.pop();
    }
}

std::unique_ptr<Interfaces::Transaction> TransactionInvoker::nextTransaction()
{
    if (m_exit.load(std::memory_order_acquire)) {
        return nullptr;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        m_cv.wait(lock, [this] {
            return m_exit.load(std::memory_order_acquire) || !m_queue.empty();
        });
        if (m_exit.load(std::memory_order_acquire)) {
            return nullptr;
        }
    }
    auto cmd = m_queue.front();
    m_queue.pop();
    return std::unique_ptr<Interfaces::Transaction>(cmd);
}

void TransactionInvoker::loop()
{
    QSerialPort port;
    while (auto cmd = nextTransaction()) {
        try {
            cmd->exec(port, m_cancellationSource.token());
        }
        catch (CancelledException &) {
        }
        catch (...) {
            std::terminate();
        }
    }
}
