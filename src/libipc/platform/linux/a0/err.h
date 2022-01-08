#ifndef A0_ERR_H
#define A0_ERR_H

#include "a0/thread_local.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum a0_err_e {
  A0_OK = 0,
  A0_ERR_SYS = 1,
  A0_ERR_CUSTOM_MSG = 2,
  A0_ERR_INVALID_ARG = 3,
  A0_ERR_RANGE = 4,
  A0_ERR_AGAIN = 5,
  A0_ERR_ITER_DONE = 6,
  A0_ERR_NOT_FOUND = 7,
  A0_ERR_FRAME_LARGE = 8,
  A0_ERR_BAD_PATH = 9,
  A0_ERR_BAD_TOPIC = 10,
} a0_err_t;

extern A0_THREAD_LOCAL int a0_err_syscode;
extern A0_THREAD_LOCAL char a0_err_msg[1024];

const char* a0_strerror(a0_err_t);

#ifdef __cplusplus
}
#endif

#endif  // A0_ERR_H
