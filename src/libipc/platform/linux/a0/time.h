/**
 * \file time.h
 * \rst
 *
 * Mono Time
 * ---------
 *
 * | Mono time is a number of nanoseconds from machine boottime.
 * | This time cannot decrease and duration between ticks is constant.
 * | This time is not related to wall clock time.
 * | This time is most suitable for measuring durations.
 * |
 * | As a string, it is represented as a 20 char number:
 * | **18446744072709551615**
 * |
 * | Note that this uses CLOCK_BOOTTIME under the hood, not CLOCK_MONOTONIC.
 *
 * Wall Time
 * ---------
 *
 * | Wall time is an time object representing human-readable wall clock time.
 * | This time can decrease and duration between ticks is not constant.
 * | This time is most related to wall clock time.
 * | This time is not suitable for measuring durations.
 * |
 * | As a string, it is represented as a 36 char RFC 3999 Nano / ISO 8601:
 * | **2006-01-02T15:04:05.999999999-07:00**
 *
 * \endrst
 */

#ifndef A0_TIME_H
#define A0_TIME_H

#include "a0/err.h"

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup TIME_MONO
 *  @{
 */

/// Header key for mono timestamps.
extern const char A0_TIME_MONO[];

/// Monotonic timestamp. Despite the name, uses CLOCK_BOOTTIME.
typedef struct a0_time_mono_s {
  struct timespec ts;
} a0_time_mono_t;

/// Get the current mono timestamps.
a0_err_t a0_time_mono_now(a0_time_mono_t*);

/// Stringify a given mono timestamps.
a0_err_t a0_time_mono_str(a0_time_mono_t, char mono_str[20]);

/// Parse a stringified mono timestamps.
a0_err_t a0_time_mono_parse(const char mono_str[20], a0_time_mono_t*);

/// Add a duration in nanoseconds to a mono timestamp.
a0_err_t a0_time_mono_add(a0_time_mono_t, int64_t add_nsec, a0_time_mono_t*);

/** @}*/

/** \addtogroup TIME_WALL
 *  @{
 */

/// Header key for wall timestamps.
extern const char A0_TIME_WALL[];

/// Wall clock timestamp.
typedef struct a0_time_wall_s {
  struct timespec ts;
} a0_time_wall_t;

/// Get the current wall timestamps.
a0_err_t a0_time_wall_now(a0_time_wall_t*);

/// Stringify a given wall timestamps.
a0_err_t a0_time_wall_str(a0_time_wall_t, char wall_str[36]);

/// Parse a stringified wall timestamps.
a0_err_t a0_time_wall_parse(const char wall_str[36], a0_time_wall_t*);

/** @}*/

#ifdef __cplusplus
}
#endif

#endif  // A0_TRANSPORT_H
