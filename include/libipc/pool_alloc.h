#pragma once

#include <new>
#include <utility>

#include "libipc/export.h"
#include "libipc/def.h"

namespace ipc {
namespace mem {

class IPC_EXPORT pool_alloc {
public:
    static void* alloc(std::size_t size);
    static void  free (void* p, std::size_t size);
};

////////////////////////////////////////////////////////////////
/// construct/destruct an object
////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
struct impl {
    template <typename... P>
    static T* construct(T* p, P&&... params) {
        ::new (p) T(std::forward<P>(params)...);
        return p;
    }

    static void destruct(T* p) {
        reinterpret_cast<T*>(p)->~T();
    }
};

template <typename T, size_t N>
struct impl<T[N]> {
    using type = T[N];

    template <typename... P>
    static type* construct(type* p, P&&... params) {
        for (size_t i = 0; i < N; ++i) {
            impl<T>::construct(&((*p)[i]), std::forward<P>(params)...);
        }
        return p;
    }

    static void destruct(type* p) {
        for (size_t i = 0; i < N; ++i) {
            impl<T>::destruct(&((*p)[i]));
        }
    }
};

} // namespace detail

template <typename T, typename... P>
T* construct(T* p, P&&... params) {
    return detail::impl<T>::construct(p, std::forward<P>(params)...);
}

template <typename T, typename... P>
T* construct(void* p, P&&... params) {
    return construct(static_cast<T*>(p), std::forward<P>(params)...);
}

template <typename T>
void destruct(T* p) {
    return detail::impl<T>::destruct(p);
}

template <typename T>
void destruct(void* p) {
    destruct(static_cast<T*>(p));
}

////////////////////////////////////////////////////////////////
/// general alloc/free
////////////////////////////////////////////////////////////////

inline void* alloc(std::size_t size) {
    return pool_alloc::alloc(size);
}

template <typename T, typename... P>
T* alloc(P&&... params) {
    return construct<T>(pool_alloc::alloc(sizeof(T)), std::forward<P>(params)...);
}

inline void free(void* p, std::size_t size) {
    pool_alloc::free(p, size);
}

template <typename T>
void free(T* p) {
    if (p == nullptr) return;
    destruct(p);
    pool_alloc::free(p, sizeof(T));
}

} // namespace mem
} // namespace ipc
