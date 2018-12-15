#pragma once

#include <utility>   // std::forward

#if defined(WINCE) || defined(_WIN32_WCE) || \
    defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h> // DWORD, ::Tls...
#define IPC_OS_WIN_
#else
#include <pthread.h> // pthread_...
#endif

namespace ipc {

#if defined(IPC_OS_WIN_)

#define IPC_THREAD_LOCAL_KEY_          DWORD
#define IPC_THREAD_LOCAL_SET(KEY, PTR) (::TlsSetValue(KEY, (LPVOID)PTR) == TRUE)
#define IPC_THREAD_LOCAL_GET(KEY)      (::TlsGetValue(KEY))

void thread_local_create(IPC_THREAD_LOCAL_KEY_& key, void (*destructor)(void*));
void thread_local_delete(IPC_THREAD_LOCAL_KEY_ key);

#define IPC_THREAD_LOCAL_CREATE(KEY, DESTRUCTOR) thread_local_create(KEY, DESTRUCTOR)
#define IPC_THREAD_LOCAL_DELETE(KEY)             thread_local_delete(KEY)

#else /*!IPC_OS_WIN_*/

#define IPC_THREAD_LOCAL_KEY_                    pthread_key_t
#define IPC_THREAD_LOCAL_CREATE(KEY, DESTRUCTOR) pthread_key_create(&KEY, DESTRUCTOR)
#define IPC_THREAD_LOCAL_DELETE(KEY)             pthread_key_delete(KEY)
#define IPC_THREAD_LOCAL_SET(KEY, PTR)           (pthread_setspecific(KEY, (void*)PTR) == 0)
#define IPC_THREAD_LOCAL_GET(KEY)                pthread_getspecific(KEY)

#endif/*!IPC_OS_WIN_*/

////////////////////////////////////////////////////////////////
/// Thread-local pointer
////////////////////////////////////////////////////////////////

/*
    <Remarks>

 1. In Windows, if you do not compile thread_local_ptr.cpp,
    use thread_local_ptr will cause memory leaks.

 2. You need to set the thread_local_ptr's storage manually:
    ```
        thread_local_ptr<int> p;
        if (!p) p = new int(123);
    ```
    Just like an ordinary pointer. Or you could just call create:
    ```
        thread_local_ptr<int> p;
        p.create(123);
    ```
*/

template <typename T>
class thread_local_ptr {
    IPC_THREAD_LOCAL_KEY_ key_;

public:
    using value_type = T;

    thread_local_ptr() {
        IPC_THREAD_LOCAL_CREATE(key_, [](void* p) {
            delete static_cast<T*>(p);
        });
    }

    ~thread_local_ptr() {
        IPC_THREAD_LOCAL_DELETE(key_);
    }

    template <typename... P>
    T* create(P&&... params) {
        auto ptr = static_cast<T*>(*this);
        if (ptr == nullptr) {
            return (*this) = new T(std::forward<P>(params)...);
        }
        return ptr;
    }

    T* operator=(T* ptr) {
        IPC_THREAD_LOCAL_SET(key_, ptr);
        return ptr;
    }

    operator T*() const { return static_cast<T*>(IPC_THREAD_LOCAL_GET(key_)); }

    T&       operator* ()       { return *static_cast<T*>(*this); }
    const T& operator* () const { return *static_cast<T*>(*this); }

    T*       operator->()       { return  static_cast<T*>(*this); }
    const T* operator->() const { return  static_cast<T*>(*this); }
};

} // namespace ipc
