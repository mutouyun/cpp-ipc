#include "a0/err.h"
#include "a0/thread_local.h"

#include <errno.h>
#include <string.h>

A0_THREAD_LOCAL int a0_err_syscode;
A0_THREAD_LOCAL char a0_err_msg[1024];

const char* a0_strerror(a0_err_t err) {
  switch (err) {
    case A0_OK: {
      return strerror(0);
    }
    case A0_ERR_SYS: {
      return strerror(a0_err_syscode);
    }
    case A0_ERR_CUSTOM_MSG: {
      return a0_err_msg;
    }
    case A0_ERR_INVALID_ARG: {
      return strerror(EINVAL);
    }
    case A0_ERR_RANGE: {
      return "Index out of bounds";
    }
    case A0_ERR_AGAIN: {
      return "Not available yet";
    }
    case A0_ERR_FRAME_LARGE: {
      return "Frame size too large";
    }
    case A0_ERR_ITER_DONE: {
      return "Done iterating";
    }
    case A0_ERR_NOT_FOUND: {
      return "Not found";
    }
    case A0_ERR_BAD_PATH: {
      return "Invalid path";
    }
    case A0_ERR_BAD_TOPIC: {
      return "Invalid topic name";
    }
    default: {
      break;
    }
  }
  return "";
}
