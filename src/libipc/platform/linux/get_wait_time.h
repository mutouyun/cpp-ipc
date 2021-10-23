#pragma once

#include <cstdint>
#include <cinttypes>
#include <system_error>

#include "libipc/utility/log.h"

#include "a0/time.h"
#include "a0/err_macro.h"

namespace ipc {
namespace detail {

inline bool calc_wait_time(timespec &ts, std::uint64_t tm /*ms*/) noexcept {
    std::int64_t add_ns = static_cast<std::int64_t>(tm * 1000000ull);
    if (add_ns < 0) {
        ipc::error("invalid time = " PRIu64 "\n", tm);
        return false;
    }
    a0_time_mono_t now;
    int eno = A0_SYSERR(a0_time_mono_now(&now));
    if (eno != 0) {
        ipc::error("fail get time[%d]\n", eno);
        return false;
    }
    a0_time_mono_t *target = reinterpret_cast<a0_time_mono_t *>(&ts);
    if ((eno = A0_SYSERR(a0_time_mono_add(now, add_ns, target))) != 0) {
        ipc::error("fail get time[%d]\n", eno);
        return false;
    }
    return true;
}

inline timespec make_timespec(std::uint64_t tm /*ms*/) noexcept(false) {
    timespec ts {};
    if (!calc_wait_time(ts, tm)) {
        ipc::error("fail calc_wait_time: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                    tm, ts.tv_sec, ts.tv_nsec);
        throw std::system_error{static_cast<int>(errno), std::system_category()};
    }
    return ts;
}

} // namespace detail
} // namespace ipc
