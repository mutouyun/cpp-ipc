#include <cstring>

#include "def.h"

namespace ipc {

class id_pool {
public:
    enum : std::size_t {
        max_count = (std::numeric_limits<uint_t<8>>::max)() // 255
    };

private:
    uint_t<8> cursor_ = 0;
    uint_t<8> block_[max_count] {};

public:
    void init() {
        for (std::size_t i = 0; i < max_count;) {
            i = block_[i] = static_cast<uint_t<8>>(i + 1);
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
        cursor_ = block_[id]; // point to next
        block_[id] = 0;       // clear flag
        return id;
    }

    void release(std::size_t id) {
        block_[id] = cursor_;
        cursor_ = static_cast<uint_t<8>>(id); // put it back
    }

    template <typename F>
    void for_each(F&& fr) {
        for (std::size_t i = 0; i < max_count; ++i) {
            fr(i, block_[i] == 0);
        }
    }
};

} // namespace ipc
