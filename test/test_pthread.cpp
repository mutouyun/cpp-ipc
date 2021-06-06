
#include <thread>

#include "test.h"

#ifdef __linux__
#include <pthread.h>
#include <time.h>

TEST(PThread, Robust) {
    pthread_mutexattr_t ma;
    pthread_mutexattr_init(&ma);
    pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&mutex, &ma);

    std::thread{[&mutex] {
        pthread_mutex_lock(&mutex);
        // pthread_mutex_unlock(&mutex);
    }}.join();

    struct timespec tout;
    clock_gettime(CLOCK_REALTIME, &tout);
    struct tm *tmp = localtime(&tout.tv_sec);
    tout.tv_sec += 1;   /* 1 seconds from now */
    int r = pthread_mutex_timedlock(&mutex, &tout);
    EXPECT_EQ(r, EOWNERDEAD);

    pthread_mutex_consistent(&mutex);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
}
#endif // __linux__