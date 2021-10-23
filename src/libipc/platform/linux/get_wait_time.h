#pragma once

#include <cstdint>
#include <system_error>

#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "libipc/utility/log.h"

namespace ipc {
namespace detail {

inline bool calc_wait_time(timespec &ts, std::uint64_t tm /*ms*/) noexcept {
    timeval now;
    int eno = ::gettimeofday(&now, NULL);
    if (eno != 0) {
        ipc::error("fail gettimeofday [%d]\n", eno);
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
        ipc::error("fail calc_wait_time: tm = %zd, tv_sec = %ld, tv_nsec = %ld\n",
                    tm, ts.tv_sec, ts.tv_nsec);
        throw std::system_error{static_cast<int>(errno), std::system_category()};
    }
    return ts;
}

} // namespace detail
} // namespace ipc
