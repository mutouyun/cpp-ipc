
#include "circ_queue.h"

namespace ipc {

class circ_queue_ {
public:
};

circ_queue::circ_queue(void)
    : p_(new circ_queue_) {
}

circ_queue::circ_queue(void* mem, std::size_t size)
    : circ_queue() {
    attach(mem, size);
}

circ_queue::circ_queue(circ_queue&& rhs)
    : circ_queue() {
    swap(rhs);
}

circ_queue::~circ_queue(void) {
    delete p_;
}

void circ_queue::swap(circ_queue& rhs) {
    std::swap(p_, rhs.p_);
}

circ_queue& circ_queue::operator=(circ_queue rhs) {
    swap(rhs);
    return *this;
}

bool circ_queue::valid(void) const {
    return (p_ != nullptr) && (p_->elem_start_ != nullptr) && (p_->elem_size_ != 0);
}

std::size_t circ_queue::head_size(void) const {
    if (!valid()) return 0;
    return reinterpret_cast<std::uint8_t*>(p_->start_) - p_->elem_start_;
}

std::size_t circ_queue::elem_size(void) const {
    if (!valid()) return 0;
    return p_->elem_size_;
}

bool circ_queue::attach(void* mem, std::size_t size) {
    if (p_ == nullptr) return false;
    if ((mem == nullptr) || (size == 0)) return false;
    if (size < p_->size_min) return false;

    detach();

    p_->start_ = static_cast<std::atomic_uint8_t*>(mem);
    p_->count_ = p_->start_ + 1;
    p_->flag_  = reinterpret_cast<std::atomic_flag*>(p_->count_ + 1);

    std::size_t hs  = std::max(size / (p_->elem_max + 1), static_cast<std::size_t>(p_->head_size));
    p_->elem_start_ = reinterpret_cast<std::uint8_t*>(p_->start_) + hs;
    p_->elem_size_  = (size - hs) / p_->elem_max;

    p_->lc_start_ = p_->start_->load();
    p_->lc_count_ = p_->count_->load();
    return true;
}

void circ_queue::detach(void) {
    if (!valid()) return;
    p_->elem_start_ = nullptr;
    p_->elem_size_  = 0;
    p_->start_      = nullptr;
    p_->count_      = nullptr;
    p_->lc_start_   = 0;
    p_->lc_count_   = 0;
}

void* circ_queue::alloc(void) {

}

bool circ_queue::push(void* elem) {
    if (!valid()) return false;
    if (elem == nullptr) return false;

    do {
        std::uint8_t offset = p_->start_->load() + p_->count_->load(); // overflow is mod
        std::size_t  lastps = offset * p_->elem_size_;
        if (p_->flag_->test_and_set(std::memory_order_acquire)) {
            continue;
        }
        memcpy(p_->elem_start_ + lastps, elem, p_->elem_size_);
    } while(1);

    return true;
}

} // namespace ipc
