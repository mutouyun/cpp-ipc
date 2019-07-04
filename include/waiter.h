#pragma once

#include "export.h"
#include "def.h"

namespace ipc {

class condition;
class IPC_EXPORT mutex {
public:
    mutex();
    explicit mutex(char const * name);
    mutex(mutex&& rhs);

    ~mutex();

    static void remove(char const * name);

    void swap(mutex& rhs);
    mutex& operator=(mutex rhs);

    bool         valid() const;
    char const * name () const;

    bool open (char const * name);
    void close();

    bool lock  ();
    bool unlock();

private:
    class mutex_;
    mutex_* p_;

    friend class condition;
};

class IPC_EXPORT semaphore {
public:
    semaphore();
    explicit semaphore(char const * name);
    semaphore(semaphore&& rhs);

    ~semaphore();

    static void remove(char const * name);

    void swap(semaphore& rhs);
    semaphore& operator=(semaphore rhs);

    bool         valid() const;
    char const * name () const;

    bool open (char const * name, long count = 0);
    void close();

    bool wait(std::size_t tm = invalid_value);
    bool post(long count = 1);

private:
    class semaphore_;
    semaphore_* p_;
};

class IPC_EXPORT condition {
public:
    condition();
    explicit condition(char const * name);
    condition(condition&& rhs);

    ~condition();

    static void remove(char const * name);

    void swap(condition& rhs);
    condition& operator=(condition rhs);

    bool         valid() const;
    char const * name () const;

    bool open (char const * name);
    void close();

    bool wait(mutex&, std::size_t tm = invalid_value);
    bool notify();
    bool broadcast();

private:
    class condition_;
    condition_* p_;
};

} // namespace ipc
