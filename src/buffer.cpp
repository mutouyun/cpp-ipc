#include "buffer.h"
#include "pimpl.h"

#include <cstring>

namespace ipc {

bool operator==(buffer const & b1, buffer const & b2) {
    return (b1.size() == b2.size()) && (std::memcmp(b1.data(), b2.data(), b1.size()) == 0);
}

class buffer::buffer_ : public pimpl<buffer_> {
public:
    void*        p_;
    std::size_t  s_;
    destructor_t d_;

    buffer_(void* p, std::size_t s, destructor_t d)
        : p_(p), s_(s), d_(d) {
    }

    ~buffer_() {
        if (d_ == nullptr) return;
        d_(p_, s_);
    }
};

buffer::buffer()
    : buffer(nullptr, 0, nullptr) {
}

buffer::buffer(void* p, std::size_t s, destructor_t d)
    : p_(p_->make(p, s, d)) {
}

buffer::buffer(void* p, std::size_t s)
    : buffer(p, s, nullptr) {
}

buffer::buffer(char const & c)
    : buffer(const_cast<char*>(&c), 1, nullptr) {
}

buffer::buffer(buffer&& rhs)
    : buffer() {
    swap(rhs);
}

buffer::~buffer() {
    p_->clear();
}

void buffer::swap(buffer& rhs) {
    std::swap(p_, rhs.p_);
}

buffer& buffer::operator=(buffer rhs) {
    swap(rhs);
    return *this;
}

bool buffer::empty() const noexcept {
    return (impl(p_)->p_ == nullptr) || (impl(p_)->s_ == 0);
}

void* buffer::data() noexcept {
    return impl(p_)->p_;
}

void const * buffer::data() const noexcept {
    return impl(p_)->p_;
}

std::size_t buffer::size() const noexcept {
    return impl(p_)->s_;
}

} // namespace ipc
