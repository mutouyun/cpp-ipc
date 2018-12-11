#pragma once

#include <vector>

#include "export.h"
#include "def.h"
#include "shm.h"

namespace ipc {

using shm::handle_t;

IPC_EXPORT handle_t connect   (std::string const & name);
IPC_EXPORT void     disconnect(handle_t h);

IPC_EXPORT bool                send(handle_t h, void* data, int size);
IPC_EXPORT std::vector<byte_t> recv(handle_t h);

class IPC_EXPORT channel {
public:
    channel(void);
    channel(std::string const & name);
    channel(channel&& rhs);

    ~channel(void);

    void swap(channel& rhs);
    channel& operator=(channel rhs);

private:
    class channel_;
    channel_* p_;
};

} // namespace ipc
