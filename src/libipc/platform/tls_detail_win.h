#pragma once

#include <unordered_map>    // std::unordered_map
#include <cassert>          // assert

#include "libipc/tls_pointer.h"

#include "libipc/utility/utility.h"

namespace ipc {
namespace tls {

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

inline void at_thread_exit() {
    delete tls_get_recs();
}

} // namespace tls
} // namespace ipc
