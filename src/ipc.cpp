#include "ipc.h"

#include <unordered_map>
#include <memory>
#include <type_traits>
#include <cstring>
#include <string>
#include <algorithm>
#include <utility>
#include <shared_mutex>
#include <mutex>

#include "def.h"
#include "circ_queue.h"
#include "rw_lock.h"
#include "tls_pointer.h"

namespace {

using namespace ipc;

using data_t = byte_t[data_length];

struct msg_t {
    int      remain_;
    unsigned id_;
    data_t   data_;
};

using queue_t = circ::queue<msg_t>;
using guard_t = std::unique_ptr<std::remove_pointer_t<handle_t>, void(*)(handle_t)>;

/*
 * thread_local stl object's destructor causing crash
 * See: https://sourceforge.net/p/mingw-w64/bugs/527/
 *      https://sourceforge.net/p/mingw-w64/bugs/727/
*/
/*thread_local*/
tls::pointer<std::unordered_map<decltype(msg_t::id_), std::vector<byte_t>>> recv_caches__;

std::unordered_map<handle_t, queue_t> h2q__;
rw_lock h2q_lc__;

queue_t* queue_of(handle_t h) {
    if (h == nullptr) {
        return nullptr;
    }
    std::shared_lock<rw_lock> guard { h2q_lc__ };
    auto it = h2q__.find(h);
    if (it == h2q__.end()) {
        return nullptr;
    }
    if (it->second.elems() == nullptr) {
        return nullptr;
    }
    return &(it->second);
}

} // internal-linkage

namespace ipc {

handle_t connect(char const * name) {
    auto h = shm::acquire(name, sizeof(queue_t));
    if (h == nullptr) {
        return nullptr;
    }
    guard_t h_guard {
        h, [](handle_t h) { shm::release(h, sizeof(queue_t)); }
    };
    auto mem = shm::open(h);
    if (mem == nullptr) {
        return nullptr;
    }
    {
        std::unique_lock<rw_lock> guard { h2q_lc__ };
        h2q__[h].attach(static_cast<queue_t::array_t*>(mem));
    }
    h_guard.release();
    return h;
}

void disconnect(handle_t h) {
    void* mem = nullptr;
    {
        std::unique_lock<rw_lock> guard { h2q_lc__ };
        auto it = h2q__.find(h);
        if (it == h2q__.end()) return;
        it->second.disconnect();
        mem = it->second.elems(); // needn't to detach
        h2q__.erase(it);
    }
    shm::close(mem);
    shm::release(h, sizeof(queue_t));
}

bool send(handle_t h, void* data, int size) {
    if (data == nullptr) {
        return false;
    }
    if (size <= 0) {
        return false;
    }
    auto queue = queue_of(h);
    if (queue == nullptr) {
        return false;
    }
    static unsigned msg_id = 0;
    ++msg_id; // calc a new message id, atomic is unnecessary
    int offset = 0;
    for (int i = 0; i < (size / static_cast<int>(data_length)); ++i, offset += data_length) {
        msg_t msg {
            size - offset - static_cast<int>(data_length),
            msg_id, { 0 }
        };
        std::memcpy(msg.data_, static_cast<byte_t*>(data) + offset, data_length);
        queue->push(msg);
    }
    int remain = size - offset;
    if (remain > 0) {
        msg_t msg {
            remain - static_cast<int>(data_length),
            msg_id, { 0 }
        };
        std::memcpy(msg.data_, static_cast<byte_t*>(data) + offset,
                    static_cast<std::size_t>(remain));
        queue->push(msg);
    }
    return true;
}

std::vector<byte_t> recv(handle_t h) {
    auto queue = queue_of(h);
    if (queue == nullptr) {
        return {};
    }
    if (!queue->connected()) {
        queue->connect();
    }
    auto rcs = recv_caches__.create();
    while(1) {
        // pop a new message
        auto msg = queue->pop();
        // remain_ may minus & abs(remain_) < data_length
        std::size_t remain = static_cast<std::size_t>(
                             static_cast<int>(data_length) + msg.remain_);
        auto cache_it = rcs->find(msg.id_);
        if (cache_it == rcs->end()) {
            std::vector<byte_t> buf(remain);
            std::memcpy(buf.data(), msg.data_, remain);
            return buf;
        }
        // has cache before this message
        auto& cache = cache_it->second;
        auto last_size = cache.size();
        // this is the last message fragment
        if (msg.remain_ <= 0) {
            cache.resize(last_size + remain);
            std::memcpy(cache.data() + last_size, msg.data_, remain);
            // finish this message, erase it from cache
            auto buf = std::move(cache);
            rcs->erase(cache_it);
            return buf;
        }
        // there are remain datas after this message
        cache.resize(last_size + data_length);
        std::memcpy(cache.data() + last_size, msg.data_, data_length);
    }
}

class channel::channel_ : public pimpl<channel_> {
public:
    handle_t    h_ = nullptr;
    std::string n_;
};

channel::channel(void)
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

channel::~channel(void) {
    p_->clear();
}

void channel::swap(channel& rhs) {
    std::swap(p_, rhs.p_);
}

channel& channel::operator=(channel rhs) {
    swap(rhs);
    return *this;
}

bool channel::valid(void) const {
    return (impl(p_)->h_ != nullptr);
}

char const * channel::name(void) const {
    return impl(p_)->n_.c_str();
}

channel channel::clone(void) const {
    return { name() };
}

bool channel::connect(char const * name) {
    if (name == nullptr || name[0] == '\0') return false;
    this->disconnect();
    impl(p_)->h_ = ipc::connect((impl(p_)->n_ = name).c_str());
    return valid();
}

void channel::disconnect(void) {
    if (!valid()) return;
    ipc::disconnect(impl(p_)->h_);
}

} // namespace ipc
