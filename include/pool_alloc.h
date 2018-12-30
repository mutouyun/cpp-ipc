#pragma once

#include "export.h"
#include "def.h"

namespace ipc {
namespace mem {

class IPC_EXPORT pool_alloc {
public:
    static void  clear();
    static void* alloc(std::size_t size);
    static void  free(void* p, std::size_t size);
};

} // namespace mem
} // namespace ipc
