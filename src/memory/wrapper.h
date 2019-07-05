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
#include "concept.h"

#include "memory/alloc.h"
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
class default_alloc_recycler {
public:
    using alloc_policy = AllocP;

private:
    ipc::spin_lock            master_lock_;
    std::vector<alloc_policy> master_allocs_;

    IPC_CONCEPT_(has_remain, remain());
    IPC_CONCEPT_(has_empty , empty());

public:
    void swap(default_alloc_recycler& rhs) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        master_allocs_.swap(rhs.master_allocs_);
    }

    void clear() {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        master_allocs_.clear();
    }

    void try_recover(alloc_policy & alc) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        if (!master_allocs_.empty()) {
            alc.swap(master_allocs_.back());
            master_allocs_.pop_back();
        }
    }

    template <typename A = AllocP>
    auto try_replenish(alloc_policy & alc, std::size_t size)
        -> ipc::require<detail::has_take<A>::value && has_remain<A>::value> {
        if (alc.remain() >= size) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        if (!master_allocs_.empty()) {
            alc.take(std::move(master_allocs_.back()));
            master_allocs_.pop_back();
        }
    }

    template <typename A = AllocP>
    auto try_replenish(alloc_policy & alc, std::size_t /*size*/)
        -> ipc::require<detail::has_take<A>::value && !has_remain<A>::value && has_empty<A>::value> {
        if (!alc.empty()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        if (!master_allocs_.empty()) {
            alc.take(std::move(master_allocs_.back()));
            master_allocs_.pop_back();
        }
    }

    template <typename A = AllocP>
    constexpr auto try_replenish(alloc_policy & /*alc*/, std::size_t /*size*/) const noexcept
        -> ipc::require<!detail::has_take<A>::value || (!has_remain<A>::value && !has_empty<A>::value)> {}

    void collect(alloc_policy && alc) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        master_allocs_.emplace_back(std::move(alc));
    }
};

template <typename AllocP>
class empty_alloc_recycler {
public:
    using alloc_policy = AllocP;

    constexpr static void swap(empty_alloc_recycler&)               noexcept {}
    constexpr static void clear()                                   noexcept {}
    constexpr static void try_recover(alloc_policy&)                noexcept {}
    constexpr static auto try_replenish(alloc_policy&, std::size_t) noexcept {}
    constexpr static void collect(alloc_policy&&)                   noexcept {}
};

template <typename AllocP,
          template <typename> class RecyclerP = default_alloc_recycler>
class async_wrapper {
public:
    using alloc_policy = AllocP;

private:
    RecyclerP<alloc_policy> recycler_;

    class alloc_proxy : public AllocP {
        async_wrapper * w_ = nullptr;

    public:
        alloc_proxy(alloc_proxy && rhs)
            : AllocP(std::move(rhs))
        {}

        alloc_proxy(async_wrapper* w)
            : AllocP(), w_(w) {
            if (w_ == nullptr) return;
            w_->recycler_.try_recover(*this);
        }

        ~alloc_proxy() {
            if (w_ == nullptr) return;
            w_->recycler_.collect(std::move(*this));
        }

        auto alloc(std::size_t size) {
            if (w_ != nullptr) {
                w_->recycler_.try_replenish(*this, size);
            }
            return AllocP::alloc(size);
        }
    };

    friend class alloc_proxy;

    auto& get_alloc() {
        static tls::pointer<alloc_proxy> tls_alc;
        return *tls_alc.create(this);
    }

public:
    void swap(async_wrapper& rhs) {
        recycler_.swap(rhs.recycler_);
    }

    void clear() {
        recycler_.clear();
    }

    void* alloc(std::size_t size) {
        return get_alloc().alloc(size);
    }

    void free(void* p, std::size_t size) {
        get_alloc().free(p, size);
    }
};

////////////////////////////////////////////////////////////////
/// Thread-safe allocation wrapper (with spin_lock)
////////////////////////////////////////////////////////////////

template <typename AllocP, typename MutexT = ipc::spin_lock>
class sync_wrapper {
public:
    using alloc_policy = AllocP;
    using mutex_type   = MutexT;

private:
    mutex_type   lock_;
    alloc_policy alloc_;

public:
    void swap(sync_wrapper& rhs) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        alloc_.swap(rhs.alloc_);
    }

    void clear() {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        alloc_.~alloc_policy();
    }

    void* alloc(std::size_t size) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        return alloc_.alloc(size);
    }

    void free(void* p, std::size_t size) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        alloc_.free(p, size);
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

    static void swap(static_wrapper&) {}

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

////////////////////////////////////////////////////////////////
/// Variable memory allocation wrapper
////////////////////////////////////////////////////////////////

template <std::size_t BaseSize = 0, std::size_t IterSize = sizeof(void*)>
struct default_mapping_policy {

    enum : std::size_t {
        base_size    = BaseSize,
        iter_size    = IterSize,
        classes_size = 32
    };

    static const std::size_t table[classes_size];

    IPC_CONSTEXPR_ static std::size_t classify(std::size_t size) noexcept {
        auto index = (size <= base_size) ? 0 : ((size - base_size - 1) / iter_size);
        return (index < classes_size) ?
            // always uses default_mapping_policy<>::table
            default_mapping_policy<>::table[index] : classes_size;
    }

    constexpr static std::size_t block_size(std::size_t value) noexcept {
        return base_size + (value + 1) * iter_size;
    }
};

template <std::size_t B, std::size_t I>
const std::size_t default_mapping_policy<B, I>::table[default_mapping_policy<B, I>::classes_size] = {
    /* 1 - 8 ~ 32 */
    0 , 1 , 2 , 3 ,
    /* 2 - 48 ~ 256 */
    5 , 5 , 7 , 7 , 9 , 9 , 11, 11, 13, 13, 15, 15, 17, 17,
    19, 19, 21, 21, 23, 23, 25, 25, 27, 27, 29, 29, 31, 31
};

template <template <std::size_t> class Fixed,
          typename MappingP    = default_mapping_policy<>,
          typename StaticAlloc = mem::static_alloc>
class variable_wrapper {

    template <typename F>
    constexpr static auto choose(std::size_t size, F&& f) {
        return ipc::detail::static_switch<MappingP::classes_size>(MappingP::classify(size), [&f](auto index) {
            return f(Fixed<MappingP::block_size(decltype(index)::value)>{});
        }, [&f] {
            return f(StaticAlloc{});
        });
    }

public:
    static void swap(variable_wrapper&) {}

    static void clear() {
        ipc::detail::static_for<MappingP::classes_size>([](auto index) {
            Fixed<MappingP::block_size(decltype(index)::value)>::clear();
        });
        StaticAlloc::clear();
    }

    static void* alloc(std::size_t size) {
        return choose(size, [size](auto&& alc) { return alc.alloc(size); });
    }

    static void free(void* p, std::size_t size) {
        choose(size, [p, size](auto&& alc) { alc.free(p, size); });
    }
};

} // namespace mem
} // namespace ipc
