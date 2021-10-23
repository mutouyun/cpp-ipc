#ifndef A0_THREAD_LOCAL_H
#define A0_THREAD_LOCAL_H

#ifdef __cplusplus
#define A0_THREAD_LOCAL thread_local
#else
#define A0_THREAD_LOCAL _Thread_local
#endif  // __cplusplus

#endif  // A0_THREAD_LOCAL_H
