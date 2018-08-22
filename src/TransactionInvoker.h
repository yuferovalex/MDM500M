#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

#include <QSerialPort>

#include "Cancelation.h"

namespace Interfaces {
class Transaction;
} // namespace Interfaces

class TransactionInvoker
{
public:

    TransactionInvoker();
    ~TransactionInvoker();
    void exec(Interfaces::Transaction *transaction);
    void clear();

private:
    std::unique_ptr<Interfaces::Transaction> nextTransaction();
    void loop();

    std::atomic_bool m_exit;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<Interfaces::Transaction *> m_queue;
    CancellationSource m_cancellationSource;
    QThread *m_thread;
};
