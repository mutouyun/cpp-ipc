
#include <thread>
#include <iostream>

#include "test.h"

#if defined(__linux__) || defined(__linux)
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
    int r = pthread_mutex_timedlock(&mutex, &tout);
    EXPECT_EQ(r, EOWNERDEAD);

    pthread_mutex_consistent(&mutex);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
}
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
      defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>
#include <tchar.h>

TEST(PThread, Robust) {
    HANDLE lock = CreateMutex(NULL, FALSE, _T("test-robust"));
    std::thread{[] {
        HANDLE lock = CreateMutex(NULL, FALSE, _T("test-robust"));
        WaitForSingleObject(lock, 0);
    }}.join();

    DWORD r = WaitForSingleObject(lock, 0);
    EXPECT_EQ(r, WAIT_ABANDONED);

    CloseHandle(lock);
}
#endif // !__linux__