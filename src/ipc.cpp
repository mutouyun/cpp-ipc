#include <unordered_map>
#include <memory>
#include <type_traits>
#include <cstring>
#include <algorithm>
#include <utility>

#include "ipc.h"
#include "circ_queue.h"

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

std::unordered_map<handle_t, queue_t> h2q__;

queue_t* queue_of(handle_t h) {
    if (h == nullptr) {
        return nullptr;
    }
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

handle_t connect(std::string const & name) {
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
    h2q__[h].attach(static_cast<queue_t::array_t*>(mem));
    h_guard.release();
    return h;
}

void disconnect(handle_t h) {
    auto it = h2q__.find(h);
    if (it == h2q__.end()) return;
    it->second.disconnect();
    shm::close(it->second.detach());
    shm::release(h, sizeof(queue_t));
    h2q__.erase(it);
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
    ++msg_id; // calc a new message id
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
    static thread_local std::unordered_map<int, std::vector<byte_t>> all;
    do {
        auto msg = queue->pop();
        // here comes a new message
        auto& cache = all[msg.id_]; // find the cache using message id
        auto last_size = cache.size();
        if (msg.remain_ > 0) {
            cache.resize(last_size + data_length);
            std::memcpy(cache.data() + last_size, msg.data_, data_length);
        }
        else {
            // remain_ is minus & abs(remain_) < data_length
            std::size_t remain = static_cast<std::size_t>(
                                 static_cast<int>(data_length) + msg.remain_);
            cache.resize(last_size + remain);
            std::memcpy(cache.data() + last_size, msg.data_, remain);
            // finish this message, erase it from cache
            auto ret { std::move(cache) };
            all.erase(msg.id_);
            return std::move(ret);
        }
    } while(1);
}

class channel::channel_ {
public:
};

channel::channel(void)
    : p_(new channel_) {
}

channel::channel(std::string const & /*name*/)
    : channel() {

}

channel::channel(channel&& rhs)
    : channel() {
    swap(rhs);
}

channel::~channel(void) {
    delete p_;
}

void channel::swap(channel& rhs) {
    std::swap(p_, rhs.p_);
}

channel& channel::operator=(channel rhs) {
    swap(rhs);
    return *this;
}

} // namespace ipc
