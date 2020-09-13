#pragma once

#include <atomic>   // std::atomic_thread_fence
#include <cassert>  // assert

#include "libipc/tls_pointer.h"

#include "libipc/platform/tls_detail_win.h"
#include "libipc/utility/utility.h"

namespace ipc {
namespace tls {

bool create(key_info * pkey, destructor_t destructor) {
    assert(pkey != nullptr);
    pkey->key_ = horrible_cast<key_t>(destructor);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return true;
}

void release(key_info const * pkey) {
    assert(pkey != nullptr);
    assert(tls_get_recs() != nullptr);
    tls_get_recs()->erase(pkey);
}

bool set(key_info const * pkey, void * ptr) {
    assert(pkey != nullptr);
    assert(tls_get_recs() != nullptr);
    (*tls_get_recs())[pkey] = ptr;
    return true;
}

void * get(key_info const * pkey) {
    assert(pkey != nullptr);
    assert(tls_get_recs() != nullptr);
    auto const recs = tls_get_recs();
    auto it = recs->find(pkey);
    if (it == recs->end()) {
        return nullptr;
    }
    return it->second;
}

} // namespace tls
} // namespace ipc
