#pragma once

#include <vector>

#include "export.h"
#include "def.h"
#include "shm.h"

namespace ipc {

using shm::handle_t;

IPC_EXPORT handle_t connect   (std::string const & name);
IPC_EXPORT void     disconnect(handle_t h);

IPC_EXPORT bool                send(handle_t h, byte_t* data, int size);
IPC_EXPORT std::vector<byte_t> recv(handle_t h);

} // namespace ipc
