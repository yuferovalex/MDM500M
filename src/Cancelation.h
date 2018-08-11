#pragma once

#include <atomic>
#include <memory>

class CancelToken
{
public:
    CancelToken(std::shared_ptr<std::atomic_bool> state);
    operator bool() const;
    bool isCancelled() const;
    void throwIfCanceled() const;

private:
    std::shared_ptr<std::atomic_bool> m_isCanceled;
};

class CancellationSource
{
public:
    CancelToken token() const;
    void cancel();

private:
    std::shared_ptr<std::atomic_bool> m_isCanceled =
            std::make_shared<std::atomic_bool>();
};

class CancelledException : public std::exception
{
public:
    CancelledException() noexcept;
    const char *what() const noexcept override;
};
