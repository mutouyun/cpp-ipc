#include "tls_pointer.h"

#include <pthread.h>    // pthread_...

#include <atomic>       // std::atomic_thread_fence
#include <cassert>      // assert

#include "log.h"
#include "utility.h"

namespace ipc {
namespace tls {

namespace {
namespace native {

using key_t = pthread_key_t;

bool create(key_t * pkey, void (*destructor)(void*)) {
    assert(pkey != nullptr);
    int err = ::pthread_key_create(pkey, destructor);
    if (err != 0) {
        ipc::error("[native::create] pthread_key_create failed [%d].\n", err);
        return false;
    }
    return true;
}

bool release(key_t key) {
    int err = ::pthread_key_delete(key);
    if (err != 0) {
        ipc::error("[native::release] pthread_key_delete failed [%d].\n", err);
        return false;
    }
    return true;
}

bool set(key_t key, void * ptr) {
    int err = ::pthread_setspecific(key, ptr);
    if (err != 0) {
        ipc::error("[native::set] pthread_setspecific failed [%d].\n", err);
        return false;
    }
    return true;
}

void * get(key_t key) {
    return ::pthread_getspecific(key);
}

} // namespace native
} // internal-linkage

bool create(key_info * pkey, destructor_t destructor) {
    assert(pkey != nullptr);
    native::key_t k;
    if (!native::create(&k, destructor)) {
        return false;
    }
    pkey->key_ = horrible_cast<key_t>(k);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

void release(key_info const * pkey) {
    assert(pkey != nullptr);
    static_cast<void>(
        native::release(horrible_cast<native::key_t>(pkey->key_)));
}

bool set(key_info const * pkey, void* ptr) {
    assert(pkey != nullptr);
    return native::set(horrible_cast<native::key_t>(pkey->key_), ptr);
}

void * get(key_info const * pkey) {
    assert(pkey != nullptr);
    return native::get(horrible_cast<native::key_t>(pkey->key_));
}

} // namespace tls
} // namespace ipc
