#pragma once

#include <new>
#include <utility>

#include "concept.h"
#include "pool_alloc.h"

namespace ipc {

// pimpl small object optimization helpers

template <typename T, typename R = T*>
using IsImplComfortable = ipc::require<(sizeof(T) <= sizeof(T*)), R>;

template <typename T, typename R = T*>
using IsImplUncomfortable = ipc::require<(sizeof(T) > sizeof(T*)), R>;

template <typename T, typename... P>
constexpr auto make_impl(P&&... params) -> IsImplComfortable<T> {
    T* buf {};
    ::new (&buf) T { std::forward<P>(params)... };
    return buf;
}

template <typename T>
constexpr auto impl(T* const (& p)) -> IsImplComfortable<T> {
    return reinterpret_cast<T*>(&const_cast<char &>(reinterpret_cast<char const &>(p)));
}

template <typename T>
constexpr auto clear_impl(T* p) -> IsImplComfortable<T, void> {
    if (p != nullptr) impl(p)->~T();
}

template <typename T, typename... P>
constexpr auto make_impl(P&&... params) -> IsImplUncomfortable<T> {
    return mem::alloc<T>(std::forward<P>(params)...);
}

template <typename T>
constexpr auto clear_impl(T* p) -> IsImplUncomfortable<T, void> {
    mem::free(p);
}

template <typename T>
constexpr auto impl(T* const (& p)) -> IsImplUncomfortable<T> {
    return p;
}

template <typename T>
struct pimpl {
    template <typename... P>
    constexpr static T* make(P&&... params) {
        return make_impl<T>(std::forward<P>(params)...);
    }

#if __cplusplus >= 201703L
    constexpr void clear() {
#else /*__cplusplus < 201703L*/
    void clear() {
#endif/*__cplusplus < 201703L*/
        clear_impl(static_cast<T*>(const_cast<pimpl*>(this)));
    }
};

} // namespace ipc
