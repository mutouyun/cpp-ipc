#pragma once

#include <type_traits>  // std::aligned_storage_t
#include <cstring>      // std::memcmp
#include <cstdint>

#include "libipc/def.h"
#include "libipc/platform/detail.h"

namespace ipc {

using storage_id_t = std::int32_t;

template <std::size_t DataSize, std::size_t AlignSize>
struct id_type;

template <std::size_t AlignSize>
struct id_type<0, AlignSize> {
    uint_t<8> id_;

    id_type& operator=(storage_id_t val) {
        id_ = static_cast<uint_t<8>>(val);
        return (*this);
    }

    operator uint_t<8>() const {
        return id_;
    }
};

template <std::size_t DataSize, std::size_t AlignSize>
struct id_type : id_type<0, AlignSize> {
    std::aligned_storage_t<DataSize, AlignSize> data_;
};

template <std::size_t DataSize  = 0,
          std::size_t AlignSize = (ipc::detail::min)(DataSize, alignof(std::max_align_t))>
class id_pool {

    static constexpr std::size_t limited_max_count() {
        return ipc::detail::min<std::size_t>(large_msg_cache, (std::numeric_limits<uint_t<8>>::max)());
    }

public:
    enum : std::size_t {
        /* eliminate error: taking address of temporary */
        max_count = limited_max_count()
    };

private:
    id_type<DataSize, AlignSize> next_[max_count];
    uint_t<8> cursor_ = 0;
    bool prepared_ = false;

public:
    void prepare() {
        if (!prepared_ && this->invalid()) this->init();
        prepared_ = true;
    }

    void init() {
        for (storage_id_t i = 0; i < max_count;) {
            i = next_[i] = (i + 1);
        }
    }

    bool invalid() const {
        static id_pool inv;
        return std::memcmp(this, &inv, sizeof(id_pool)) == 0;
    }

    bool empty() const {
        return cursor_ == max_count;
    }

    storage_id_t acquire() {
        if (empty()) return -1;
        storage_id_t id = cursor_;
        cursor_ = next_[id]; // point to next
        return id;
    }

    bool release(storage_id_t id) {
        if (id < 0) return false;
        next_[id] = cursor_;
        cursor_ = static_cast<uint_t<8>>(id); // put it back
        return true;
    }

    void       * at(storage_id_t id)       { return &(next_[id].data_); }
    void const * at(storage_id_t id) const { return &(next_[id].data_); }
};

template <typename T>
class obj_pool : public id_pool<sizeof(T), alignof(T)> {
    using base_t = id_pool<sizeof(T), alignof(T)>;

public:
    T       * at(storage_id_t id)       { return reinterpret_cast<T       *>(base_t::at(id)); }
    T const * at(storage_id_t id) const { return reinterpret_cast<T const *>(base_t::at(id)); }
};

} // namespace ipc
