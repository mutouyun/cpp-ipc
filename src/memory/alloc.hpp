#pragma once

#include <algorithm>
#include <utility>
#include <cstdlib>

#include "def.h"

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

class fixed_pool_base {
protected:
    std::size_t init_expand_;
    std::size_t iter_;
    void*       cursor_;

    void init(std::size_t init_expand) {
        iter_ = init_expand_ = init_expand;
        cursor_ = nullptr;
    }

    static void** node_p(void* node) {
        return reinterpret_cast<void**>(node);
    }

    static auto& next(void* node) {
        return *node_p(node);
    }

public:
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
class fixed_pool : public detail::fixed_pool_base {
public:
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

    void expand() {
        auto p = node_p(cursor_ = alloc_.alloc(block_size * iter_));
        for (std::size_t i = 0; i < iter_ - 1; ++i)
            p = node_p((*p) = reinterpret_cast<byte_t*>(p) + block_size);
        (*p) = nullptr;
        iter_ *= 2;
    }

public:
    explicit fixed_pool(std::size_t init_expand = 1) {
        init(init_expand);
    }

    fixed_pool(fixed_pool&& rhs)            { this->swap(rhs); }
    fixed_pool& operator=(fixed_pool&& rhs) { this->swap(rhs); return (*this); }

public:
    void swap(fixed_pool& rhs) {
        std::swap(this->alloc_      , rhs.alloc_);
        std::swap(this->init_expand_, rhs.init_expand_);
        std::swap(this->iter_       , rhs.iter_);
        std::swap(this->cursor_     , rhs.cursor_);
    }

    void clear() {
        alloc_.clear();
        init(init_expand_);
    }

    void* alloc() {
        if (cursor_ == nullptr) expand();
        void* p = cursor_;
        cursor_ = next(p);
        return p;
    }

    void* alloc(std::size_t) {
        return alloc();
    }
};

} // namespace mem
} // namespace ipc
