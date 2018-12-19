#include "ipc.h"
#include "def.h"

////////////////////////////////////////////////////////////////
/// class route implementation
////////////////////////////////////////////////////////////////

namespace ipc {

class route::route_ : public pimpl<route_> {
public:
    handle_t    h_ = nullptr;
    std::string n_;
};

route::route()
    : p_(p_->make()) {
}

route::route(char const * name)
    : route() {
    this->connect(name);
}

route::route(route&& rhs)
    : route() {
    swap(rhs);
}

route::~route() {
    disconnect();
    p_->clear();
}

void route::swap(route& rhs) {
    std::swap(p_, rhs.p_);
}

route& route::operator=(route rhs) {
    swap(rhs);
    return *this;
}

bool route::valid() const {
    return (impl(p_)->h_ != nullptr);
}

char const * route::name() const {
    return impl(p_)->n_.c_str();
}

route route::clone() const {
    return { name() };
}

bool route::connect(char const * name) {
    if (name == nullptr || name[0] == '\0') return false;
    this->disconnect();
    impl(p_)->h_ = ipc::connect((impl(p_)->n_ = name).c_str());
    return valid();
}

void route::disconnect() {
    ipc::disconnect(impl(p_)->h_);
    impl(p_)->h_ = nullptr;
}

std::size_t route::recv_count() const {
    return ipc::recv_count(impl(p_)->h_);
}

bool route::send(void const *data, std::size_t size) {
    return ipc::send(impl(p_)->h_, data, size);
}

bool route::send(buff_t const & buff) {
    return route::send(buff.data(), buff.size());
}

bool route::send(std::string const & str) {
    return route::send(str.c_str(), str.size() + 1);
}

buff_t route::recv() {
    return ipc::recv(impl(p_)->h_);
}

} // namespace ipc
