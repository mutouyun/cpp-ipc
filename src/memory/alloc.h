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

class fixed_alloc_base {
protected:
    std::size_t init_expand_;
    void      * cursor_;

    void init(std::size_t init_expand) {
        init_expand_ = init_expand;
        cursor_ = nullptr;
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
        std::swap(this->cursor_     , rhs.cursor_     );
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

} // namespace detail

template <std::size_t BlockSize, typename AllocP = scope_alloc<>>
class fixed_alloc : public detail::fixed_alloc_base {
public:
    using base_t = detail::fixed_alloc_base;
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

    void* try_expand() {
        if (this->cursor_ != nullptr) {
            return this->cursor_;
        }
        auto p = this->node_p(this->cursor_ = alloc_.alloc(block_size));
        auto size = alloc_.size_of(p);
        if (size > 0) for (std::size_t i = 0; i < (size / block_size) - 1; ++i)
            p = this->node_p((*p) = reinterpret_cast<byte_t*>(p) + block_size);
        (*p) = nullptr;
        return this->cursor_;
    }

public:
    explicit fixed_alloc(std::size_t init_expand = 1) {
        this->init(init_expand);
    }

    fixed_alloc(fixed_alloc&& rhs)            { this->swap(rhs); }
    fixed_alloc& operator=(fixed_alloc&& rhs) { this->swap(rhs); return (*this); }

public:
    void swap(fixed_alloc& rhs) {
        std::swap(this->alloc_, rhs.alloc_);
        base_t::swap(rhs);
    }

    void clear() {
        alloc_.clear();
        this->init(this->init_expand_);
    }

    constexpr std::size_t size_of(void* /*p*/) const {
        return block_size;
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
/// page memory allocation
////////////////////////////////////////////////////////////////

using page_alloc = fixed_alloc<4096>;

template <std::size_t BlockSize>
using page_fixed_alloc = fixed_alloc<BlockSize, page_alloc>;

} // namespace mem
} // namespace ipc
