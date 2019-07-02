#pragma once

#include <algorithm>
#include <utility>
#include <cstdlib>

#include "def.h"
#include "rw_lock.h"

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

class scope_alloc_base {
protected:
    struct block_t {
        block_t   * next_;
        std::size_t size_;
    } * list_ = nullptr;

    enum : std::size_t {
        aligned_block_size = aligned(sizeof(block_t), alignof(std::max_align_t))
    };

public:
    void swap(scope_alloc_base & rhs) {
        std::swap(this->list_, rhs.list_);
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
        while (list_ != nullptr) {
            auto curr = list_;
            list_ = list_->next_;
            alloc_.free(curr, curr->size_);
        }
        // now list_ is nullptr
    }

public:
    scope_alloc() = default;

    scope_alloc(scope_alloc&& rhs)            { this->swap(rhs); }
    scope_alloc& operator=(scope_alloc&& rhs) { this->swap(rhs); return (*this); }

    ~scope_alloc() { free_all(); }

    template <typename A>
    void set_allocator(A && alc) {
        alloc_ = std::forward<A>(alc);
    }

    void swap(scope_alloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    void clear() {
        free_all();
        alloc_.~alloc_policy();
    }

    void* alloc(std::size_t size) {
        auto curr = static_cast<block_t*>(alloc_.alloc(size += aligned_block_size));
        curr->next_ = list_;
        curr->size_ = size;
        return (reinterpret_cast<byte_t*>(list_ = curr) + aligned_block_size);
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
        std::swap(this->init_expand_, rhs.init_expand_);
        std::swap(this->cursor_     , rhs.cursor_);
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
        if (this->cursor_ != nullptr) {
            return this->cursor_;
        }
        auto size = ExpandP::template next<block_size>(this->init_expand_);
        auto p = this->node_p(this->cursor_ = alloc_.alloc(size));
        for (std::size_t i = 0; i < (size / block_size) - 1; ++i)
            p = this->node_p((*p) = reinterpret_cast<byte_t*>(p) + block_size);
        (*p) = nullptr;
        return this->cursor_;
    }

public:
    explicit fixed_alloc(std::size_t init_expand = 1) {
        this->init(init_expand);
    }

    fixed_alloc(fixed_alloc&& rhs) : fixed_alloc() { this->swap(rhs); }
    fixed_alloc& operator=(fixed_alloc&& rhs)      { this->swap(rhs); return (*this); }

    template <typename A>
    void set_allocator(A && alc) {
        alloc_ = std::forward<A>(alc);
    }

    void swap(fixed_alloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    void clear() {
        ExpandP::prev(this->init_expand_);
        this->cursor_ = nullptr;
        alloc_.~alloc_policy();
    }

    void* alloc() {
        void* p = try_expand();
        this->cursor_ = this->next(p);
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
        head_t * next_;
        size_t   size_;
        size_t   free_;
    } * head_ = nullptr;

    enum : std::size_t {
        aligned_head_size = aligned(sizeof(head_t), alignof(std::max_align_t))
    };

    static byte_t * buffer(head_t* p) {
        return reinterpret_cast<byte_t*>(p) + aligned_head_size;
    }

public:
    void swap(variable_alloc_base& rhs) {
        std::swap(this->head_, rhs.head_);
    }

    void free(void* /*p*/) {}
    void free(void* /*p*/, std::size_t) {}
};

} // namespace detail

template <std::size_t ChunkSize = (sizeof(void*) * 1024), typename AllocP = static_alloc>
class variable_alloc : public detail::variable_alloc_base {
public:
    using base_t = detail::variable_alloc_base;
    using head_t = base_t::head_t;
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

    head_t* alloc_head(std::size_t size) {
        size = (ipc::detail::max)(ChunkSize, ipc::detail::max<std::size_t>(size, aligned_head_size));
        head_t* p = static_cast<head_t*>(alloc_.alloc(size));
        p->free_ = (p->size_ = size) - aligned_head_size;
        return p;
    }

    void* alloc_new_chunk(std::size_t size) {
        head_t* p = alloc_head(aligned_head_size + size);
        if (p == nullptr) return nullptr;
        if (size > (ChunkSize - aligned_head_size) && head_ != nullptr) {
            p->next_ = head_->next_;
            head_->next_ = p;
            return base_t::buffer(p) + (p->free_ -= size);
        }
        p->next_ = head_;
        return base_t::buffer(head_ = p) + (p->free_ -= size);
    }

    void free_all() {
        while (head_ != nullptr) {
            head_t* curr = head_;
            head_ = head_->next_;
            alloc_.free(curr, curr->size_);
        }
        // now head_ is nullptr
    }

public:
    variable_alloc() = default;

    variable_alloc(variable_alloc&& rhs)            { this->swap(rhs); }
    variable_alloc& operator=(variable_alloc&& rhs) { this->swap(rhs); return (*this); }

    ~variable_alloc() { free_all(); }

    template <typename A>
    void set_allocator(A && alc) {
        alloc_ = std::forward<A>(alc);
    }

    void swap(variable_alloc& rhs) {
        alloc_.swap(rhs.alloc_);
        base_t::swap(rhs);
    }

    void clear() {
        free_all();
        alloc_.~alloc_policy();
    }

    void* alloc(size_t size) {
        if ((head_ == nullptr) || head_->free_ < size) {
            return alloc_new_chunk(size);
        }
        return base_t::buffer(head_) + (head_->free_ -= size);
    }
};

} // namespace mem
} // namespace ipc
