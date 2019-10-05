#pragma once

#include <limits>
#include <new>          // ::new
#include <tuple>
#include <thread>
#include <vector>
#include <functional>   // std::function
#include <utility>      // std::forward
#include <cstddef>
#include <cassert>      // assert
#include <type_traits>  // std::aligned_storage_t

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
        -> ipc::require<(!detail::has_take<A>::value || !has_remain<A>::value) && has_empty<A>::value> {
        if (!alc.empty()) return;
        try_recover(alc);
    }

    template <typename A = AllocP>
    constexpr auto try_replenish(alloc_policy & /*alc*/, std::size_t /*size*/) const noexcept
        -> ipc::require<(!detail::has_take<A>::value || !has_remain<A>::value) && !has_empty<A>::value> {
        // Do Nothing.
    }

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
        alloc_proxy(alloc_proxy && rhs) = default;

        template <typename ... P>
        alloc_proxy(async_wrapper* w, P && ... pars)
            : AllocP(std::forward<P>(pars) ...), w_(w) {
            assert(w_ != nullptr);
            w_->recycler_.try_recover(*this);
        }

        ~alloc_proxy() {
            w_->recycler_.collect(std::move(*this));
        }

        // auto alloc(std::size_t size) {
        //     w_->recycler_.try_replenish(*this, size);
        //     return AllocP::alloc(size);
        // }
    };

    friend class alloc_proxy;

    using ref_t = alloc_proxy&;
    using tls_t = tls::pointer<alloc_proxy>;

    tls_t tls_;
    std::function<ref_t()> get_alloc_;

public:
    template <typename ... P>
    async_wrapper(P ... pars) {
        get_alloc_ = [this, pars ...]()->ref_t {
            return *tls_.create(this, pars ...);
        };
    }

    void* alloc(std::size_t size) {
        return get_alloc_().alloc(size);
    }

    void free(void* p, std::size_t size) {
        get_alloc_().free(p, size);
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
    template <typename ... P>
    sync_wrapper(P && ... pars)
        : alloc_(std::forward<P>(pars) ...)
    {}

    void swap(sync_wrapper& rhs) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lock_);
        alloc_.swap(rhs.alloc_);
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
        classes_size = 64
    };

    template <typename F, typename ... P>
    IPC_CONSTEXPR_ static void foreach(F f, P ... params) {
        for (std::size_t i = 0; i < classes_size; ++i) f(i, params...);
    }

    IPC_CONSTEXPR_ static std::size_t block_size(std::size_t id) noexcept {
        return (id < classes_size) ? (base_size + (id + 1) * iter_size) : 0;
    }

    template <typename F, typename D, typename ... P>
    IPC_CONSTEXPR_ static auto classify(F f, D d, std::size_t size, P ... params) {
        std::size_t id = (size - base_size - 1) / iter_size;
        return (id < classes_size) ? f(id, params..., size) : d(params..., size);
    }
};

template <typename FixedAlloc,
          typename DefaultAlloc = mem::static_alloc,
          typename MappingP     = default_mapping_policy<>>
class static_variable_wrapper {
private:
    static FixedAlloc& instance(std::size_t id) {
        static struct initiator {
            std::aligned_storage_t<sizeof (FixedAlloc), 
                                   alignof(FixedAlloc)> arr_[MappingP::classes_size];
            initiator() {
                MappingP::foreach([](std::size_t id, initiator* t) {
                    ::new (&(t->arr_[id])) FixedAlloc(MappingP::block_size(id));
                }, this);
            }
        } init__;
        return reinterpret_cast<FixedAlloc&>(init__.arr_[id]);
    }

public:
    static void swap(static_variable_wrapper&) {}

    static void* alloc(std::size_t size) {
        return MappingP::classify([](std::size_t id, std::size_t size) {
            return instance(id).alloc(size);
        }, DefaultAlloc::alloc, size);
    }

    static void free(void* p, std::size_t size) {
        MappingP::classify([](std::size_t id, void* p, std::size_t size) {
            instance(id).free(p, size);
        }, static_cast<void(*)(void*, std::size_t)>(DefaultAlloc::free), size, p);
    }
};

} // namespace mem
} // namespace ipc
