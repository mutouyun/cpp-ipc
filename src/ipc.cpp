#include "ipc.h"

#include <type_traits>
#include <cstring>
#include <algorithm>
#include <utility>
#include <atomic>
#include <type_traits>
#include <string>
#include <vector>

#include "def.h"
#include "shm.h"
#include "tls_pointer.h"
#include "pool_alloc.h"
#include "queue.h"
#include "policy.h"
#include "rw_lock.h"
#include "log.h"

#include "memory/resource.h"

#include "platform/detail.h"
#include "platform/waiter_wrapper.h"

namespace {

using namespace ipc;
using msg_id_t = std::size_t;

template <std::size_t DataSize, std::size_t AlignSize>
struct msg_t;

template <std::size_t AlignSize>
struct msg_t<0, AlignSize> {
    void*    que_;
    msg_id_t id_;
    int      remain_;
};

template <std::size_t DataSize, std::size_t AlignSize>
struct msg_t {
    msg_t<0, AlignSize> head_ {};
    std::aligned_storage_t<DataSize, AlignSize> data_ {};

    msg_t() = default;
    msg_t(void* q, msg_id_t i, int r, void const * d, std::size_t s) {
        head_.que_    = q;
        head_.id_     = i;
        head_.remain_ = r;
        std::memcpy(&data_, d, s);
    }
};

template <typename T>
buff_t make_cache(T& data, std::size_t size) {
    auto ptr = mem::alloc(size);
    std::memcpy(ptr, &data, (ipc::detail::min)(sizeof(data), size));
    return { ptr, size, mem::free };
}

struct cache_t {
    std::size_t fill_;
    buff_t      buff_;

    cache_t(std::size_t f, buff_t&& b)
        : fill_(f), buff_(std::move(b))
    {}

    void append(void const * data, std::size_t size) {
        if (fill_ >= buff_.size() || data == nullptr || size == 0) return;
        auto new_fill = (ipc::detail::min)(fill_ + size, buff_.size());
        std::memcpy(static_cast<byte_t*>(buff_.data()) + fill_, data, new_fill - fill_);
        fill_ = new_fill;
    }
};

struct conn_info_head {
    using acc_t = std::atomic<msg_id_t>;

    waiter      cc_waiter_, wt_waiter_, rd_waiter_;
    shm::handle acc_h_;

    conn_info_head(char const * name)
        : cc_waiter_((std::string{ "__CC_CONN__" } + name).c_str())
        , wt_waiter_((std::string{ "__WT_CONN__" } + name).c_str())
        , rd_waiter_((std::string{ "__RD_CONN__" } + name).c_str())
        , acc_h_    ((std::string{ "__AC_CONN__" } + name).c_str(), sizeof(acc_t)) {
    }

    auto acc() {
        return static_cast<acc_t*>(acc_h_.get());
    }
};

template <typename W, typename F>
bool wait_for(W& waiter, F&& pred, std::size_t tm) {
    if (tm == 0) return !pred();
    for (unsigned k = 0; pred();) {
        bool loop = true, ret = true;
        ipc::sleep(k, [&k, &loop, &ret, &waiter, &pred, tm] {
            ret = waiter.wait_if([&loop, &pred] {
                return loop = pred();
            }, tm);
            k = 0;
            return true;
        });
        if (!ret ) return false; // timeout or fail
        if (!loop) break;
    }
    return true;
}

template <typename Policy, 
          std::size_t DataSize, 
          std::size_t AlignSize = (ipc::detail::min)(DataSize, alignof(std::max_align_t))>
struct queue_generator {

    using queue_t = ipc::queue<msg_t<DataSize, AlignSize>, Policy>;
    
    struct conn_info_t : conn_info_head {
        queue_t que_;

