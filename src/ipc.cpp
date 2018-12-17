#include "ipc.h"

#include <unordered_map>
#include <memory>
#include <type_traits>
#include <cstring>
#include <algorithm>
#include <utility>
#include <shared_mutex>
#include <mutex>
#include <atomic>

#include "def.h"
#include "circ_queue.h"
#include "rw_lock.h"
#include "tls_pointer.h"

namespace {

using namespace ipc;
using data_t = byte_t[data_length];

#pragma pack(1)
struct msg_t {
    int         remain_;
    std::size_t id_;
    data_t      data_;
};
#pragma pack()

using queue_t = circ::queue<msg_t>;
using guard_t = std::unique_ptr<std::remove_pointer_t<handle_t>, void(*)(handle_t)>;

struct shm_info_t {
    std::atomic_size_t id_acc_; // message id accumulator
    queue_t::array_t   elems_;  // the circ_elem_array in shm
};

/*
 * thread_local stl object's destructor causing crash
 * See: https://sourceforge.net/p/mingw-w64/bugs/527/
 *      https://sourceforge.net/p/mingw-w64/bugs/727/
*/
/*thread_local*/
tls::pointer<std::unordered_map<decltype(msg_t::id_), buff_t>> recv_caches__;

std::unordered_map<handle_t, queue_t> h2q__;
rw_lock h2q_lc__;

inline queue_t* queue_of(handle_t h) {
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

inline std::atomic_size_t* acc_of(queue_t* queue) {
    auto elems = queue->elems();
    return reinterpret_cast<std::atomic_size_t*>(elems) - 1;
}

} // internal-linkage

namespace ipc {

buff_t make_buff(void const * data, std::size_t size) {
    return {
        static_cast<buff_t::value_type const *>(data),
        static_cast<buff_t::value_type const *>(data) + size
    };
}

handle_t connect(char const * name) {
    auto h = shm::acquire(name, sizeof(shm_info_t));
    if (h == nullptr) {
        return nullptr;
    }
    guard_t h_guard {
        h, [](handle_t h) { shm::release(h, sizeof(shm_info_t)); }
    };
    auto mem = shm::open(h);
    if (mem == nullptr) {
        return nullptr;
    }
    {
        std::unique_lock<rw_lock> guard { h2q_lc__ };
        h2q__[h].attach(&(static_cast<shm_info_t*>(mem)->elems_));
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

bool send(handle_t h, void const * data, std::size_t size) {
    if (data == nullptr) {
        return false;
    }
    if (size == 0) {
        return false;
    }
    auto queue = queue_of(h);
    if (queue == nullptr) {
        return false;
    }
    // calc a new message id
    auto msg_id = acc_of(queue)->fetch_add(1, std::memory_order_relaxed);
    // push message fragment, one fragment size is data_length
    int offset = 0;
    for (int i = 0; i < static_cast<int>(size / data_length); ++i, offset += data_length) {
        msg_t msg {
            static_cast<int>(size) - offset - static_cast<int>(data_length),
            msg_id, { 0 }
        };
        std::memcpy(msg.data_, static_cast<byte_t const *>(data) + offset, data_length);
        queue->push(msg);
    }
    // if remain > 0, this is the last message fragment
    int remain = static_cast<int>(size) - offset;
    if (remain > 0) {
        msg_t msg {
            remain - static_cast<int>(data_length),
            msg_id, { 0 }
        };
        std::memcpy(msg.data_, static_cast<byte_t const *>(data) + offset,
                               static_cast<std::size_t>(remain));
        queue->push(msg);
    }
    return true;
}

buff_t recv(handle_t h) {
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
        // msg.remain_ may minus & abs(msg.remain_) < data_length
        std::size_t remain = static_cast<std::size_t>(
                             static_cast<int>(data_length) + msg.remain_);
        // find cache with msg.id_
        auto cache_it = rcs->find(msg.id_);
        if (cache_it == rcs->end()) {
            if (remain <= data_length) {
                return make_buff(msg.data_, remain);
            }
            // cache the first message fragment
            else rcs->emplace(msg.id_, make_buff(msg.data_));
        }
        // has cached before this message
        else {
            auto& cache = cache_it->second;
            // this is the last message fragment
            if (msg.remain_ <= 0) {
                cache.insert(cache.end(), msg.data_, msg.data_ + remain);
                // finish this message, erase it from cache
                auto buf = std::move(cache);
                rcs->erase(cache_it);
                return buf;
            }
            // there are remain datas after this message
            cache.insert(cache.end(), msg.data_, msg.data_ + data_length);
        }
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

bool channel::send(void const *data, std::size_t size) {
    return ipc::send(impl(p_)->h_, data, size);
}

bool channel::send(buff_t const & buff) {
    return channel::send(buff.data(), buff.size());
}

bool channel::send(std::string const & str) {
    return channel::send(str.c_str(), str.size() + 1);
}

buff_t channel::recv() {
    return ipc::recv(impl(p_)->h_);
}

} // namespace ipc
