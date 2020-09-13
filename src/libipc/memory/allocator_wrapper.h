#pragma once

#include <limits>   // std::numeric_limits
#include <utility>  // std::forward
#include <cstddef>

#include "libipc/pool_alloc.h"

namespace ipc {
namespace mem {

////////////////////////////////////////////////////////////////
/// The allocator wrapper class for STL
////////////////////////////////////////////////////////////////

namespace detail {

template <typename T, typename AllocP>
struct rebind {
    template <typename U>
    using alloc_t = AllocP;
};

template <typename T, template <typename> class AllocT>
struct rebind<T, AllocT<T>> {
    template <typename U>
    using alloc_t = AllocT<U>;
};

} // namespace detail

template <typename T, typename AllocP>
class allocator_wrapper {

    template <typename U, typename AllocU>
    friend class allocator_wrapper;

public:
    // type definitions
    typedef T                 value_type;
    typedef value_type*       pointer;
    typedef const value_type* const_pointer;
    typedef value_type&       reference;
    typedef const value_type& const_reference;
    typedef std::size_t       size_type;
    typedef std::ptrdiff_t    difference_type;
    typedef AllocP            alloc_policy;

private:
    alloc_policy alloc_;

public:
    allocator_wrapper() noexcept {}

    // construct by copying (do nothing)
    allocator_wrapper           (const allocator_wrapper<T, AllocP>&) noexcept {}
    allocator_wrapper& operator=(const allocator_wrapper<T, AllocP>&) noexcept { return *this; }
	
    // construct from a related allocator (do nothing)
    template <typename U, typename AllocU> allocator_wrapper           (const allocator_wrapper<U, AllocU>&) noexcept {}
    template <typename U, typename AllocU> allocator_wrapper& operator=(const allocator_wrapper<U, AllocU>&) noexcept { return *this; }

    allocator_wrapper           (allocator_wrapper && rhs) noexcept : alloc_ ( std::move(rhs.alloc_) ) {}
    allocator_wrapper& operator=(allocator_wrapper && rhs) noexcept { alloc_ = std::move(rhs.alloc_); return *this; }

public:
    // the other type of std_allocator
    template <typename U>
    struct rebind { 
        using other = allocator_wrapper< U, typename detail::rebind<T, AllocP>::template alloc_t<U> >;
    };

    constexpr size_type max_size(void) const noexcept {
        return (std::numeric_limits<size_type>::max)() / sizeof(value_type);
    }

public:
    pointer allocate(size_type count) noexcept {
        if (count == 0) return nullptr;
        if (count > this->max_size()) return nullptr;
        return static_cast<pointer>(alloc_.alloc(count * sizeof(value_type)));
    }

    void deallocate(pointer p, size_type count) noexcept {
        alloc_.free(p, count * sizeof(value_type));
    }

    template <typename... P>
    static void construct(pointer p, P && ... params) {
        ipc::mem::construct(p, std::forward<P>(params)...);
    }

    static void destroy(pointer p) {
        ipc::mem::destruct(p);
    }
};

template <class AllocP>
class allocator_wrapper<void, AllocP> {
public:
    // type definitions
    typedef void              value_type;
    typedef value_type*       pointer;
    typedef const value_type* const_pointer;
    typedef std::size_t       size_type;
    typedef std::ptrdiff_t    difference_type;
    typedef AllocP            alloc_policy;
};

template <typename T, typename U, class AllocP>
constexpr bool operator==(const allocator_wrapper<T, AllocP>&, const allocator_wrapper<U, AllocP>&) noexcept {
    return true;
}

template <typename T, typename U, class AllocP>
constexpr bool operator!=(const allocator_wrapper<T, AllocP>&, const allocator_wrapper<U, AllocP>&) noexcept {
    return false;
}

} // namespace mem
} // namespace ipc
