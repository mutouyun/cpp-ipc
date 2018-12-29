#include "ipc.h"

#include <atomic>
#include <string>
#include <array>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <memory>
#include <utility>

#include "def.h"
#include "shm.h"
#include "rw_lock.h"

#include "id_pool.inc"

namespace {

using namespace ipc;

#pragma pack(1)
struct ch_info_t {
    rw_lock lc_;
    id_pool ch_acc_; // only support 255 channels with one name
};
#pragma pack()

inline bool wait_for_recv(route const & rt, std::size_t r_count, std::size_t until) {
    for (unsigned k = 0; rt.recv_count() < r_count; ++k) {
        if (k > until) return false;
        std::this_thread::yield();
    }
    return true;
}

template <typename... P>
inline bool channel_send(route& rt, std::size_t r_count, P&&... params) {
    if (!wait_for_recv(rt, r_count, 1024)) return false;
    return rt.send(params...); // no need std::forward
}

} // internal-linkage

////////////////////////////////////////////////////////////////
/// class channel implementation
////////////////////////////////////////////////////////////////

namespace ipc {

class channel::channel_ : public pimpl<channel_> {
public:
    shm::handle h_;
    route       r_;

    std::string n_;
    std::size_t id_;
    bool        marked_ = false;

    std::array<route, id_pool::max_count> rts_;

    ch_info_t& info() {
        return *static_cast<ch_info_t*>(h_.get());
    }

    auto& acc() {
        return info().ch_acc_;
    }

    void mark_id() {
        if (marked_) return;
        marked_ = true;
        [[maybe_unused]] auto guard = std::unique_lock { info().lc_ };
        acc().mark_acquired(id_);
    }

    auto& sender() {
        mark_id();
        return r_;
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
    return impl(p_)->n_.c_str();
}

channel channel::clone() const {
    return channel { name() };
}

bool channel::connect(char const * name) {
    if (name == nullptr || name[0] == '\0') {
        return false;
    }
    this->disconnect();
    if (!impl(p_)->h_.acquire(((impl(p_)->n_ = name) + "_").c_str(), sizeof(ch_info_t))) {
        return false;
    }
    {
        [[maybe_unused]] auto guard = std::unique_lock { impl(p_)->info().lc_ };
        if (impl(p_)->acc().invalid()) {
            impl(p_)->acc().init();
        }
        impl(p_)->id_ = impl(p_)->acc().acquire();
    }
    if (impl(p_)->id_ == invalid_value) {
        return false;
    }
    impl(p_)->r_.connect((name + std::to_string(impl(p_)->id_)).c_str());
    return valid();
}

void channel::disconnect() {
    if (!valid()) return;
    {
        [[maybe_unused]] auto guard = std::unique_lock { impl(p_)->info().lc_ };
        impl(p_)->acc().release(impl(p_)->id_);
    }
    for (auto& rt : impl(p_)->rts_) {
        rt.disconnect();
    }
    impl(p_)->r_.disconnect();
    impl(p_)->h_.release();
}

std::size_t channel::recv_count() const {
    return impl(p_)->r_.recv_count();
}

bool channel::wait_for_recv(std::size_t r_count, std::size_t until) const {
    return ::wait_for_recv(impl(p_)->sender(), r_count, until);
}

bool channel::wait_for_recv(std::size_t r_count) const {
    return wait_for_recv(r_count, (std::numeric_limits<std::size_t>::max)());
}

bool channel::send(void const * data, std::size_t size) {
    return impl(p_)->sender().send(data, size);
}

bool channel::send(buff_t const & buff) {
    return impl(p_)->sender().send(buff);
}

bool channel::send(std::string const & str) {
    return impl(p_)->sender().send(str);
}

bool channel::send_for(std::size_t r_count, void const * data, std::size_t size) {
    return ::channel_send(impl(p_)->sender(), r_count, data, size);
}

bool channel::send_for(std::size_t r_count, buff_t const & buff) {
    return ::channel_send(impl(p_)->sender(), r_count, buff);
}

bool channel::send_for(std::size_t r_count, std::string const & str) {
    return ::channel_send(impl(p_)->sender(), r_count, str);
}

buff_t channel::recv() {
    if (!valid()) return {};
    std::array<queue_t*, id_pool::max_count> ques;
    return ipc::multi_recv([&] {
        std::array<std::size_t, id_pool::max_count> acqs;
        std::size_t counter = 0;
        // get all acquired ids
        {
            [[maybe_unused]] auto guard = std::shared_lock { impl(p_)->info().lc_ };
            impl(p_)->acc().for_acquired([this, &acqs, &counter](std::size_t id) {
                if (id == impl(p_)->id_) return;
                acqs[counter++] = id;
            });
        }
        // populate route cache & ques
        for (std::size_t k = 0; k < counter; ++k) {
            std::size_t id = acqs[k];
            auto& it = impl(p_)->rts_[id];
            // it's a new id
            if (!it.valid()) {
                it.connect((impl(p_)->n_ + std::to_string(id)).c_str());
                queue_of(it.handle())->connect();
            }
            // get queue of this route
            ques[k] = queue_of(it.handle());
        }
        return std::make_tuple(ques.data(), counter);
    });
}

} // namespace ipc
