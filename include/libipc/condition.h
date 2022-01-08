#pragma once

#include <cstdint>  // std::uint64_t

#include "libipc/export.h"
#include "libipc/def.h"
#include "libipc/mutex.h"

namespace ipc {
namespace sync {

class IPC_EXPORT condition {
    condition(condition const &) = delete;
    condition &operator=(condition const &) = delete;

public:
    condition();
    explicit condition(char const *name);
    ~condition();

    void const *native() const noexcept;
    void *native() noexcept;

    bool valid() const noexcept;

    bool open(char const *name) noexcept;
    void close() noexcept;

    bool wait(ipc::sync::mutex &mtx, std::uint64_t tm = ipc::invalid_value) noexcept;
    bool notify(ipc::sync::mutex &mtx) noexcept;
    bool broadcast(ipc::sync::mutex &mtx) noexcept;

private:
    class condition_;
    condition_* p_;
};

} // namespace sync
} // namespace ipc
