#ifndef A0_SRC_ERR_MACRO_H
#define A0_SRC_ERR_MACRO_H

#include "a0/err.h"
#include "a0/inline.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

A0_STATIC_INLINE
a0_err_t A0_MAKE_SYSERR(int syserr) {
  a0_err_syscode = syserr;
  return A0_ERR_SYS;
}

A0_STATIC_INLINE
int A0_SYSERR(a0_err_t err) {
  return err == A0_ERR_SYS ? a0_err_syscode : 0;
}

A0_STATIC_INLINE_RECURSIVE
a0_err_t A0_MAKE_MSGERR(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (fmt) {
    vsnprintf(a0_err_msg, sizeof(a0_err_msg), fmt, args);  // NOLINT(clang-analyzer-valist.Uninitialized): https://bugs.llvm.org/show_bug.cgi?id=41311
    va_end(args);
    return A0_ERR_CUSTOM_MSG;
  }
  va_end(args);
  return A0_OK;
}

#define A0_RETURN_SYSERR_ON_MINUS_ONE(X) \
  do {                                   \
    if ((X) == -1) {                     \
      a0_err_syscode = errno;            \
      return A0_ERR_SYS;                 \
    }                                    \
  } while (0)

#define A0_RETURN_ERR_ON_ERR(X) \
  do {                          \
    a0_err_t _err = (X);        \
    if (_err) {                 \
      return _err;              \
    }                           \
  } while (0)

#endif  // A0_SRC_ERR_MACRO_H
