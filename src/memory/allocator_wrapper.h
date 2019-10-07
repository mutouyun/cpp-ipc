#pragma once

#include <limits>   // std::numeric_limits
#include <utility>  // std::forward
#include <cstddef>
#include <new>      // ::new

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

template <template <typename U> class AllocT, typename T>
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

    // no copy
    allocator_wrapper(const allocator_wrapper<T, AllocP>&) noexcept {}
    template <typename U>
    allocator_wrapper(const allocator_wrapper<U, AllocP>&) noexcept {}

    allocator_wrapper(allocator_wrapper<T, AllocP> && rhs) noexcept : alloc_(std::move(rhs.alloc_)) {}
    template <typename U>
    allocator_wrapper(allocator_wrapper<U, AllocP> && rhs) noexcept : alloc_(std::move(rhs.alloc_)) {}
    allocator_wrapper(alloc_policy                 && rhs) noexcept : alloc_(std::move(rhs)) {}

public:
    // the other type of std_allocator
    template <typename U>
    struct rebind { typedef allocator_wrapper< U, typename detail::rebind<T, AllocP>::template alloc_t<U> > other; };

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
        ::new (static_cast<void*>(p)) value_type(std::forward<P>(params) ...);
    }

    static void destroy(pointer p) {
        p->~value_type();
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
