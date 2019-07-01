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

public:
    scope_alloc() = default;

    scope_alloc(scope_alloc&& rhs)            { this->swap(rhs); }
    scope_alloc& operator=(scope_alloc&& rhs) { this->swap(rhs); return (*this); }

    ~scope_alloc() { clear(); }

    void swap(scope_alloc& rhs) {
        std::swap(this->alloc_, rhs.alloc_);
        base_t::swap(rhs);
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

template <std::size_t BlockSize>
struct fixed_expand_policy {
    static std::size_t next(std::size_t & e) {
        return (ipc::detail::max)(BlockSize, static_cast<std::size_t>(2048)) * (e *= 2);
    }
};

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

} // namespace detail

template <std::size_t BlockSize,
          template <std::size_t> class ExpandP = detail::fixed_expand_policy,
          typename AllocP = scope_alloc<>>
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
        auto size = ExpandP<block_size>::next(this->init_expand_);
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

    fixed_alloc(fixed_alloc&& rhs)            { this->swap(rhs); }
    fixed_alloc& operator=(fixed_alloc&& rhs) { this->swap(rhs); return (*this); }

    void swap(fixed_alloc& rhs) {
        std::swap(this->alloc_, rhs.alloc_);
        base_t::swap(rhs);
    }

    void clear() {
        alloc_.clear();
        this->init(this->init_expand_);
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
/// Variable-size blocks allocation
////////////////////////////////////////////////////////////////

namespace detail {

class variable_alloc_base {
protected:
    struct head_t {
        head_t * next_;
        size_t   size_;
        size_t   free_;
    };

    char * head_, * tail_;

    void init() {
        // makes chain to nullptr
        head_ = tail_ = reinterpret_cast<char*>(sizeof(head_t));
    }

    head_t* chain() {
        return reinterpret_cast<head_t*>(head_ - sizeof(head_t));
    }

public:
    void swap(variable_alloc_base& rhs) {
        std::swap(this->head_, rhs.head_);
        std::swap(this->tail_, rhs.tail_);
    }

    void free(void* /*p*/) {}
    void free(void* /*p*/, std::size_t) {}
};

} // namespace detail

template <std::size_t ChunkSize = 4096, typename AllocP = scope_alloc<>>
class variable_alloc : public detail::variable_alloc_base {
public:
    using base_t = detail::variable_alloc_base;
    using head_t = base_t::head_t;
    using alloc_policy = AllocP;

private:
    alloc_policy alloc_;

    head_t* alloc_head(std::size_t size) {
        size = (ipc::detail::max)(ChunkSize, (ipc::detail::max)(size, sizeof(head_t)));
        head_t* p = static_cast<head_t*>(alloc_.alloc(size));
        p->free_ = (p->size_ = size) - sizeof(head_t);
        return p;
    }

    void free_head(head_t* curr) {
        alloc_.free(curr, curr->size_);
    }

    std::size_t remain() const {
        return (tail_ - head_);
    }

    void* alloc_new_chunk(std::size_t size) {
        head_t* p = alloc_head(sizeof(head_t) + size);
        if (p == nullptr) return nullptr;
        head_t* list = chain();
        if (size > (ChunkSize - sizeof(head_t)) && list != nullptr) {
            p->next_ = list->next_;
            list->next_ = p;
            char* head = reinterpret_cast<char*>(p + 1);
            char* tail = head + p->free_ - size;
            p->free_ = tail - head;
            return tail;
        }
        else {
            p->next_ = list;
            head_ = reinterpret_cast<char*>(p + 1);
            tail_ = head_ + p->free_ - size;
            p->free_ = remain();
            return tail_;
        }
    }

public:
    variable_alloc() { this->init(); }

    variable_alloc(variable_alloc&& rhs)            { this->swap(rhs); }
    variable_alloc& operator=(variable_alloc&& rhs) { this->swap(rhs); return (*this); }

    ~variable_alloc() { clear(); }

    void swap(variable_alloc& rhs) {
        std::swap(this->alloc_, rhs.alloc_);
        base_t::swap(rhs);
    }

    void clear() {
        head_t* list = chain();
        while (list != nullptr) {
            head_t* curr = list;
            list = list->next_;
            free_head(curr);
        }
        alloc_.clear();
        this->init();
    }

    void* alloc(size_t size) {
        if (remain() < size) {
            return alloc_new_chunk(size);
        }
        char* buff = tail_ - size;
        if (buff < head_) {
            return alloc_new_chunk(size);
        }
        tail_ = buff;
        chain()->free_ = remain();
        return tail_;
    }
};

} // namespace mem
} // namespace ipc
