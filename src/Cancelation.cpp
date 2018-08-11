#include "Cancelation.h"

CancelToken::CancelToken(std::shared_ptr<std::atomic_bool> state)
    : m_isCanceled(state)
{}

CancelToken::operator bool() const
{
    return isCancelled();
}

bool CancelToken::isCancelled() const
{
    return m_isCanceled->load(std::memory_order_relaxed);
}

void CancelToken::throwIfCanceled() const
{
    if (isCancelled()) {
        throw CancelledException();
    }
}

CancelToken CancellationSource::token() const
{
    return CancelToken(m_isCanceled);
}

void CancellationSource::cancel()
{
    m_isCanceled->store(true, std::memory_order_relaxed);
}

CancelledException::CancelledException() noexcept
{
}

const char *CancelledException::what() const noexcept
{
    return "operation cancelled";
}
