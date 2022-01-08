#ifndef A0_SRC_CLOCK_H
#define A0_SRC_CLOCK_H

#include "a0/err.h"
#include "a0/inline.h"

#include <stdint.h>
#include <time.h>

#include "err_macro.h"

#ifdef __cplusplus
extern "C" {
#endif

static const int64_t NS_PER_SEC = 1e9;

typedef struct timespec timespec_t;

A0_STATIC_INLINE
a0_err_t a0_clock_now(clockid_t clk, timespec_t* out) {
  A0_RETURN_SYSERR_ON_MINUS_ONE(clock_gettime(clk, out));
  return A0_OK;
}

A0_STATIC_INLINE
a0_err_t a0_clock_add(timespec_t ts, int64_t add_nsec, timespec_t* out) {
  out->tv_sec = ts.tv_sec + add_nsec / NS_PER_SEC;
  out->tv_nsec = ts.tv_nsec + add_nsec % NS_PER_SEC;
  if (out->tv_nsec >= NS_PER_SEC) {
    out->tv_sec++;
    out->tv_nsec -= NS_PER_SEC;
  } else if (out->tv_nsec < 0) {
    out->tv_sec--;
    out->tv_nsec += NS_PER_SEC;
  }

  return A0_OK;
}

A0_STATIC_INLINE
a0_err_t a0_clock_convert(
    clockid_t orig_clk,
    timespec_t orig_ts,
    clockid_t target_clk,
    timespec_t* target_ts) {
  timespec_t orig_now;
  A0_RETURN_ERR_ON_ERR(a0_clock_now(orig_clk, &orig_now));
  timespec_t target_now;
  A0_RETURN_ERR_ON_ERR(a0_clock_now(target_clk, &target_now));

  int64_t add_nsec = (orig_ts.tv_sec - orig_now.tv_sec) * NS_PER_SEC + (orig_ts.tv_nsec - orig_now.tv_nsec);
  return a0_clock_add(target_now, add_nsec, target_ts);
}

#ifdef __cplusplus
}
#endif

#endif  // A0_SRC_CLOCK_H
