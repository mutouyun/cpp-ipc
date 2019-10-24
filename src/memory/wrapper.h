#pragma once

#include <tuple>
#include <thread>
#include <set>          // std::set
#include <functional>   // std::function
#include <utility>      // std::forward
#include <cstddef>
#include <cassert>      // assert
#include <type_traits>  // std::aligned_storage_t
#include <new>          // ::new

#include "def.h"
#include "rw_lock.h"
#include "tls_pointer.h"
#include "concept.h"

#include "memory/alloc.h"
#include "platform/detail.h"

namespace ipc {
namespace mem {

////////////////////////////////////////////////////////////////
/// Thread-safe allocation wrapper
////////////////////////////////////////////////////////////////

namespace detail {

IPC_CONCEPT_(is_comparable, operator<(std::declval<Type>()));

} // namespace detail

template <typename AllocP, bool = detail::is_comparable<AllocP>::value>
class limited_recycler;

template <typename AllocP>
class limited_recycler<AllocP, true> {
public:
    using alloc_policy = AllocP;

protected:
    template <typename T>
    struct fixed_alloc_t : public fixed_alloc<sizeof(T)> {};

    template <typename T>
    using allocator = ipc::mem::allocator_wrapper<T, fixed_alloc_t<T>>;

    std::multiset<alloc_policy, std::less<alloc_policy>, 
                  allocator<alloc_policy>
    > master_allocs_;

    ipc::spin_lock master_lock_;

    template <typename F>
    void take_first_do(F && pred) {
        auto it = master_allocs_.begin();
        pred(const_cast<alloc_policy&>(*it));
        master_allocs_.erase(it);
    }

public:
    void swap(limited_recycler & rhs) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        master_allocs_.swap(rhs.master_allocs_);
    }

    void try_recover(alloc_policy & alc) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        if (master_allocs_.empty()) return;
        take_first_do([&alc](alloc_policy & first) { alc.swap(first); });
    }

    void collect(alloc_policy && alc) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(master_lock_);
        if (master_allocs_.size() >= 32) {
            take_first_do([](alloc_policy &) {}); // erase first
        }
        master_allocs_.emplace(std::move(alc));
    }

    IPC_CONSTEXPR_ auto try_replenish(alloc_policy&, std::size_t) noexcept {}
};

template <typename AllocP>
class default_recycler : public limited_recycler<AllocP> {

    IPC_CONCEPT_(has_remain, remain());
    IPC_CONCEPT_(has_empty , empty());

    template <typename A>
    void try_fill(A & alc) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(this->master_lock_);
        if (this->master_allocs_.empty()) return;
        this->take_first_do([&alc](alloc_policy & first) { alc.take(std::move(first)); });
    }

public:
    using alloc_policy = typename limited_recycler<AllocP>::alloc_policy;

    template <typename A = AllocP>
    auto try_replenish(alloc_policy & alc, std::size_t size)
        -> ipc::require<detail::has_take<A>::value && has_remain<A>::value> {
        if (alc.remain() >= size) return;
        this->try_fill(alc);
    }

    template <typename A = AllocP>
    auto try_replenish(alloc_policy & alc, std::size_t /*size*/)
        -> ipc::require<detail::has_take<A>::value && !has_remain<A>::value && has_empty<A>::value> {
        if (!alc.empty()) return;
        this->try_fill(alc);
    }

    template <typename A = AllocP>
    auto try_replenish(alloc_policy & alc, std::size_t /*size*/)
        -> ipc::require<!detail::has_take<A>::value && has_empty<A>::value> {
        if (!alc.empty()) return;
        this->try_recover(alc);
    }

    template <typename A = AllocP>
    IPC_CONSTEXPR_ auto try_replenish(alloc_policy & /*alc*/, std::size_t /*size*/) noexcept
        -> ipc::require<(!detail::has_take<A>::value || !has_remain<A>::value) && !has_empty<A>::value> {
        // Do Nothing.
    }
};

template <typename AllocP>
class empty_recycler {
public:
    using alloc_policy = AllocP;

    IPC_CONSTEXPR_ void swap(empty_recycler&)                     noexcept {}
    IPC_CONSTEXPR_ void try_recover(alloc_policy&)                noexcept {}
    IPC_CONSTEXPR_ auto try_replenish(alloc_policy&, std::size_t) noexcept {}
    IPC_CONSTEXPR_ void collect(alloc_policy&&)                   noexcept {}
};

template <typename AllocP,
          template <typename> class RecyclerP = default_recycler>
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

        auto alloc(std::size_t size) {
            w_->recycler_.try_replenish(*this, size);
            return AllocP::alloc(size);
        }
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
        return (id < classes_size) ? f(id, size, params...) : d(size, params...);
    }
};

template <typename FixedAlloc,
          typename DefaultAlloc = mem::static_alloc,
          typename MappingP     = default_mapping_policy<>>
class variable_wrapper {

    struct initiator {

        std::aligned_storage_t< sizeof(FixedAlloc), 
                               alignof(FixedAlloc)> arr_[MappingP::classes_size];

        initiator() {
            MappingP::foreach([](std::size_t id, initiator* t) {
                ::new (&(t->arr_[id])) FixedAlloc(MappingP::block_size(id));
            }, this);
        }

        FixedAlloc& at(std::size_t id) {
            return reinterpret_cast<FixedAlloc&>(arr_[id]);
        }
    } init_;

public:
    void swap(variable_wrapper & other) {
        MappingP::foreach([](std::size_t id, initiator* t, initiator* o) {
            t->at(id).swap(o->at(id));
        }, &init_, &other.init_);
    }

    void* alloc(std::size_t size) {
        return MappingP::classify([](std::size_t id, std::size_t size, initiator* t) {
            return t->at(id).alloc(size);
        }, [](std::size_t size, initiator*) {
            return DefaultAlloc::alloc(size);
        }, size, &init_);
    }

    void free(void* p, std::size_t size) {
        MappingP::classify([](std::size_t id, std::size_t size, void* p, initiator* t) {
            t->at(id).free(p, size);
        }, [](std::size_t size, void* p, initiator*) {
            DefaultAlloc::free(p, size);
        }, size, p, &init_);
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

} // namespace mem
} // namespace ipc