        conn_info_t(char const * name)
            : conn_info_head(name)
            , que_(("__QU_CONN__" + 
                    std::to_string(DataSize ) + "__" + 
                    std::to_string(AlignSize) + "__" + name).c_str()) {
        }
    };
};

template <typename Policy>
struct detail_impl {

using queue_t     = typename queue_generator<Policy, data_length>::queue_t;
using conn_info_t = typename queue_generator<Policy, data_length>::conn_info_t;

constexpr static conn_info_t* info_of(ipc::handle_t h) {
    return static_cast<conn_info_t*>(h);
}

constexpr static queue_t* queue_of(ipc::handle_t h) {
    return (info_of(h) == nullptr) ? nullptr : &(info_of(h)->que_);
}

static auto& recv_cache() {
    /*
        <Remarks> thread_local may have some bugs.
        See: https://sourceforge.net/p/mingw-w64/bugs/727/
             https://sourceforge.net/p/mingw-w64/bugs/527/
             https://github.com/Alexpux/MINGW-packages/issues/2519
             https://github.com/ChaiScript/ChaiScript/issues/402
             https://developercommunity.visualstudio.com/content/problem/124121/thread-local-variables-fail-to-be-initialized-when.html
             https://software.intel.com/en-us/forums/intel-c-compiler/topic/684827
    */
    static tls::pointer<mem::unordered_map<msg_id_t, cache_t>> rc;
    return *rc.create();
}

/* API implementations */

static ipc::handle_t connect(char const * name, bool start) {
    auto h = mem::alloc<conn_info_t>(name);
    auto que = queue_of(h);
    if (que == nullptr) {
        return nullptr;
    }
    if (start) {
        if (que->connect()) { // wouldn't connect twice
            info_of(h)->cc_waiter_.broadcast();
        }
    }
    return h;
}

static void disconnect(ipc::handle_t h) {
    auto que = queue_of(h);
    if (que == nullptr) {
        return;
    }
    if (que->disconnect()) {
        info_of(h)->cc_waiter_.broadcast();
    }
    mem::free(info_of(h));
}

static std::size_t recv_count(ipc::handle_t h) {
    auto que = queue_of(h);
    if (que == nullptr) {
        return invalid_value;
    }
    return que->conn_count();
}

static bool wait_for_recv(ipc::handle_t h, std::size_t r_count, std::size_t tm) {
    auto que = queue_of(h);
    if (que == nullptr) {
        return false;
    }
    return wait_for(info_of(h)->cc_waiter_, [que, r_count] {
        return que->conn_count() < r_count;
    }, tm);
}

template <typename F>
static bool send(F&& gen_push, ipc::handle_t h, void const * data, std::size_t size) {
    if (data == nullptr || size == 0) {
        ipc::error("fail: send(%p, %zd)\n", data, size);
        return false;
    }
    auto que = queue_of(h);
    if (que == nullptr) {
        ipc::error("fail: send, queue_of(h) == nullptr\n");
        return false;
    }
    // calc a new message id
    auto acc = info_of(h)->acc();
    if (acc == nullptr) {
        ipc::error("fail: send, info_of(h)->acc() == nullptr\n");
        return false;
    }
    auto msg_id   = acc->fetch_add(1, std::memory_order_relaxed);
    auto try_push = std::forward<F>(gen_push)(info_of(h), que, msg_id);
    // push message fragment
    int offset = 0;
    for (int i = 0; i < static_cast<int>(size / data_length); ++i, offset += data_length) {
        if (!try_push(static_cast<int>(size) - offset - static_cast<int>(data_length),
                      static_cast<byte_t const *>(data) + offset, data_length)) {
            return false;
        }
    }
    // if remain > 0, this is the last message fragment
    int remain = static_cast<int>(size) - offset;
    if (remain > 0) {
        if (!try_push(remain - static_cast<int>(data_length),
                      static_cast<byte_t const *>(data) + offset, static_cast<std::size_t>(remain))) {
            return false;
        }
    }
    return true;
}

static bool send(ipc::handle_t h, void const * data, std::size_t size) {
    return send([](auto info, auto que, auto msg_id) {
        return [info, que, msg_id](int remain, void const * data, std::size_t size) {
            if (!wait_for(info->wt_waiter_, [&] {
                    return !que->push(que, msg_id, remain, data, size);
                }, que->dis_flag() ? 0 : static_cast<std::size_t>(default_timeut))) {
                ipc::log("force_push: msg_id = %zd, remain = %d, size = %zd\n", msg_id, remain, size);
                if (!que->force_push(que, msg_id, remain, data, size)) {
                    return false;
                }
            }
            info->rd_waiter_.broadcast();
            return true;
        };
    }, h, data, size);
}

static bool try_send(ipc::handle_t h, void const * data, std::size_t size) {
    return send([](auto info, auto que, auto msg_id) {
        return [info, que, msg_id](int remain, void const * data, std::size_t size) {
            if (!wait_for(info->wt_waiter_, [&] {
                    return !que->push(que, msg_id, remain, data, size);
                }, 0)) {
                return false;
            }
            info->rd_waiter_.broadcast();
            return true;
        };
    }, h, data, size);
}

static buff_t recv(ipc::handle_t h, std::size_t tm) {
    auto que = queue_of(h);
    if (que == nullptr) {
        ipc::error("fail: recv, queue_of(h) == nullptr\n");
        return {};
    }
    if (que->connect()) { // wouldn't connect twice
        info_of(h)->cc_waiter_.broadcast();
    }
    auto& rc = recv_cache();
    while (1) {
        // pop a new message
        typename queue_t::value_t msg;
        if (!wait_for(info_of(h)->rd_waiter_,
                      [que, &msg] { return !que->pop(msg); }, tm)) {
            return {};
        }
        info_of(h)->wt_waiter_.broadcast();
        if (msg.head_.que_ == nullptr) {
            ipc::error("fail: recv, msg.head_.que_ == nullptr\n");
            return {};
        }
        if (msg.head_.que_ == que) continue; // pop next
        // msg.head_.remain_ may minus & abs(msg.head_.remain_) < data_length
        auto remain = static_cast<std::size_t>(static_cast<int>(data_length) + msg.head_.remain_);
        // find cache with msg.head_.id_
        auto cac_it = rc.find(msg.head_.id_);
        if (cac_it == rc.end()) {
            if (remain <= data_length) {
                return make_cache(msg.data_, remain);
            }
            else {
                // gc
                if (rc.size() > 1024) {
                    std::vector<msg_id_t> need_del;
                    for (auto const & pair : rc) {
                        auto cmp = std::minmax(msg.head_.id_, pair.first);
                        if (cmp.second - cmp.first > 8192) {
                            need_del.push_back(pair.first);
                        }
                    }
                    for (auto id : need_del) rc.erase(id);
                }
                // cache the first message fragment
                rc.emplace(msg.head_.id_, cache_t { data_length, make_cache(msg.data_, remain) });
            }
        }
        // has cached before this message
        else {
            auto& cac = cac_it->second;
            // this is the last message fragment
            if (msg.head_.remain_ <= 0) {
                cac.append(&(msg.data_), remain);
                // finish this message, erase it from cache
                auto buff = std::move(cac.buff_);
                rc.erase(cac_it);
                return buff;
            }
            // there are remain datas after this message
            cac.append(&(msg.data_), data_length);
        }
    }
}

static buff_t try_recv(ipc::handle_t h) {
    return recv(h, 0);
}

}; // detail_impl<Policy>

template <typename Flag>
using policy_t = policy::choose<circ::elem_array, Flag>;

} // internal-linkage

