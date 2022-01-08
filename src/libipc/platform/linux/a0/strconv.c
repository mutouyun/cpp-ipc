#include "strconv.h"

#include "a0/err.h"

#include <string.h>

static const char DECIMAL_DIGITS[] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

a0_err_t a0_u32_to_str(uint32_t val, char* buf_start, char* buf_end, char** start_ptr) {
  return a0_u64_to_str(val, buf_start, buf_end, start_ptr);
}

a0_err_t a0_u64_to_str(uint64_t val, char* buf_start, char* buf_end, char** start_ptr) {
  uint64_t orig_val = val;
  char* ptr = buf_end;
  while (val >= 10) {
    ptr -= 2;
    memcpy(ptr, &DECIMAL_DIGITS[2 * (val % 100)], sizeof(uint16_t));
    val /= 100;
  }
  if (val) {
    *--ptr = (char)('0' + val);
  }
  memset(buf_start, '0', ptr - buf_start);
  ptr -= (!orig_val);
  if (start_ptr) {
    *start_ptr = ptr;
  }
  return A0_OK;
}

a0_err_t a0_str_to_u32(const char* start, const char* end, uint32_t* out) {
  *out = 0;
  for (const char* ptr = start; ptr < end; ptr++) {
    if (*ptr < '0' || *ptr > '9') {
      return A0_ERR_INVALID_ARG;
    }
    *out *= 10;
    *out += *ptr - '0';
  }
  return A0_OK;
}

a0_err_t a0_str_to_u64(const char* start, const char* end, uint64_t* out) {
  *out = 0;
  for (const char* ptr = start; ptr < end; ptr++) {
    if (*ptr < '0' || *ptr > '9') {
      return A0_ERR_INVALID_ARG;
    }
    *out *= 10;
    *out += *ptr - '0';
  }
  return A0_OK;
}
