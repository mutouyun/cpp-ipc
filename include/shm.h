#pragma once

#include <cstddef>

#include "export.h"

namespace ipc {
namespace shm {

IPC_EXPORT void* acquire(char const * name, std::size_t size);
IPC_EXPORT void  release(void* mem, std::size_t size);

class IPC_EXPORT handle {
public:
    handle();
    handle(char const * name, std::size_t size);
    handle(handle&& rhs);

    ~handle();

    void swap(handle& rhs);
    handle& operator=(handle rhs);

    bool         valid() const;
    std::size_t  size () const;
    char const * name () const;

    bool acquire(char const * name, std::size_t size);
    void release();

    void* get() const;

private:
    class handle_;
    handle_* p_;
};

} // namespace shm
} // namespace ipc
