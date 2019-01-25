#pragma once

#include <cstring>

#include "def.h"

namespace ipc {

class id_pool {
public:
    enum : std::size_t {
        max_count = (std::numeric_limits<uint_t<8>>::max)() // 255
    };

private:
    uint_t<8> acquir_ = 0;
    uint_t<8> cursor_ = 0;
    uint_t<8> next_[max_count] {};

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
        if (acquir_ == max_count) return false;
        if (acquir_ == id) {
            acquir_ = next_[id]; // point to next
        }
        else {
            auto a = next_[acquir_], l = acquir_;
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
            fr(a);
            a = next_[a];
        }
    }
};

} // namespace ipc
