#pragma once

#include <string>
#include <cstddef>

#include "export.h"

namespace ipc {
namespace shm {

using handle_t = void*;

IPC_EXPORT handle_t acquire(std::string const & name, std::size_t size);
IPC_EXPORT void     release(handle_t h, std::size_t size);
IPC_EXPORT void*    open   (handle_t h);
IPC_EXPORT void     close  (void* mem);

class IPC_EXPORT handle {
public:
    handle(void);
    handle(std::string const & name, std::size_t size);
    handle(handle&& rhs);

    ~handle(void);

    void swap(handle& rhs);
    handle& operator=(handle rhs);

    bool valid(void) const;
    std::size_t size(void) const;
    std::string const & name(void) const;

    bool acquire(std::string const & name, std::size_t size);
    void release(void);

    void* get  (void);
    void  close(void);

private:
    class handle_;
    handle_* p_;
};

} // namespace shm
} // namespace ipc
