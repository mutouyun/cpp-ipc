#pragma once

#include <cstdint>
#include <system_error>

#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "libipc/imp/log.h"

namespace ipc {
namespace posix_ {
namespace detail {

inline bool calc_wait_time(timespec &ts, std::uint64_t tm /*ms*/) noexcept {
    timeval now;
    int eno = ::gettimeofday(&now, NULL);
    if (eno != 0) {
        log.error("fail gettimeofday [", eno, "]");
        return false;
    }
    ts.tv_nsec = (now.tv_usec + (tm % 1000) * 1000) * 1000;
    ts.tv_sec  =  now.tv_sec  + (tm / 1000) + (ts.tv_nsec / 1000000000l);
    ts.tv_nsec %= 1000000000l;
    return true;
}

inline timespec make_timespec(std::uint64_t tm /*ms*/) noexcept(false) {
    timespec ts {};
    if (!calc_wait_time(ts, tm)) {
        log.error("fail calc_wait_time: tm = ", tm, ", tv_sec = ", ts.tv_sec, ", tv_nsec = ", ts.tv_nsec);
        throw std::system_error{static_cast<int>(errno), std::system_category()};
    }
    return ts;
}

} // namespace detail
} // namespace posix_
} // namespace ipc
