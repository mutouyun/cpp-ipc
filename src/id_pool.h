#pragma once

#include <cstring>
#include <algorithm>

#include "def.h"

namespace ipc {

template <std::size_t DataSize, std::size_t AlignSize>
struct id_type {
    uint_t<8> id_;
    alignas(AlignSize) byte_t data_[DataSize] {};

    id_type& operator=(uint_t<8> val) {
        id_ = val;
        return (*this);
    }

    operator uint_t<8>() const {
        return id_;
    }
};

template <std::size_t AlignSize>
struct id_type<0, AlignSize> {
    uint_t<8> id_;

    id_type& operator=(uint_t<8> val) {
        id_ = val;
        return (*this);
    }

    operator uint_t<8>() const {
        return id_;
    }
};

template <std::size_t DataSize  = 0,
#if __cplusplus >= 201703L
          std::size_t AlignSize = (std::min)(DataSize, alignof(std::size_t))>
#else /*__cplusplus < 201703L*/
          std::size_t AlignSize = (alignof(std::size_t) < DataSize) ? alignof(std::size_t) : DataSize>
#endif/*__cplusplus < 201703L*/
class id_pool {
public:
    enum : std::size_t {
        max_count = (std::numeric_limits<uint_t<8>>::max)() // 255
    };

private:
    id_type<DataSize, AlignSize> next_[max_count];

    uint_t<8> acquir_ = 0;
    uint_t<8> cursor_ = 0;

public:
    void init() {
        acquir_ = max_count;
        for (std::size_t i = 0; i < max_count;) {
            i = next_[i] = static_cast<uint_t<8>>(i + 1);
        }
    }

    bool invalid() const {
        static id_pool inv;
        return std::memcmp(this, &inv, sizeof(id_pool)) == 0;
    }

    bool empty() const {
        return cursor_ == max_count;
    }

    std::size_t acquire() {
        if (empty()) {
            return invalid_value;
        }
        std::size_t id = cursor_;
        cursor_ = next_[id]; // point to next
        return id;
    }

    void mark_acquired(std::size_t id) {
        next_[id] = acquir_;
        acquir_ = static_cast<uint_t<8>>(id); // put it in acquired list
    }

    bool release(std::size_t id) {
        if (id == invalid_value) return false;
        if (acquir_ == max_count) return false;
        if (acquir_ == id) {
            acquir_ = next_[id]; // point to next
        }
        else {
            uint_t<8> a = next_[acquir_], l = acquir_;
            while (1) {
                if (a == max_count) {
                    return false; // found nothing
                }
                if (a == id) {
                    next_[l] = next_[a];
                    break;
                }
                l = a;
                a = next_[a];
            }
        }
        next_[id] = cursor_;
        cursor_ = static_cast<uint_t<8>>(id); // put it back
        return true;
    }

    template <typename F>
    void for_acquired(F&& fr) {
        auto a = acquir_;
        while (a != max_count) {
            if (!fr(a)) return;
            a = next_[a];
        }
    }

    void* at(std::size_t id) {
        return next_[id].data_;
    }
};

} // namespace ipc
