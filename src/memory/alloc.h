#pragma once

#include <algorithm>
#include <utility>
#include <cstdlib>
#include <map>
#include <iterator>

#include "def.h"
#include "rw_lock.h"
#include "concept.h"

#include "platform/detail.h"

namespace ipc {
namespace mem {

class static_alloc {
public:
    static void swap(static_alloc&) {}
    static void clear() {}

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
    return ((size - 1) & ~(alignment - 1)) + alignment;
}

IPC_CONCEPT_(has_take, take(Type{}));

class scope_alloc_base {
protected:
    struct block_t {
        block_t   * next_;
        std::size_t size_;
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

    scope_alloc(scope_alloc&& rhs)            { swap(rhs); }
    scope_alloc& operator=(scope_alloc&& rhs) { swap(rhs); return (*this); }

    ~scope_alloc() { free_all(); }

    template <typename A>
    void set_allocator(A && alc) {
        alloc_ = std::forward<A>(alc);
    }

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

    void clear() {
        free_all();
        tail_ = nullptr;
        alloc_.~alloc_policy();
    }

    void* alloc(std::size_t size) {
        auto curr = static_cast<block_t*>(alloc_.alloc(size += aligned_block_size));
        curr->next_ = head_;
        curr->size_ = size;
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
    std::size_t init_expand_;
    void      * cursor_;

    void init(std::size_t init_expand) {
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
    void swap(fixed_alloc_base& rhs) {
        std::swap(init_expand_, rhs.init_expand_);
        std::swap(cursor_     , rhs.cursor_);
    }

    bool empty() const noexcept {
        return cursor_ == nullptr;
    }

    void take(fixed_alloc_base && rhs) {
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

struct fixed_expand_policy {

    enum : std::size_t {
        base_size = sizeof(void*) * 1024 / 2
    };

    static std::size_t prev(std::size_t& e) {
        if ((e /= 2) == 0) e = 1;
        return e;
    }

    static std::size_t next(std::size_t& e) {
        return e *= 2;
    }

    template <std::size_t BlockSize>
    static std::size_t next(std::size_t & e) {
        return ipc::detail::max<std::size_t>(BlockSize, base_size) * next(e);
    }
};

} // namespace detail

template <std::size_t BlockSize,
          typename AllocP  = scope_alloc<>,
          typename ExpandP = detail::fixed_expand_policy>
class fixed_alloc : public detail::fixed_alloc_base {
public:
    using base_t = detail::fixed_alloc_base;
    using alloc_policy = AllocP;

    enum : std::size_t {
        block_size = (ipc::detail::max)(BlockSize, sizeof(void*))
    };

private:
    alloc_policy alloc_;

    void* try_expand() {
        if (empty()) {
            auto size = ExpandP::template next<block_size>(init_expand_);
            auto p = node_p(cursor_ = alloc_.alloc(size));
            for (std::size_t i = 0; i < (size / block_size) - 1; ++i)
                p = node_p((*p) = reinterpret_cast<byte_t*>(p) + block_size);
            (*p) = nullptr;
        }
        return cursor_;
    }

public:
    explicit fixed_alloc(std::size_t init_expand = 1) {
        init(init_expand);
    }

    fixed_alloc(fixed_alloc&& rhs) : fixed_alloc() { swap(rhs); }
    fixed_alloc& operator=(fixed_alloc&& rhs)      { swap(rhs); return (*this); }

    template <typename A>
    void set_allocator(A && alc) {
        alloc_ = std::forward<A>(alc);
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

    template <typename A = AllocP>
    auto take(fixed_alloc && rhs) -> ipc::require<!detail::has_take<A>::value> {
        base_t::take(std::move(rhs));
    }

    void clear() {
        ExpandP::prev(init_expand_);
        cursor_ = nullptr;
        alloc_.~alloc_policy();
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

////////////////////////////////////////////////////////////////
/// Variable-size blocks allocation (without alignment)
////////////////////////////////////////////////////////////////

namespace detail {

class variable_alloc_base {
protected:
    struct head_t {
        std::size_t free_;
    } * head_ = nullptr;

    std::map<std::size_t, head_t*, std::greater<std::size_t>> reserves_;

    enum : std::size_t {
        aligned_head_size = aligned(sizeof(head_t), alignof(std::max_align_t))
    };

    static byte_t * buffer(head_t* p) {
        return reinterpret_cast<byte_t*>(p) + aligned_head_size + p->free_;
    }

    std::size_t remain() const noexcept {
        return (head_ == nullptr) ? 0 : head_->free_;
    }

public:
    void swap(variable_alloc_base& rhs) {
        std::swap(head_, rhs.head_);
    }

    bool empty() const noexcept {
        return remain() == 0;
    }

    void take(variable_alloc_base && rhs) {
        if (rhs.remain() > remain()) {
            if (!empty()) {
                reserves_.emplace(head_->free_, head_);
            }
            head_ = rhs.head_;
        }
        else if (!rhs.empty()) {
            reserves_.emplace(rhs.head_->free_, rhs.head_);
        }
        rhs.head_ = nullptr;
    }

    void free(void* /*p*/) {}
    void free(void* /*p*/, std::size_t) {}
};

} // namespace detail

template <std::size_t ChunkSize = (sizeof(void*) * 1024), typename AllocP = scope_alloc<>>
class variable_alloc : public detail::variable_alloc_base {
public:
    using base_t = detail::variable_alloc_base;
    using head_t = base_t::head_t;
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

public:
    variable_alloc() = default;

    variable_alloc(variable_alloc&& rhs)            { swap(rhs); }
    variable_alloc& operator=(variable_alloc&& rhs) { swap(rhs); return (*this); }

    template <typename A>
    void set_allocator(A && alc) {
        alloc_ = std::forward<A>(alc);
    }

    void swap(variable_alloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    template <typename A = AllocP>
    auto take(variable_alloc && rhs) -> ipc::require<detail::has_take<A>::value> {
        base_t::take(std::move(rhs));
        alloc_.take(std::move(rhs.alloc_));
    }

    template <typename A = AllocP>
    auto take(variable_alloc && rhs) -> ipc::require<!detail::has_take<A>::value> {
        base_t::take(std::move(rhs));
    }

    void clear() {
        alloc_.~alloc_policy();
    }

    void* alloc(std::size_t size) {
        if (size >= (ChunkSize - aligned_head_size)) {
            return alloc_.alloc(size);
        }
        if (remain() < size) {
            auto it = reserves_.begin();
            if ((it == reserves_.end()) || (it->first < size)) {
                head_ = static_cast<head_t*>(alloc_.alloc(ChunkSize));
                head_->free_ = ChunkSize - aligned_head_size - size;
            }
            else {
                auto temp = it->second;
                temp->free_ -= size;
                reserves_.erase(it);
                if (remain() < temp->free_) {
                    head_ = temp;
                }
                else return base_t::buffer(temp);
            }
        }
        // size shouldn't be 0 here, otherwise behavior is undefined
        else head_->free_ -= size;
        return base_t::buffer(head_);
    }
};

} // namespace mem
} // namespace ipc
