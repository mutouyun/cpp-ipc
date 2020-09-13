#pragma once

#include <memory>   // std::unique_ptr
#include <utility>  // std::forward
#include <cstddef>  // std::size_t

#include "export.h"
#include "platform/detail.h"

namespace ipc {
namespace tls {

using key_t = std::size_t;
using destructor_t = void (*)(void *);

struct key_info {
    key_t key_;
};

IPC_EXPORT bool create (key_info * pkey, destructor_t destructor = nullptr);
IPC_EXPORT void release(key_info const * pkey);

IPC_EXPORT bool   set(key_info const * pkey, void * ptr);
IPC_EXPORT void * get(key_info const * pkey);

////////////////////////////////////////////////////////////////
/// Thread-local pointer
////////////////////////////////////////////////////////////////

/**
 * @remarks
 * You need to set the ipc::tls::pointer's storage manually:
 * ```
 *     tls::pointer<int> p;
 *     if (!p) p = new int(123);
 * ```
 * It would be like an ordinary pointer.
 * Or you could just call create_once to 'new' this pointer automatically.
 * ```
 *     tls::pointer<int> p;
 *     p.create_once(123);
 * ```
*/

template <typename T>
class pointer : public key_info {

    pointer(pointer const &) = delete;
    pointer & operator=(pointer const &) = delete;

    void destruct() const {
        delete static_cast<T*>(tls::get(this));
    }

public:
    using value_type = T;

    pointer() {
        tls::create(this, [](void* p) {
            delete static_cast<T*>(p);
        });
    }

    ~pointer() {
        destruct();
        tls::release(this);
    }

    template <typename... P>
    T* create(P&&... params) {
        destruct();
        auto ptr = detail::unique_ptr(new T(std::forward<P>(params)...));
        if (!tls::set(this, ptr.get())) {
            return nullptr;
        }
        return ptr.release();
    }

    template <typename... P>
    T* create_once(P&&... params) {
        auto p = static_cast<T*>(tls::get(this));
        if (p == nullptr) {
            auto ptr = detail::unique_ptr(new T(std::forward<P>(params)...));
            if (!tls::set(this, ptr.get())) {
                return nullptr;
            }
            p = ptr.release();
        }
        return p;
    }

    T* operator=(T* p) {
        set(this, p);
        return p;
    }

    explicit operator T *() const {
        return static_cast<T *>(tls::get(this));
    }

    explicit operator bool() const {
        return tls::get(this) != nullptr;
    }

    T &       operator* ()       { return *static_cast<T *>(*this); }
    const T & operator* () const { return *static_cast<T *>(*this); }

    T *       operator->()       { return  static_cast<T *>(*this); }
    const T * operator->() const { return  static_cast<T *>(*this); }
};

} // namespace tls
} // namespace ipc
