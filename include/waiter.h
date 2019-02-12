#pragma once

#include "export.h"

namespace ipc {

class IPC_EXPORT waiter {
public:
    waiter();
    explicit waiter(char const * name);
    waiter(waiter&& rhs);

    virtual ~waiter();

    void swap(waiter& rhs);
    waiter& operator=(waiter rhs);

    bool         valid() const;
    char const * name () const;

    bool open (char const * name);
    void close();

    bool wait();
    bool wait_if_pred();

    bool notify();
    bool broadcast();

protected:
    virtual bool pred();

private:
    class waiter_;
    waiter_* p_;
};

} // namespace ipc
