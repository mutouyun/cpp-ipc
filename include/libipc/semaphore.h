#pragma once

#include <cstdint>  // std::uint64_t

#include "libipc/export.h"
#include "libipc/def.h"

namespace ipc {
namespace sync {

class IPC_EXPORT semaphore {
    semaphore(semaphore const &) = delete;
    semaphore &operator=(semaphore const &) = delete;

public:
    semaphore();
    explicit semaphore(char const *name, std::uint32_t count = 0);
    ~semaphore();

    void const *native() const noexcept;
    void *native() noexcept;

    bool valid() const noexcept;

    bool open(char const *name, std::uint32_t count = 0) noexcept;
    void close() noexcept;

    bool wait(std::uint64_t tm = ipc::invalid_value) noexcept;
    bool post(std::uint32_t count = 1) noexcept;

private:
    class semaphore_;
    semaphore_* p_;
};

} // namespace sync
} // namespace ipc
