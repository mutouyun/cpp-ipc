#include "a0/inline.h"
#include "a0/thread_local.h"
#include "a0/tid.h"

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

A0_THREAD_LOCAL uint32_t a0_tid_cache = 0;
static pthread_once_t a0_tid_reset_atfork_once;

A0_STATIC_INLINE
void a0_tid_reset() {
  a0_tid_cache = 0;
}

A0_STATIC_INLINE
void a0_tid_reset_atfork() {
  pthread_atfork(NULL, NULL, &a0_tid_reset);
}

uint32_t a0_tid() {
  if (!a0_tid_cache) {
    a0_tid_cache = syscall(SYS_gettid);
    pthread_once(&a0_tid_reset_atfork_once, a0_tid_reset_atfork);
  }
  return a0_tid_cache;
}
