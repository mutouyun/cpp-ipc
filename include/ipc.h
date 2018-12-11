#pragma once

#include <vector>

#include "export.h"
#include "def.h"
#include "shm.h"

namespace ipc {

using shm::handle_t;

IPC_EXPORT handle_t connect   (char const * name);
IPC_EXPORT void     disconnect(handle_t h);

IPC_EXPORT bool                send(handle_t h, void* data, int size);
IPC_EXPORT std::vector<byte_t> recv(handle_t h);

class IPC_EXPORT channel {
public:
    channel(void);
    channel(char const * name);
    channel(channel&& rhs);

    ~channel(void);

    void swap(channel& rhs);
    channel& operator=(channel rhs);

    bool         valid(void) const;
    char const * name (void) const;

    channel clone(void) const;

    bool connect(char const * name);
    void disconnect(void);

private:
    class channel_;
    channel_* p_;
};

} // namespace ipc
