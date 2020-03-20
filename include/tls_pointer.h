#pragma once

#include <cstdint>
#include <utility>
#include <limits>

#include "export.h"

namespace ipc {
namespace tls {

using key_t        = std::uint64_t;
using destructor_t = void (*)(void*);

enum : key_t {
    invalid_value = (std::numeric_limits<key_t>::max)()
};

IPC_EXPORT key_t create (destructor_t destructor = nullptr);
IPC_EXPORT void  release(key_t key);

IPC_EXPORT bool  set(key_t key, void* ptr);
IPC_EXPORT void* get(key_t key);

////////////////////////////////////////////////////////////////
/// Thread-local pointer
////////////////////////////////////////////////////////////////

/*
 * <Remarks>
 *
 * You need to set the ipc::tls::pointer's storage manually:
 * ```
 *     tls::pointer<int> p;
 *     if (!p) p = new int(123);
 * ```
 * It would be like an ordinary pointer.
 * Or you could just call create to 'new' this pointer automatically.
 * ```
 *     tls::pointer<int> p;
 *     p.create(123);
 * ```
*/

template <typename T>
class pointer {
    key_t key_;

public:
    using value_type = T;

    pointer()
        : key_(tls::create([](void* p) { delete static_cast<T*>(p); })) {
    }

    ~pointer() {
        tls::release(key_);
    }

    template <typename... P>
    T* create(P&&... params) {
        auto ptr = static_cast<T*>(get(key_));
        if (ptr == nullptr) {
            ptr = new T(std::forward<P>(params)...);
            if (!set(key_, ptr)) {
                delete ptr;
                return nullptr;
            }
        }
        return ptr;
    }

    T* operator=(T* ptr) {
        set(key_, ptr);
        return ptr;
    }

    operator T*() const { return static_cast<T*>(get(key_)); }

    T&       operator* ()       { return *static_cast<T*>(*this); }
    const T& operator* () const { return *static_cast<T*>(*this); }

    T*       operator->()       { return  static_cast<T*>(*this); }
    const T* operator->() const { return  static_cast<T*>(*this); }
};

} // namespace tls
} // namespace ipc
