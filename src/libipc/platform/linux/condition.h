#pragma once

#include "libipc/imp/log.h"
#include "libipc/mutex.h"

#include "get_wait_time.h"
#include "sync_obj_impl.h"

#include "a0/err_macro.h"
#include "a0/mtx.h"

namespace ipc {
namespace detail {
namespace sync {

class condition : public sync::obj_impl<a0_cnd_t> {
public:
    condition() = default;
    ~condition() = default;

    bool wait(ipc::sync::mutex &mtx, std::uint64_t tm) noexcept {
        LIBIPC_LOG();
        if (!valid()) return false;
        if (tm == invalid_value) {
            int eno = A0_SYSERR(a0_cnd_wait(native(), static_cast<a0_mtx_t *>(mtx.native())));
            if (eno != 0) {
                log.error("fail condition wait[", eno, "]");
                return false;
            }
        } else {
            auto ts = linux_::detail::make_timespec(tm);
            int eno = A0_SYSERR(a0_cnd_timedwait(native(), static_cast<a0_mtx_t *>(mtx.native()), {ts}));
            if (eno != 0) {
                if (eno != ETIMEDOUT) {
                    log.error("fail condition timedwait[", eno, "]: tm = ", tm, ", tv_sec = ", ts.tv_sec, ", tv_nsec = ", ts.tv_nsec);
                }
                return false;
            }
        }
        return true;
    }

    bool notify(ipc::sync::mutex &mtx) noexcept {
        LIBIPC_LOG();
        if (!valid()) return false;
        int eno = A0_SYSERR(a0_cnd_signal(native(), static_cast<a0_mtx_t *>(mtx.native())));
        if (eno != 0) {
            log.error("fail condition notify[", eno, "]");
            return false;
        }
        return true;
    }

    bool broadcast(ipc::sync::mutex &mtx) noexcept {
        LIBIPC_LOG();
        if (!valid()) return false;
        int eno = A0_SYSERR(a0_cnd_broadcast(native(), static_cast<a0_mtx_t *>(mtx.native())));
        if (eno != 0) {
            log.error("fail condition broadcast[", eno, "]");
            return false;
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
