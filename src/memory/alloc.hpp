#pragma once

#include <limits>
#include <algorithm>
#include <cstdlib>

#include "def.h"

namespace ipc {
namespace memory {

struct static_alloc {
    static constexpr std::size_t remain() {
        return (std::numeric_limits<std::size_t>::max)();
    }

    static constexpr void clear() {}

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

template <typename AllocP = static_alloc>
class scope_alloc {
public:
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

    struct block_t {
        block_t* next_;
    };
    block_t* list_ = nullptr;

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

    std::size_t remain() const {
        std::size_t c = 0;
        auto curr = list_;
        while (curr != nullptr) {
            ++c;
            curr = curr->next_;
        }
        return c;
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

    void free(void* /*p*/) {}
};

////////////////////////////////////////////////////////////////
/// Fixed-size blocks allocation
////////////////////////////////////////////////////////////////

template <std::size_t BlockSize, typename AllocP = scope_alloc<>>
class fixed_pool {
public:
    using alloc_policy = AllocP;

    enum : std::size_t {
        block_size = (std::max)(BlockSize, sizeof(void*))
    };

private:
    alloc_policy alloc_;
    std::size_t init_expand_, iter_;
    void*  cursor_;

    void expand() {
        void** p = reinterpret_cast<void**>(cursor_ = alloc_.alloc(block_size * iter_));
        for (std::size_t i = 0; i < iter_ - 1; ++i)
            p = reinterpret_cast<void**>((*p) = reinterpret_cast<byte_t*>(p) + block_size);
        (*p) = nullptr;
        iter_ *= 2;
    }

    void init(std::size_t init_expand) {
        iter_ = init_expand_ = init_expand;
        cursor_ = nullptr;
    }

public:
    explicit fixed_pool(std::size_t init_expand = 1) {
        init(init_expand);
    }

    fixed_pool(fixed_pool&& rhs)            { this->swap(rhs); }
    fixed_pool& operator=(fixed_pool&& rhs) { this->swap(rhs); return (*this); }

    ~fixed_pool() { clear(); }

public:
    void swap(fixed_pool& rhs) {
        std::swap(this->alloc_      , rhs.alloc_);
        std::swap(this->init_expand_, rhs.init_expand_);
        std::swap(this->iter_       , rhs.iter_);
        std::swap(this->cursor_     , rhs.cursor_);
    }

    std::size_t remain() const {
        std::size_t c = 0;
        void* curr = cursor_;
        while (curr != nullptr) {
            ++c;
            curr = *reinterpret_cast<void**>(curr); // curr = next
        }
        return c * block_size;
    }

    void clear() {
        alloc_.clear();
        init(init_expand_);
    }

    void* alloc() {
        if (cursor_ == nullptr) expand();
        void* p = cursor_;
        cursor_ = *reinterpret_cast<void**>(p);
        return p;
    }

    void free(void* p) {
        if (p == nullptr) return;
        *reinterpret_cast<void**>(p) = cursor_;
        cursor_ = p;
    }
};

} // namespace memory
} // namespace ipc
