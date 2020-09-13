#pragma once

#include <cstddef>
#include <tuple>
#include <vector>
#include <type_traits>

#include "libipc/export.h"
#include "libipc/def.h"

namespace ipc {

class IPC_EXPORT buffer {
public:
    using destructor_t = void (*)(void*, std::size_t);

    buffer();

    buffer(void* p, std::size_t s, destructor_t d);
    buffer(void* p, std::size_t s, destructor_t d, void* additional);
    buffer(void* p, std::size_t s);

    template <std::size_t N>
    explicit buffer(byte_t const (& data)[N])
        : buffer(data, sizeof(data)) {
    }
    explicit buffer(char const & c);

    buffer(buffer&& rhs);
    ~buffer();

    void swap(buffer& rhs);
    buffer& operator=(buffer rhs);

    bool empty() const noexcept;

    void *       data()       noexcept;
    void const * data() const noexcept;

    template <typename T>
    T get() const { return T(data()); }

    std::size_t size() const noexcept;

    std::tuple<void*, std::size_t> to_tuple() {
        return std::make_tuple(data(), size());
    }

    std::tuple<void const *, std::size_t> to_tuple() const {
        return std::make_tuple(data(), size());
    }

    std::vector<byte_t> to_vector() const {
        return {
            get<byte_t const *>(),
            get<byte_t const *>() + size()
        };
    }

    friend IPC_EXPORT bool operator==(buffer const & b1, buffer const & b2);
    friend IPC_EXPORT bool operator!=(buffer const & b1, buffer const & b2);

private:
    class buffer_;
    buffer_* p_;
};

} // namespace ipc
