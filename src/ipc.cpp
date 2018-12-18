#include "ipc.h"

#include <unordered_map>
#include <type_traits>
#include <cstring>
#include <algorithm>
#include <utility>
//#include <shared_mutex>
//#include <mutex>
#include <atomic>

#include "def.h"
#include "circ_queue.h"
//#include "rw_lock.h"
//#include "tls_pointer.h"

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

struct shm_info_t {
    std::atomic_size_t id_acc_; // message id accumulator
    queue_t::array_t   elems_;  // the circ_elem_array in shm
};

constexpr queue_t* queue_of(handle_t h) {
    return static_cast<queue_t*>(h);
}

constexpr std::atomic_size_t* acc_of(queue_t* queue) {
    return reinterpret_cast<std::atomic_size_t*>(queue->elems()) - 1;
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
    auto mem = shm::acquire(name, sizeof(shm_info_t));
    if (mem == nullptr) {
        return nullptr;
    }
    return new queue_t { &(static_cast<shm_info_t*>(mem)->elems_) };
}

void disconnect(handle_t h) {
    queue_t* que = queue_of(h);
    if (que == nullptr) {
        return;
    }
    que->disconnect(); // needn't to detach, cause it will be deleted soon.
    shm::release(acc_of(que), sizeof(shm_info_t));
    delete que;
}

std::size_t recv_count(handle_t h) {
    auto queue = queue_of(h);
    if (queue == nullptr) {
        return error_count;
    }
    return queue->conn_count();
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
    return recv(&h, 1);
}

buff_t recv(handle_t const * hs, std::size_t size) {
    thread_local std::vector<queue_t*> q_arr(size);
    q_arr.clear(); // make the size to 0
    for (size_t i = 0; i < size; ++i) {
        auto queue = queue_of(hs[i]);
        if (queue == nullptr) continue;
        queue->connect(); // wouldn't connect twice
        q_arr.push_back(queue);
    }
    if (q_arr.empty()) {
        return {};
    }
    /*
     * the performance of tls::pointer is not good enough
     * so regardless of the mingw-crash-problem for the moment
    */
    thread_local std::unordered_map<decltype(msg_t::id_), buff_t> rcs;
    while(1) {
        // pop a new message
        auto msg = queue_t::multi_pop(q_arr);
        // msg.remain_ may minus & abs(msg.remain_) < data_length
        std::size_t remain = static_cast<std::size_t>(
                             static_cast<int>(data_length) + msg.remain_);
        // find cache with msg.id_
        auto cache_it = rcs.find(msg.id_);
        if (cache_it == rcs.end()) {
            if (remain <= data_length) {
                return make_buff(msg.data_, remain);
            }
            // cache the first message fragment
            else rcs.emplace(msg.id_, make_buff(msg.data_));
        }
        // has cached before this message
        else {
            auto& cache = cache_it->second;
            // this is the last message fragment
            if (msg.remain_ <= 0) {
                cache.insert(cache.end(), msg.data_, msg.data_ + remain);
                // finish this message, erase it from cache
                auto buf = std::move(cache);
                rcs.erase(cache_it);
                return buf;
            }
            // there are remain datas after this message
            cache.insert(cache.end(), msg.data_, msg.data_ + data_length);
        }
    }
}

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
