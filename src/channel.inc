#include "ipc.h"

#include <atomic>
#include <string>
#include <array>
#include <limits>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <thread>

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

    std::unordered_map<std::size_t, route> rts_;

    ~channel_(void) { rts_.clear(); }

    ch_info_t& info() {
        return *static_cast<ch_info_t*>(h_.get());
    }

    auto& acc() {
        return info().ch_acc_;
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
    return { name() };
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
        std::unique_lock<rw_lock> guard { impl(p_)->info().lc_ };
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
        std::unique_lock<rw_lock> guard { impl(p_)->info().lc_ };
        impl(p_)->acc().release(impl(p_)->id_);
    }
    impl(p_)->rts_.clear();
    impl(p_)->r_.disconnect();
    impl(p_)->h_.release();
}

std::size_t channel::recv_count() const {
    return impl(p_)->r_.recv_count();
}

template <typename... P>
inline bool channel_send(route& rt, P&&... params) {
    for (unsigned k = 0; rt.recv_count() == 0; ++k) {
        if (k >= 1024) return false;
        std::this_thread::yield();
    }
    return rt.send(params...); // no need std::forward
}

bool channel::send(void const * data, std::size_t size) {
    return channel_send(impl(p_)->r_, data, size);
}

bool channel::send(buff_t const & buff) {
    return channel_send(impl(p_)->r_, buff);
}

bool channel::send(std::string const & str) {
    return channel_send(impl(p_)->r_, str);
}

buff_t channel::recv() {
    if (!valid()) return {};
    std::array<queue_t*, id_pool::max_count> ques;
    return ipc::multi_recv([&] {
        std::array<std::size_t, id_pool::max_count> acqs;
        std::size_t counter = 0;
        std::unordered_map<std::size_t, route> cache;
        // get all acquired ids
        {
            std::shared_lock<rw_lock> guard { impl(p_)->info().lc_ };
            impl(p_)->acc().for_each([&](std::size_t id, bool acquired) {
                if (id == impl(p_)->id_) return;
                if (acquired) {
                    acqs[counter++] = id;
                }
            });
        }
        // populate route cache & ques
        for (std::size_t i = 0; i < counter; ++i) {
            auto id = acqs[i];
            auto it = impl(p_)->rts_.find(id);
            // it's a new id
            if (it == impl(p_)->rts_.end()) {
                it = cache.emplace(id, (impl(p_)->n_ + std::to_string(id)).c_str()).first;
                queue_of(it->second.handle())->connect();
            }
            // it's an existing id
            else it = cache.insert(impl(p_)->rts_.extract(it)).position;
            // get queue of this route
            ques[i] = queue_of(it->second.handle());
        }
        // update rts mapping
        impl(p_)->rts_.swap(cache);
        return std::make_tuple(ques.data(), counter);
    });
}

} // namespace ipc
