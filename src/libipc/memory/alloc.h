#pragma once

#include <algorithm>
#include <utility>
#include <iterator>
#include <limits>       // std::numeric_limits
#include <cstdlib>
#include <cassert>      // assert

#include "libipc/def.h"
#include "libipc/rw_lock.h"

#include "libipc/utility/concept.h"
#include "libipc/memory/allocator_wrapper.h"
#include "libipc/platform/detail.h"

namespace ipc {
namespace mem {

class static_alloc {
public:
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
};

////////////////////////////////////////////////////////////////
/// Scope allocation -- The destructor will release all allocated blocks.
////////////////////////////////////////////////////////////////

namespace detail {

constexpr std::size_t aligned(std::size_t size, size_t alignment) noexcept {
    return ( (size - 1) & ~(alignment - 1) ) + alignment;
}

IPC_CONCEPT_(has_take, take(std::move(std::declval<Type>())));

class scope_alloc_base {
protected:
    struct block_t {
        std::size_t size_;
        block_t   * next_;
    } * head_ = nullptr, * tail_ = nullptr;

    enum : std::size_t {
        aligned_block_size = aligned(sizeof(block_t), alignof(std::max_align_t))
    };

public:
    void swap(scope_alloc_base & rhs) {
        std::swap(head_, rhs.head_);
        std::swap(tail_, rhs.tail_);
    }

    bool empty() const noexcept {
        return head_ == nullptr;
    }

    void take(scope_alloc_base && rhs) {
        if (rhs.empty()) return;
        if (empty()) swap(rhs);
        else {
            std::swap(tail_->next_, rhs.head_);
            // rhs.head_ should be nullptr here
            tail_ = rhs.tail_;
            rhs.tail_ = nullptr;
        }
    }

    void free(void* /*p*/) {}
    void free(void* /*p*/, std::size_t) {}
};

} // namespace detail

template <typename AllocP = static_alloc>
class scope_alloc : public detail::scope_alloc_base {
public:
    using base_t = detail::scope_alloc_base;
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

    void free_all() {
        while (!empty()) {
            auto curr = head_;
            head_ = head_->next_;
            alloc_.free(curr, curr->size_);
        }
        // now head_ is nullptr
    }

public:
    scope_alloc() = default;

    scope_alloc(scope_alloc && rhs)         { swap(rhs); }
    scope_alloc& operator=(scope_alloc rhs) { swap(rhs); return (*this); }

    ~scope_alloc() { free_all(); }

    void swap(scope_alloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    template <typename A = AllocP>
    auto take(scope_alloc && rhs) -> ipc::require<detail::has_take<A>::value> {
        base_t::take(std::move(rhs));
        alloc_.take(std::move(rhs.alloc_));
    }

    template <typename A = AllocP>
    auto take(scope_alloc && rhs) -> ipc::require<!detail::has_take<A>::value> {
        base_t::take(std::move(rhs));
    }

    void* alloc(std::size_t size) {
        std::size_t real_size = aligned_block_size + size;
        auto curr = static_cast<block_t*>(alloc_.alloc(real_size));
        curr->size_ = real_size;
        curr->next_ = head_;
        head_ = curr;
        if (tail_ == nullptr) {
            tail_ = curr;
        }
        return (reinterpret_cast<byte_t*>(curr) + aligned_block_size);
    }
};

////////////////////////////////////////////////////////////////
/// Fixed-size blocks allocation
////////////////////////////////////////////////////////////////

namespace detail {

class fixed_alloc_base {
protected:
    std::size_t block_size_;
    std::size_t init_expand_;
    void      * cursor_;

    void init(std::size_t block_size, std::size_t init_expand) {
        block_size_  = block_size;
        init_expand_ = init_expand;
        cursor_      = nullptr;
    }

    static void** node_p(void* node) {
        return reinterpret_cast<void**>(node);
    }

    static auto& next(void* node) {
        return *node_p(node);
    }

public:
    bool operator<(fixed_alloc_base const & right) const {
        return init_expand_ < right.init_expand_;
    }

    void set_block_size(std::size_t block_size) {
        block_size_ = block_size;
    }

    void swap(fixed_alloc_base& rhs) {
        std::swap(block_size_ , rhs.block_size_);
        std::swap(init_expand_, rhs.init_expand_);
        std::swap(cursor_     , rhs.cursor_);
    }

    bool empty() const noexcept {
        return cursor_ == nullptr;
    }

    void take(fixed_alloc_base && rhs) {
        assert(block_size_ == rhs.block_size_);
        init_expand_ = (ipc::detail::max)(init_expand_, rhs.init_expand_);
        if (rhs.empty()) return;
        auto curr = cursor_;
        if (curr != nullptr) while (1) {
            auto next_cur = next(curr);
            if (next_cur == nullptr) {
                std::swap(next(curr), rhs.cursor_);
                return;
            }
            // next_cur != nullptr
            else curr = next_cur;
        }
        // curr == nullptr, means cursor_ == nullptr
        else std::swap(cursor_, rhs.cursor_);
        // rhs.cursor_ must be nullptr
    }

    void free(void* p) {
        if (p == nullptr) return;
        next(p) = cursor_;
        cursor_ = p;
    }

    void free(void* p, std::size_t) {
        free(p);
    }
};

template <typename AllocP, typename ExpandP>
class fixed_alloc : public detail::fixed_alloc_base {
public:
    using base_t = detail::fixed_alloc_base;
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

