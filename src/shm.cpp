#include "shm.h"

#include <utility>

namespace ipc {
namespace shm {

class handle_ {
public:
    handle*     t_ = nullptr;
    handle_t    h_ = nullptr;
    void*       m_ = nullptr;

    std::string n_;
    std::size_t s_ = 0;

    ~handle_(void) {
        t_->close();
        t_->release();
    }
};

handle::handle(void)
    : p_(new handle_) {
    p_->t_ = this;
}

handle::handle(std::string const & name, std::size_t size)
    : handle() {
    acquire(name, size);
}

handle::handle(handle&& rhs)
    : handle() {
    swap(rhs);
}

handle::~handle(void) {
    delete p_;
}

void handle::swap(handle& rhs) {
    std::swap(p_, rhs.p_);
}

handle& handle::operator=(handle rhs) {
    swap(rhs);
    return *this;
}

bool handle::valid(void) const {
    return (p_ != nullptr) && (p_->h_ != nullptr);
}

std::size_t handle::size(void) const {
    return (p_ == nullptr) ? 0 : p_->s_;
}

std::string const & handle::name(void) const {
    static const std::string dummy;
    return (p_ == nullptr) ? dummy : p_->n_;
}

bool handle::acquire(std::string const & name, std::size_t size) {
    if (p_ == nullptr) return false;
    close();
    release();
    p_->h_ = shm::acquire(p_->n_ = name, p_->s_ = size);
    return valid();
}

void handle::release(void) {
    if (!valid()) return;
    shm::release(p_->h_, p_->s_);
    p_->h_ = nullptr;
    p_->s_ = 0;
    p_->n_.clear();
}

void* handle::get(void) {
    if (!valid()) return nullptr;
    if (p_->m_ == nullptr) {
        return p_->m_ = shm::open(p_->h_);
    }
    else return p_->m_;
}

void handle::close(void) {
    if (!valid()) return;
    shm::close(p_->m_);
    p_->m_ = nullptr;
}

} // namespace shm
} // namespace ipc
