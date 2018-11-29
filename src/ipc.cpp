#include <unordered_map>
#include <memory>
#include <type_traits>
#include <cstring>
#include <algorithm>

#include "ipc.h"
#include "circ_queue.h"

namespace {

using namespace ipc;

using data_t = byte_t[data_length];

struct msg_t {
    int    remain_;
    data_t data_;
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

bool send(handle_t h, byte_t* data, int size) {
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
    queue_t drop_box { queue->elems() };
    int offset = 0;
    for (int i = 0; i < (size / static_cast<int>(data_length)); ++i, offset += data_length) {
        msg_t msg {
            size - offset - static_cast<int>(data_length),
            { 0 }
        };
        std::memcpy(msg.data_, data + offset, data_length);
        drop_box.push(msg);
    }
    int remain = size - offset;
    if (remain > 0) {
        msg_t msg { remain - static_cast<int>(data_length), { 0 } };
        std::memcpy(msg.data_, data + offset, static_cast<std::size_t>(remain));
        drop_box.push(msg);
    }
    return true;
}

std::vector<byte_t> recv(handle_t h) {
    std::vector<byte_t> all;
    auto queue = queue_of(h);
    if (queue == nullptr) {
        return all;
    }
    if (!queue->connected()) {
        queue->connect();
    }
    do {
        auto msg = queue->pop();
        auto last_size = all.size();
        if (msg.remain_ > 0) {
            all.resize(last_size + data_length);
            std::memcpy(all.data() + last_size, msg.data_, data_length);
        }
        else {
            // remain_ is minus & abs(remain_) < data_length
            std::size_t remain = static_cast<std::size_t>(
                                 static_cast<int>(data_length) + msg.remain_);
            all.resize(last_size + remain);
            std::memcpy(all.data() + last_size, msg.data_, remain);
            break;
        }
    } while(1);
    return all;
}

} // namespace ipc