namespace ipc {

template <typename Flag>
ipc::handle_t chan_impl<Flag>::connect(char const * name, unsigned mode) {
    return detail_impl<policy_t<Flag>>::connect(name, mode & receiver);
}

template <typename Flag>
void chan_impl<Flag>::disconnect(ipc::handle_t h) {
    detail_impl<policy_t<Flag>>::disconnect(h);
}

template <typename Flag>
std::size_t chan_impl<Flag>::recv_count(ipc::handle_t h) {
    return detail_impl<policy_t<Flag>>::recv_count(h);
}

template <typename Flag>
bool chan_impl<Flag>::wait_for_recv(ipc::handle_t h, std::size_t r_count, std::size_t tm) {
    return detail_impl<policy_t<Flag>>::wait_for_recv(h, r_count, tm);
}

template <typename Flag>
bool chan_impl<Flag>::send(ipc::handle_t h, void const * data, std::size_t size) {
    return detail_impl<policy_t<Flag>>::send(h, data, size);
}

template <typename Flag>
buff_t chan_impl<Flag>::recv(ipc::handle_t h, std::size_t tm) {
    return detail_impl<policy_t<Flag>>::recv(h, tm);
}

template <typename Flag>
bool chan_impl<Flag>::try_send(ipc::handle_t h, void const * data, std::size_t size) {
    return detail_impl<policy_t<Flag>>::try_send(h, data, size);
}

template <typename Flag>
buff_t chan_impl<Flag>::try_recv(ipc::handle_t h) {
    return detail_impl<policy_t<Flag>>::try_recv(h);
}

template struct chan_impl<ipc::wr<relat::single, relat::single, trans::unicast  >>;
template struct chan_impl<ipc::wr<relat::single, relat::multi , trans::unicast  >>;
template struct chan_impl<ipc::wr<relat::multi , relat::multi , trans::unicast  >>;
template struct chan_impl<ipc::wr<relat::single, relat::multi , trans::broadcast>>;
template struct chan_impl<ipc::wr<relat::multi , relat::multi , trans::broadcast>>;

} // namespace ipc
