#include "ipc.h"

#include <atomic>
#include <string>

#include "def.h"
#include "shm.h"

namespace {

using namespace ipc;

struct ch_info_t {
    std::atomic<uint_t<8>> ch_acc_; // only support 256 channels with one name
};

} // internal-linkage

////////////////////////////////////////////////////////////////
/// class channel implementation
////////////////////////////////////////////////////////////////

namespace ipc {

class channel::channel_ : public pimpl<channel_> {
public:
    shm::handle h_;
    route       r_;

    ch_info_t* info() {
        return static_cast<ch_info_t*>(h_.get());
    }

    auto& acc() {
        return info()->ch_acc_;
    }
};

channel::channel()
    : p_(p_->make()) {
}

channel::channel(char const * name)
    : channel() {
    this->connect(name);
}

channel::channel(channel&& rhs)
    : channel() {
    swap(rhs);
}

channel::~channel() {
    disconnect();
    p_->clear();
}

void channel::swap(channel& rhs) {
    std::swap(p_, rhs.p_);
}

channel& channel::operator=(channel rhs) {
    swap(rhs);
    return *this;
}

bool channel::valid() const {
    return impl(p_)->h_.valid() && impl(p_)->r_.valid();
}

char const * channel::name() const {
    std::string n { impl(p_)->h_.name() };
    n.pop_back();
    return n.c_str();
}

channel channel::clone() const {
    return { name() };
}

bool channel::connect(char const * name) {
    if (name == nullptr || name[0] == '\0') {
        return false;
    }
    this->disconnect();
    using namespace std::literals::string_literals;
    if (!impl(p_)->h_.acquire((name + "_"s).c_str(), sizeof(ch_info_t))) {
        return false;
    }
    auto cur_id = impl(p_)->acc().fetch_add(1, std::memory_order_relaxed);
    impl(p_)->r_.connect((name + std::to_string(cur_id)).c_str());
    return valid();
}

void channel::disconnect() {
    impl(p_)->r_.disconnect();
    impl(p_)->h_.release();
}

std::size_t channel::recv_count() const {
    return 0;
}

bool channel::send(void const *data, std::size_t size) {
    return false;
}

bool channel::send(buff_t const & buff) {
    return false;
}

bool channel::send(std::string const & str) {
    return false;
}

buff_t channel::recv() {
    return {};
}

} // namespace ipc
