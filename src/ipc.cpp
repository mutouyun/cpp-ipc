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

inline std::atomic_size_t* acc_of(queue_t* que) {
    return reinterpret_cast<std::atomic_size_t*>(que->elems()) - 1;
}

inline auto& recv_cache() {
    /*
     * the performance of tls::pointer is not good enough
     * so regardless of the mingw-crash-problem for the moment
    */
    thread_local std::unordered_map<decltype(msg_t::id_), buff_t> rc;
    return rc;
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
    auto que = queue_of(h);
    if (que == nullptr) {
        return error_count;
    }
    return que->conn_count();
}

void clear_recv(handle_t h) {
    auto* head = acc_of(queue_of(h));
    if (head == nullptr) {
        return;
    }
    std::memset(head, 0, sizeof(shm_info_t));
}

void clear_recv(char const * name) {
    auto h = ipc::connect(name);
    ipc::clear_recv(h);
    ipc::disconnect(h);
}

bool send(handle_t h, void const * data, std::size_t size) {
    if (data == nullptr) {
        return false;
    }
    if (size == 0) {
        return false;
    }
    auto que = queue_of(h);
    if (que == nullptr) {
        return false;
    }
    // calc a new message id
    auto msg_id = acc_of(que)->fetch_add(1, std::memory_order_relaxed);
    // push message fragment, one fragment size is data_length
    int offset = 0;
    for (int i = 0; i < static_cast<int>(size / data_length); ++i, offset += data_length) {
        msg_t msg {
            static_cast<int>(size) - offset - static_cast<int>(data_length),
            msg_id, { 0 }
        };
        std::memcpy(msg.data_, static_cast<byte_t const *>(data) + offset, data_length);
        que->push(msg);
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
        que->push(msg);
    }
    return true;
}

template <typename F>
buff_t multi_recv(F&& upd) {
    auto& rc = recv_cache();
    while(1) {
        // pop a new message
        auto msg = queue_t::pop(queue_t::multi_wait_for(upd));
        // msg.remain_ may minus & abs(msg.remain_) < data_length
        std::size_t remain = static_cast<std::size_t>(
                             static_cast<int>(data_length) + msg.remain_);
        // find cache with msg.id_
        auto cache_it = rc.find(msg.id_);
        if (cache_it == rc.end()) {
            if (remain <= data_length) {
                return make_buff(msg.data_, remain);
            }
            // cache the first message fragment
            else rc.emplace(msg.id_, make_buff(msg.data_));
        }
        // has cached before this message
        else {
            auto& cache = cache_it->second;
            // this is the last message fragment
            if (msg.remain_ <= 0) {
                cache.insert(cache.end(), msg.data_, msg.data_ + remain);
                // finish this message, erase it from cache
                auto buf = std::move(cache);
                rc.erase(cache_it);
                return buf;
            }
            // there are remain datas after this message
            cache.insert(cache.end(), msg.data_, msg.data_ + data_length);
        }
    }
}

buff_t recv(handle_t const * hs, std::size_t size) {
    thread_local std::vector<queue_t*> q_arr(size);
    q_arr.clear(); // make the size to 0
    for (size_t i = 0; i < size; ++i) {
        auto que = queue_of(hs[i]);
        if (que == nullptr) continue;
        que->connect(); // wouldn't connect twice
        q_arr.push_back(que);
    }
    if (q_arr.empty()) {
        return {};
    }
    return multi_recv([&] {
        return std::make_tuple(q_arr.data(), q_arr.size());
    });
}

buff_t recv(handle_t h) {
    return recv(&h, 1);
}

} // namespace ipc

#include "route.hpp"
#include "channel.hpp"
