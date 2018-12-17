#pragma once

#include <cstddef>

#include "export.h"

namespace ipc {
namespace shm {

using handle_t = void*;

IPC_EXPORT handle_t acquire(char const * name, std::size_t size);
IPC_EXPORT void     release(handle_t h, std::size_t size);
IPC_EXPORT void*    open   (handle_t h);
IPC_EXPORT void     close  (void* mem);

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

    void* get  ();
    void  close();

private:
    class handle_;
    handle_* p_;
};

} // namespace shm
} // namespace ipc
