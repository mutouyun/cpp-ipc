#pragma once

#include "libipc/utility/log.h"
#include "libipc/shm.h"

#include "a0/empty.h"

namespace ipc {
namespace detail {
namespace sync {

template <typename SyncT>
class obj_impl {
public:
    using sync_t = SyncT;

protected:
    ipc::shm::handle shm_;
    sync_t *h_ = nullptr;

    sync_t *acquire_handle(char const *name) {
        if (!shm_.acquire(name, sizeof(sync_t))) {
            ipc::error("[acquire_handle] fail shm.acquire: %s\n", name);
            return nullptr;
        }
        return static_cast<sync_t *>(shm_.get());
    }

public:
    obj_impl() = default;
    ~obj_impl() = default;

    sync_t const *native() const noexcept {
        return h_;
    }

    sync_t *native() noexcept {
        return h_;
    }

    char const *name() const noexcept {
        return shm_.name();
    }

    bool valid() const noexcept {
        return h_ != nullptr;
    }

    bool open(char const *name) noexcept {
        close();
        if ((h_ = acquire_handle(name)) == nullptr) {
            return false;
        }
        if (shm_.ref() > 1) {
            return true;
        }
        *h_ = A0_EMPTY;
        return true;
    }

    void close() noexcept {
        shm_.release();
        h_ = nullptr;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
