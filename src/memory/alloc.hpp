#pragma once

#include <algorithm>
#include <utility>
#include <atomic>
#include <thread>
#include <mutex>
#include <cstdlib>

#include "def.h"
#include "rw_lock.h"

#include "platform/detail.h"

namespace ipc {
namespace mem {

class static_alloc {
public:
    static void clear() {}
    static void swap(static_alloc&) {}

    static void* alloc(std::size_t size) {
        return size ? std::malloc(size) : nullptr;
    }

    static void free(void* p) {
        std::free(p);
    }

    static void free(void* p, std::size_t /*size*/) {
        free(p);
    }

    static std::size_t size_of(void* /*p*/) {
        return 0;
    }
};

////////////////////////////////////////////////////////////////
/// Scope allocation -- The destructor will release all allocated blocks.
////////////////////////////////////////////////////////////////

namespace detail {

class scope_alloc_base {
protected:
    struct block_t {
        block_t* next_;
    };
    block_t* list_ = nullptr;

public:
    void free(void* /*p*/) {}
    void free(void* /*p*/, std::size_t) {}

    constexpr std::size_t size_of(void* /*p*/) const {
        return 0;
    }
};

} // namespace detail

template <typename AllocP = static_alloc>
class scope_alloc : public detail::scope_alloc_base {
public:
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

public:
    scope_alloc() = default;

    scope_alloc(scope_alloc&& rhs)            { this->swap(rhs); }
    scope_alloc& operator=(scope_alloc&& rhs) { this->swap(rhs); return (*this); }

    ~scope_alloc() { clear(); }

public:
    void swap(scope_alloc& rhs) {
        std::swap(this->alloc_, rhs.alloc_);
        std::swap(this->list_ , rhs.list_);
    }

    void clear() {
        while (list_ != nullptr) {
            auto curr = list_;
            list_ = list_->next_;
            alloc_.free(curr);
        }
        // now list_ is nullptr
        alloc_.clear();
    }

    void* alloc(std::size_t size) {
        auto curr = static_cast<block_t*>(alloc_.alloc(sizeof(block_t) + size));
        curr->next_ = list_;
        return ((list_ = curr) + 1);
    }
};

////////////////////////////////////////////////////////////////
/// Fixed-size blocks allocation
////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
class non_atomic {
    T val_;

public:
    void store(T val, std::memory_order) noexcept {
        val_ = val;
    }

    T load(std::memory_order) const noexcept {
        return val_;
    }

    bool compare_exchange_weak(T&, T val, std::memory_order) noexcept {
        // always return true
        val_ = val;
        return true;
    }
};

class non_lock {
public:
    void lock  (void) noexcept {}
    void unlock(void) noexcept {}
};

template <template <typename> class Atomic>
class fixed_alloc_base {
protected:
    std::size_t init_expand_;
    Atomic<void*> cursor_;

    void init(std::size_t init_expand) {
        init_expand_ = init_expand;
        cursor_.store(nullptr, std::memory_order_relaxed);
    }

    static void** node_p(void* node) {
        return reinterpret_cast<void**>(node);
    }

    static auto& next(void* node) {
        return *node_p(node);
    }

public:
    void swap(fixed_alloc_base& rhs) {
        std::swap(this->init_expand_, rhs.init_expand_);
        auto tmp = this->cursor_.load(std::memory_order_relaxed);
        this->cursor_.store(rhs.cursor_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        rhs.cursor_.store(tmp, std::memory_order_relaxed);
    }

    void free(void* p) {
        if (p == nullptr) return;
        while (1) {
            next(p) = cursor_.load(std::memory_order_acquire);
            if (cursor_.compare_exchange_weak(next(p), p, std::memory_order_relaxed)) {
                break;
            }
            std::this_thread::yield();
        }
    }

    void free(void* p, std::size_t) {
        free(p);
    }
};

} // namespace detail

template <std::size_t BlockSize, typename AllocP = scope_alloc<>,
          template <typename> class Atomic = detail::non_atomic,
          typename Lock = detail::non_lock>
class fixed_alloc : public detail::fixed_alloc_base<Atomic> {
public:
    using base_t = detail::fixed_alloc_base<Atomic>;
    using alloc_policy = AllocP;

    enum : std::size_t {
#   if __cplusplus >= 201703L
        block_size = (std::max)(BlockSize, sizeof(void*))
#   else /*__cplusplus < 201703L*/
        block_size = (BlockSize < sizeof(void*)) ? sizeof(void*) : BlockSize
#   endif/*__cplusplus < 201703L*/
    };

private:
    alloc_policy alloc_;
    Lock lc_;

    void* expand() {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lc_);
        auto c = this->cursor_.load(std::memory_order_relaxed);
        if (c != nullptr) {
            return c;
        }
        auto a = alloc_.alloc(block_size);
        this->cursor_.store(a, std::memory_order_relaxed);
        auto p = this->node_p(a);
        auto size = alloc_.size_of(p);
        if (size > 0) for (std::size_t i = 0; i < (size / block_size) - 1; ++i)
            p = this->node_p((*p) = reinterpret_cast<byte_t*>(p) + block_size);
        (*p) = nullptr;
        return a;
    }

public:
    explicit fixed_alloc(std::size_t init_expand = 1) {
        this->init(init_expand);
    }

    fixed_alloc(fixed_alloc&& rhs)            { this->swap(rhs); }
    fixed_alloc& operator=(fixed_alloc&& rhs) { this->swap(rhs); return (*this); }

public:
    void swap(fixed_alloc& rhs) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lc_);
        std::swap(this->alloc_, rhs.alloc_);
        base_t::swap(rhs);
    }

    void clear() {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(lc_);
        alloc_.clear();
        this->init(this->init_expand_);
    }

    constexpr std::size_t size_of(void* /*p*/) const {
        return block_size;
    }

    void* alloc() {
        void* p;
        while (1) {
            p = expand();
            if (this->cursor_.compare_exchange_weak(p, this->next(p), std::memory_order_relaxed)) {
                break;
            }
            std::this_thread::yield();
        }
        return p;
    }

    void* alloc(std::size_t) {
        return alloc();
    }
};

////////////////////////////////////////////////////////////////
/// page memory allocation
////////////////////////////////////////////////////////////////

using page_alloc = fixed_alloc<4096>;

template <std::size_t BlockSize>
using page_fixed_alloc = fixed_alloc<BlockSize, page_alloc>;

template <std::size_t BlockSize>
using locked_fixed_alloc = fixed_alloc<BlockSize, page_alloc, std::atomic, ipc::spin_lock>;

} // namespace mem
} // namespace ipc
