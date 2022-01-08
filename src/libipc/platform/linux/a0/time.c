#include "a0/empty.h"
#include "a0/err.h"
#include "a0/time.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "clock.h"
#include "err_macro.h"
#include "strconv.h"

const char A0_TIME_MONO[] = "a0_time_mono";

a0_err_t a0_time_mono_now(a0_time_mono_t* out) {
  return a0_clock_now(CLOCK_BOOTTIME, &out->ts);
}

a0_err_t a0_time_mono_str(a0_time_mono_t time_mono, char mono_str[20]) {
  // Mono time as unsigned integer with up to 20 chars: "18446744072709551615"
  uint64_t ns = time_mono.ts.tv_sec * NS_PER_SEC + time_mono.ts.tv_nsec;
  mono_str[19] = '\0';
  return a0_u64_to_str(ns, mono_str, mono_str + 19, NULL);
}

a0_err_t a0_time_mono_parse(const char mono_str[20], a0_time_mono_t* out) {
  uint64_t ns;
  A0_RETURN_ERR_ON_ERR(a0_str_to_u64(mono_str, mono_str + 19, &ns));
  out->ts.tv_sec = ns / NS_PER_SEC;
  out->ts.tv_nsec = ns % NS_PER_SEC;
  return A0_OK;
}

a0_err_t a0_time_mono_add(a0_time_mono_t time_mono, int64_t add_nsec, a0_time_mono_t* out) {
  return a0_clock_add(time_mono.ts, add_nsec, &out->ts);
}

const char A0_TIME_WALL[] = "a0_time_wall";

a0_err_t a0_time_wall_now(a0_time_wall_t* out) {
  A0_RETURN_SYSERR_ON_MINUS_ONE(clock_gettime(CLOCK_REALTIME, &out->ts));
  return A0_OK;
}

a0_err_t a0_time_wall_str(a0_time_wall_t wall_time, char wall_str[36]) {
  // Wall time in RFC 3999 Nano: "2006-01-02T15:04:05.999999999-07:00"
  struct tm wall_tm;
  gmtime_r(&wall_time.ts.tv_sec, &wall_tm);

  strftime(&wall_str[0], 20, "%Y-%m-%dT%H:%M:%S", &wall_tm);
  snprintf(&wall_str[19], 17, ".%09ld-00:00", wall_time.ts.tv_nsec);
  wall_str[35] = '\0';

  return A0_OK;
}

a0_err_t a0_time_wall_parse(const char wall_str[36], a0_time_wall_t* out) {
  // strptime requires _GNU_SOURCE, which we don't want, so we do it our selves.
  // Hard code "%Y-%m-%dT%H:%M:%S" + ".%09ld-00:00" pattern.

  struct tm wall_tm = A0_EMPTY;
  // %Y
  A0_RETURN_ERR_ON_ERR(a0_str_to_u32(wall_str + 0, wall_str + 4, (uint32_t*)&wall_tm.tm_year));
  wall_tm.tm_year -= 1900;
  // -
  if (wall_str[4] != '-') {
    return A0_ERR_INVALID_ARG;
  }
  // %m
  A0_RETURN_ERR_ON_ERR(a0_str_to_u32(wall_str + 5, wall_str + 7, (uint32_t*)&wall_tm.tm_mon));
  if (wall_tm.tm_mon < 1 || wall_tm.tm_mon > 12) {
    return A0_ERR_INVALID_ARG;
  }
  wall_tm.tm_mon--;
  // -
  if (wall_str[7] != '-') {
    return A0_ERR_INVALID_ARG;
  }
  // %d
  A0_RETURN_ERR_ON_ERR(a0_str_to_u32(wall_str + 8, wall_str + 10, (uint32_t*)&wall_tm.tm_mday));
  if (wall_tm.tm_mday < 1 || wall_tm.tm_mday > 31) {
    return A0_ERR_INVALID_ARG;
  }
  // T
  if (wall_str[10] != 'T') {
    return A0_ERR_INVALID_ARG;
  }
  // %H
  A0_RETURN_ERR_ON_ERR(a0_str_to_u32(wall_str + 11, wall_str + 13, (uint32_t*)&wall_tm.tm_hour));
  if (wall_tm.tm_hour > 24) {
    return A0_ERR_INVALID_ARG;
  }
  // :
  if (wall_str[13] != ':') {
    return A0_ERR_INVALID_ARG;
  }
  // %M
  A0_RETURN_ERR_ON_ERR(a0_str_to_u32(wall_str + 14, wall_str + 16, (uint32_t*)&wall_tm.tm_min));
  if (wall_tm.tm_min > 60) {
    return A0_ERR_INVALID_ARG;
  }
  // :
  if (wall_str[16] != ':') {
    return A0_ERR_INVALID_ARG;
  }
  // %S
  A0_RETURN_ERR_ON_ERR(a0_str_to_u32(wall_str + 17, wall_str + 19, (uint32_t*)&wall_tm.tm_sec));
  if (wall_tm.tm_sec > 61) {
    return A0_ERR_INVALID_ARG;
  }
  // .
  if (wall_str[19] != '.') {
    return A0_ERR_INVALID_ARG;
  }

  if (memcmp(wall_str + 29, "-00:00", 6) != 0) {
    return A0_ERR_INVALID_ARG;
  }

  // Use timegm, cause it's a pain to compute months/years to seconds.
  out->ts.tv_sec = timegm(&wall_tm);
  return a0_str_to_u64(wall_str + 20, wall_str + 29, (uint64_t*)&out->ts.tv_nsec);
}
