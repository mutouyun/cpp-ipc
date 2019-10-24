#pragma once

#include <cstddef>
#include <tuple>
#include <vector>
#include <type_traits>

#include "export.h"
#include "def.h"

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
    auto data() noexcept -> typename std::enable_if<!std::is_const<T>::value, T*>::type {
        return static_cast<T*>(data());
    }

    template <typename T>
    auto data() const noexcept -> typename std::enable_if<std::is_const<T>::value, T*>::type {
        return static_cast<T*>(data());
    }

    std::size_t size() const noexcept;

    std::tuple<void*, std::size_t> to_tuple() {
        return std::make_tuple(data(), size());
    }

    std::tuple<void const *, std::size_t> to_tuple() const {
        return std::make_tuple(data(), size());
    }

    std::vector<byte_t> to_vector() const {
#   if __cplusplus >= 201703L
        auto [d, s] = to_tuple();
#   else /*__cplusplus < 201703L*/
        auto tp = to_tuple();
        auto d  = std::get<0>(tp);
        auto s  = std::get<1>(tp);
#   endif/*__cplusplus < 201703L*/
        return {
            static_cast<byte_t const *>(d),
            static_cast<byte_t const *>(d) + s
        };
    }

    friend IPC_EXPORT bool operator==(buffer const & b1, buffer const & b2);
    friend IPC_EXPORT bool operator!=(buffer const & b1, buffer const & b2);

private:
    class buffer_;
    buffer_* p_;
};

} // namespace ipc