    void* try_expand() {
        if (empty()) {
            auto size = ExpandP::next(block_size_, init_expand_);
            auto p = node_p(cursor_ = alloc_.alloc(size));
            for (std::size_t i = 0; i < (size / block_size_) - 1; ++i)
                p = node_p((*p) = reinterpret_cast<byte_t*>(p) + block_size_);
            (*p) = nullptr;
        }
        return cursor_;
    }

public:
    explicit fixed_alloc(std::size_t block_size, std::size_t init_expand = 1) {
        init(block_size, init_expand);
    }

    fixed_alloc(fixed_alloc && rhs) {
        init(0, 0);
        swap(rhs);
    }

    fixed_alloc& operator=(fixed_alloc rhs) {
        swap(rhs);
        return (*this);
    }

    void swap(fixed_alloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    template <typename A = AllocP>
    auto take(fixed_alloc && rhs) -> ipc::require<detail::has_take<A>::value> {
        base_t::take(std::move(rhs));
        alloc_.take(std::move(rhs.alloc_));
    }

    void* alloc() {
        void* p = try_expand();
        cursor_ = next(p);
        return p;
    }

    void* alloc(std::size_t) {
        return alloc();
    }
};

} // namespace detail

template <std::size_t BaseSize  = sizeof(void*) * 1024, 
          std::size_t LimitSize = (std::numeric_limits<std::uint32_t>::max)()>
struct fixed_expand_policy {

    enum : std::size_t {
        base_size  = BaseSize,
        limit_size = LimitSize
    };

    constexpr static std::size_t prev(std::size_t e) noexcept {
        return ((e / 2) == 0) ? 1 : (e / 2);
    }

    constexpr static std::size_t next(std::size_t e) noexcept {
        return e * 2;
    }

    static std::size_t next(std::size_t block_size, std::size_t & e) {
        auto n = ipc::detail::max<std::size_t>(block_size, base_size) * e;
             e = ipc::detail::min<std::size_t>(limit_size, next(e));
        return n;
    }
};

template <std::size_t BlockSize,
          typename AllocP  = scope_alloc<>,
          typename ExpandP = fixed_expand_policy<>>
class fixed_alloc : public detail::fixed_alloc<AllocP, ExpandP> {
public:
    using base_t = detail::fixed_alloc<AllocP, ExpandP>;

    enum : std::size_t {
        block_size = ipc::detail::max<std::size_t>(BlockSize, sizeof(void*))
    };

public:
    explicit fixed_alloc(std::size_t init_expand)
        : base_t(block_size, init_expand) {
    }

    fixed_alloc() : fixed_alloc(1) {}

    fixed_alloc(fixed_alloc && rhs)
        : base_t(std::move(rhs)) {
    }

    fixed_alloc& operator=(fixed_alloc rhs) {
        swap(rhs);
        return (*this);
    }

    void swap(fixed_alloc& rhs) {
        base_t::swap(rhs);
    }
};

////////////////////////////////////////////////////////////////
/// Variable-size blocks allocation (without alignment)
////////////////////////////////////////////////////////////////

namespace detail {

class variable_alloc_base {
protected:
    byte_t * head_ = nullptr, * tail_ = nullptr;

public:
    void swap(variable_alloc_base & rhs) {
        std::swap(head_, rhs.head_);
        std::swap(tail_, rhs.tail_);
    }

    std::size_t remain() const noexcept {
        return static_cast<std::size_t>(tail_ - head_);
    }

    bool empty() const noexcept {
        return remain() == 0;
    }

    void take(variable_alloc_base && rhs) {
        if (remain() < rhs.remain()) {
            // replace this by rhs
            head_ = rhs.head_;
            tail_ = rhs.tail_;
        }
        // discard rhs
        rhs.head_ = rhs.tail_ = nullptr;
    }

    void free(void* /*p*/) {}
    void free(void* /*p*/, std::size_t) {}
};

} // namespace detail

template <std::size_t ChunkSize = (sizeof(void*) * 1024), typename AllocP = scope_alloc<>>
class variable_alloc : public detail::variable_alloc_base {
public:
    using base_t       = detail::variable_alloc_base;
    using alloc_policy = AllocP;

    enum : std::size_t {
        aligned_chunk_size = detail::aligned(ChunkSize, alignof(std::max_align_t))
    };

private:
    alloc_policy alloc_;

public:
    variable_alloc() = default;

    variable_alloc(variable_alloc && rhs)         { swap(rhs); }
    variable_alloc& operator=(variable_alloc rhs) { swap(rhs); return (*this); }

    void swap(variable_alloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    template <typename A = AllocP>
    auto take(variable_alloc && rhs) -> ipc::require<detail::has_take<A>::value> {
        base_t::take(std::move(rhs));
        alloc_.take(std::move(rhs.alloc_));
    }

    void* alloc(std::size_t size) {
        /*
         * byte alignment is always alignof(std::max_align_t).
        */
        size = detail::aligned(size, alignof(std::max_align_t));
        void* ptr;
        // size would never be 0 here
        if (remain() < size) {
            std::size_t chunk_size = ipc::detail::max<std::size_t>(aligned_chunk_size, size);
            ptr   = alloc_.alloc(chunk_size);
            tail_ = static_cast<byte_t*>(ptr) + chunk_size;
            head_ = tail_ - (chunk_size - size);
        }
        else {
            ptr   = head_;
            head_ += size;
        }
        return ptr;
    }
};

} // namespace mem
} // namespace ipc
