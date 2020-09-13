#pragma once

#include <unordered_map>    // std::unordered_map
#include <atomic>           // std::atomic_thread_fence
#include <cassert>          // assert

#include "log.h"
#include "utility.h"

namespace ipc {
namespace tls {

namespace {

inline void tls_destruct(key_info const * pkey, void * p) {
    assert(pkey != nullptr);
    auto destructor = horrible_cast<destructor_t>(pkey->key_);
    if (destructor != nullptr) destructor(p);
}

struct tls_recs : public std::unordered_map<key_info const *, void *> {
    ~tls_recs() {
        for (auto & pair : *this) {
            tls_destruct(pair.first, pair.second);
        }
    }
};

inline tls_recs * tls_get_recs() {
    thread_local tls_recs * recs_ptr = nullptr;
    if (recs_ptr == nullptr) {
        recs_ptr = new tls_recs;
    }
    assert(recs_ptr != nullptr);
    return recs_ptr;
}

} // internal-linkage

void at_thread_exit() {
    delete tls_get_recs();
}

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
