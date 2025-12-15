#pragma once

#include <cstdint>
#include <cinttypes>
#include <system_error>

#include "libipc/imp/log.h"

#include "a0/time.h"
#include "a0/err_macro.h"

namespace ipc {
namespace linux_ {
namespace detail {

inline bool calc_wait_time(timespec &ts, std::uint64_t tm /*ms*/) noexcept {
    LIBIPC_LOG();
    std::int64_t add_ns = static_cast<std::int64_t>(tm * 1000000ull);
    if (add_ns < 0) {
        log.error("invalid time = ", tm);
        return false;
    }
    a0_time_mono_t now;
    int eno = A0_SYSERR(a0_time_mono_now(&now));
    if (eno != 0) {
        log.error("fail get time[", eno, "]");
        return false;
    }
    a0_time_mono_t *target = reinterpret_cast<a0_time_mono_t *>(&ts);
    if ((eno = A0_SYSERR(a0_time_mono_add(now, add_ns, target))) != 0) {
        log.error("fail get time[", eno, "]");
        return false;
    }
    return true;
}

inline timespec make_timespec(std::uint64_t tm /*ms*/) noexcept(false) {
    LIBIPC_LOG();
    timespec ts {};
    if (!calc_wait_time(ts, tm)) {
        log.error("fail calc_wait_time: tm = ", tm, ", tv_sec = ", ts.tv_sec, ", tv_nsec = ", ts.tv_nsec);
        throw std::system_error{static_cast<int>(errno), std::system_category()};
    }
    return ts;
}

} // namespace detail
} // namespace linux_
} // namespace ipc
