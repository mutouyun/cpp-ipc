#pragma once

#include "libipc/utility/log.h"
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
        if (!valid()) return false;
        if (tm == invalid_value) {
            int eno = A0_SYSERR(a0_cnd_wait(native(), static_cast<a0_mtx_t *>(mtx.native())));
            if (eno != 0) {
                ipc::error("fail condition wait[%d]\n", eno);
                return false;
            }
        } else {
            auto ts = detail::make_timespec(tm);
            int eno = A0_SYSERR(a0_cnd_timedwait(native(), static_cast<a0_mtx_t *>(mtx.native()), {ts}));
            if (eno != 0) {
                if (eno != ETIMEDOUT) {
                    ipc::error("fail condition timedwait[%d]: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                                eno, tm, ts.tv_sec, ts.tv_nsec);
                }
                return false;
            }
        }
        return true;
    }

    bool notify(ipc::sync::mutex &mtx) noexcept {
        if (!valid()) return false;
        int eno = A0_SYSERR(a0_cnd_signal(native(), static_cast<a0_mtx_t *>(mtx.native())));
        if (eno != 0) {
            ipc::error("fail condition notify[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool broadcast(ipc::sync::mutex &mtx) noexcept {
        if (!valid()) return false;
        int eno = A0_SYSERR(a0_cnd_broadcast(native(), static_cast<a0_mtx_t *>(mtx.native())));
        if (eno != 0) {
            ipc::error("fail condition broadcast[%d]\n", eno);
            return false;
        }
        return true;
    }
};

} // namespace sync
} // namespace detail
} // namespace ipc
