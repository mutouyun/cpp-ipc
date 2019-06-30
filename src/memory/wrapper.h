#pragma once

#include <limits>
#include <new>
#include <tuple>
#include <thread>
#include <vector>
#include <functional>
#include <utility>
#include <cstddef>
#include <type_traits>

#include "def.h"
#include "rw_lock.h"
#include "tls_pointer.h"
#include "platform/detail.h"

namespace ipc {
namespace mem {

////////////////////////////////////////////////////////////////
/// The allocator wrapper class for STL
////////////////////////////////////////////////////////////////

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
    allocator_wrapper(void) noexcept = default;

    allocator_wrapper(const allocator_wrapper<T, AllocP>& rhs) noexcept
        : alloc_(rhs.alloc_)
    {}

    template <typename U>
    allocator_wrapper(const allocator_wrapper<U, AllocP>& rhs) noexcept
        : alloc_(rhs.alloc_)
    {}

    allocator_wrapper(allocator_wrapper<T, AllocP>&& rhs) noexcept
        : alloc_(std::move(rhs.alloc_))
    {}

    template <typename U>
    allocator_wrapper(allocator_wrapper<U, AllocP>&& rhs) noexcept
        : alloc_(std::move(rhs.alloc_))
    {}

    allocator_wrapper(const AllocP& rhs) noexcept
        : alloc_(rhs)
    {}

    allocator_wrapper(AllocP&& rhs) noexcept
        : alloc_(std::move(rhs))
    {}

public:
    // the other type of std_allocator
    template <typename U>
    struct rebind { typedef allocator_wrapper<U, AllocP> other; };

    constexpr size_type max_size(void) const noexcept {
        return (std::numeric_limits<size_type>::max)() / sizeof(T);
    }

public:
    pointer allocate(size_type count) noexcept {
        if (count == 0) return nullptr;
        if (count > this->max_size()) return nullptr;
        return static_cast<pointer>(alloc_.alloc(count * sizeof(T)));
    }

    void deallocate(pointer p, size_type count) noexcept {
        alloc_.free(p, count * sizeof(T));
    }

    template <typename... P>
    static void construct(pointer p, P&&... params) {
        ::new (static_cast<void*>(p)) T(std::forward<P>(params)...);
    }

    static void destroy(pointer p) {
        p->~T();
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

////////////////////////////////////////////////////////////////
/// Thread-safe allocation wrapper
////////////////////////////////////////////////////////////////

template <typename AllocP>
class async_wrapper {
public:
    using alloc_policy = AllocP;

private:
    spin_lock master_lock_;
    std::vector<alloc_policy> master_allocs_;

    class alloc_proxy : public AllocP {
        async_wrapper * w_ = nullptr;

    public:
        alloc_proxy(alloc_proxy&& rhs)
            : AllocP(std::move(rhs))
        {}

        alloc_proxy(async_wrapper* w) : w_(w) {
            if (w_ == nullptr) return;
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(w_->master_lock_);
            if (!w_->master_allocs_.empty()) {
                AllocP::swap(w_->master_allocs_.back());
                w_->master_allocs_.pop_back();
            }
        }

        ~alloc_proxy() {
            if (w_ == nullptr) return;
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(w_->master_lock_);
            w_->master_allocs_.emplace_back(std::move(*this));
        }
    };

    friend class alloc_proxy;

    auto& get_alloc() {
        static tls::pointer<alloc_proxy> tls_alc;
        return *tls_alc.create(this);
    }

public:
    ~async_wrapper() {
        clear();
    }

    void clear() {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        master_allocs_.clear();
    }

    void* alloc(std::size_t size) {
        return get_alloc().alloc(size);
    }

    void free(void* p) {
        get_alloc().free(p);
    }

    void free(void* p, std::size_t /*size*/) {
        free(p);
    }
};

////////////////////////////////////////////////////////////////
/// Static allocation wrapper
////////////////////////////////////////////////////////////////

template <typename AllocP>
class static_wrapper {
public:
    using alloc_policy = AllocP;

    static alloc_policy& instance() {
        static alloc_policy alloc;
        return alloc;
    }

    static void clear() {
        instance().clear();
    }

    static void* alloc(std::size_t size) {
        return instance().alloc(size);
    }

    static void free(void* p, std::size_t size) {
        instance().free(p, size);
    }
};

} // namespace mem
} // namespace ipc
