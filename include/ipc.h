#pragma once

#include <vector>
#include <string>

#include "export.h"
#include "def.h"
#include "shm.h"

namespace ipc {

using shm::handle_t;

IPC_EXPORT handle_t connect   (char const * name);
IPC_EXPORT void     disconnect(handle_t h);

IPC_EXPORT bool                send(handle_t h, void const * data, std::size_t size);
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

    bool send(void const * data, std::size_t size);
    bool send(std::vector<byte_t> const & buff);
    bool send(std::string const & str);

    std::vector<byte_t> recv();

private:
    class channel_;
    channel_* p_;
};

} // namespace ipc
