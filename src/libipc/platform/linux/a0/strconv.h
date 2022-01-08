#ifndef A0_SRC_STRCONV_H
#define A0_SRC_STRCONV_H

#include "a0/err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Converts a uint32 or uint64 to string.
// The entire buffer will be populated, if not with given the value, then with '0'.
// Populates the buffer from the back.
// start_ptr, if not null, will be set to the point within the buffer where the number starts.
// Does NOT check for overflow.
a0_err_t a0_u32_to_str(uint32_t val, char* buf_start, char* buf_end, char** start_ptr);
a0_err_t a0_u64_to_str(uint64_t val, char* buf_start, char* buf_end, char** start_ptr);

// Converts a string to uint32 or uint64.
// The string may have leading '0's.
// Returns A0_ERR_INVALID_ARG if any character is not a digit.
// Does NOT check for overflow.
a0_err_t a0_str_to_u32(const char* start, const char* end, uint32_t* out);
a0_err_t a0_str_to_u64(const char* start, const char* end, uint64_t* out);

#ifdef __cplusplus
}
#endif

#endif  // A0_SRC_STRCONV_H
