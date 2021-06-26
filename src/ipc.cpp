
#include <type_traits>
#include <cstring>
#include <algorithm>
#include <utility>          // std::pair, std::move, std::forward
#include <atomic>
#include <type_traits>      // aligned_storage_t
#include <string>
#include <vector>
#include <array>
#include <cassert>

#include "libipc/ipc.h"
#include "libipc/def.h"
#include "libipc/shm.h"
#include "libipc/tls_pointer.h"
#include "libipc/pool_alloc.h"
#include "libipc/queue.h"
#include "libipc/policy.h"
#include "libipc/rw_lock.h"

#include "libipc/utility/log.h"
#include "libipc/utility/id_pool.h"
#include "libipc/utility/utility.h"

#include "libipc/memory/resource.h"

#include "libipc/platform/detail.h"
#include "libipc/platform/waiter_wrapper.h"

#include "libipc/circ/elem_array.h"

namespace {

using msg_id_t = std::uint32_t;
using acc_t    = std::atomic<msg_id_t>;

template <std::size_t DataSize, std::size_t AlignSize>
struct msg_t;

template <std::size_t AlignSize>
struct msg_t<0, AlignSize> {
    msg_id_t     conn_;
    msg_id_t     id_;
    std::int32_t remain_;
    bool         storage_;
};

template <std::size_t DataSize, std::size_t AlignSize>
struct msg_t : msg_t<0, AlignSize> {
    std::aligned_storage_t<DataSize, AlignSize> data_ {};

    msg_t() = default;
    msg_t(msg_id_t c, msg_id_t i, std::int32_t r, void const * d, std::size_t s)
        : msg_t<0, AlignSize> { c, i, r, (d == nullptr) || (s == 0) } {
        if (this->storage_) {
            if (d != nullptr) {
                // copy storage-id
                *reinterpret_cast<std::size_t*>(&data_) = *static_cast<std::size_t const *>(d);
            }
        }
        else std::memcpy(&data_, d, s);
    }
};

template <typename T>
ipc::buff_t make_cache(T& data, std::size_t size) {
    auto ptr = ipc::mem::alloc(size);
    std::memcpy(ptr, &data, (ipc::detail::min)(sizeof(data), size));
    return { ptr, size, ipc::mem::free };
}

struct cache_t {
    std::size_t fill_;
    ipc::buff_t buff_;

    cache_t(std::size_t f, ipc::buff_t && b)
        : fill_(f), buff_(std::move(b))
    {}

    void append(void const * data, std::size_t size) {
        if (fill_ >= buff_.size() || data == nullptr || size == 0) return;
        auto new_fill = (ipc::detail::min)(fill_ + size, buff_.size());
        std::memcpy(static_cast<ipc::byte_t*>(buff_.data()) + fill_, data, new_fill - fill_);
        fill_ = new_fill;
    }
};

auto cc_acc() {
    static ipc::shm::handle acc_h("__CA_CONN__", sizeof(acc_t));
    return static_cast<acc_t*>(acc_h.get());
}

struct chunk_info_t {
    ipc::id_pool<> pool_;
    ipc::spin_lock lock_;

    IPC_CONSTEXPR_ static std::size_t chunks_elem_size(std::size_t chunk_size) noexcept {
        return ipc::make_align(alignof(std::max_align_t), chunk_size);
    }

    IPC_CONSTEXPR_ static std::size_t chunks_mem_size(std::size_t chunk_size) noexcept {
        return ipc::id_pool<>::max_count * chunks_elem_size(chunk_size);
    }

    ipc::byte_t *at(std::size_t chunk_size, std::size_t id) noexcept {
        if (id == ipc::invalid_value) return nullptr;
        return reinterpret_cast<ipc::byte_t *>(this + 1) + (chunks_elem_size(chunk_size) * id);
    }
};

auto& chunk_storages() {
    class chunk_t {
        ipc::shm::handle handle_;

    public:
        chunk_info_t *get_info(std::size_t chunk_size) {
            if (!handle_.valid() &&
                !handle_.acquire( ("__CHUNK_INFO__" + ipc::to_string(chunk_size)).c_str(), 
                                  sizeof(chunk_info_t) + chunk_info_t::chunks_mem_size(chunk_size) )) {
                ipc::error("[chunk_storages] chunk_shm.id_info_.acquire failed: chunk_size = %zd\n", chunk_size);
                return nullptr;
            }
            auto info = static_cast<chunk_info_t*>(handle_.get());
            if (info == nullptr) {
                ipc::error("[chunk_storages] chunk_shm.id_info_.get failed: chunk_size = %zd\n", chunk_size);
                return nullptr;
            }
            return info;
        }
    };
    static ipc::unordered_map<std::size_t, chunk_t> chunk_s;
    return chunk_s;
}

auto& chunk_lock() {
    static ipc::spin_lock chunk_l;
    return chunk_l;
}

constexpr std::size_t calc_chunk_size(std::size_t size) noexcept {
    return ( ((size - 1) / ipc::large_msg_align) + 1 ) * ipc::large_msg_align;
}

auto& chunk_storage(std::size_t chunk_size) {
    IPC_UNUSED_ auto guard = ipc::detail::unique_lock(chunk_lock());
    return chunk_storages()[chunk_size];
}

std::pair<std::size_t, void*> apply_storage(std::size_t size) {
    std::size_t chunk_size = calc_chunk_size(size);
    auto &      chunk_shm  = chunk_storage(chunk_size);

    auto info = chunk_shm.get_info(chunk_size);
    if (info == nullptr) return {};

    info->lock_.lock();
    info->pool_.prepare();
    // got an unique id
    auto id = info->pool_.acquire();
    info->lock_.unlock();

    return { id, info->at(chunk_size, id) };
}

void *find_storage(std::size_t id, std::size_t size) {
    if (id == ipc::invalid_value) {
        ipc::error("[find_storage] id is invalid: id = %ld, size = %zd\n", (long)id, size);
        return nullptr;
    }
    std::size_t chunk_size = calc_chunk_size(size);
    auto &      chunk_shm  = chunk_storage(chunk_size);
    auto info = chunk_shm.get_info(chunk_size);
    if (info == nullptr) return nullptr;
    return info->at(chunk_size, id);
}

void clear_storage(std::size_t id, std::size_t size) {
    if (id == ipc::invalid_value) {
        ipc::error("[clear_storage] id is invalid: id = %ld, size = %zd\n", (long)id, size);
        return;
    }

    std::size_t chunk_size = calc_chunk_size(size);
    auto &      chunk_shm  = chunk_storage(chunk_size);
    auto info = chunk_shm.get_info(chunk_size);
    if (info == nullptr) return;

    info->lock_.lock();
    info->pool_.release(id);
    info->lock_.unlock();
}

template <typename MsgT>
bool recycle_message(void* p) {
    auto msg = static_cast<MsgT*>(p);
    if (msg->storage_) {
        clear_storage(
            *reinterpret_cast<std::size_t*>(&msg->data_),
            static_cast<std::int32_t>(ipc::data_length) + msg->remain_);
    }
    return true;
}

struct conn_info_head {

    ipc::string name_;
    msg_id_t    cc_id_; // connection-info id
    ipc::waiter cc_waiter_, wt_waiter_, rd_waiter_;
    ipc::shm::handle acc_h_;

    /*
     * <Remarks> thread_local may have some bugs.
     *
     * <Reference>
     * - https://sourceforge.net/p/mingw-w64/bugs/727/
     * - https://sourceforge.net/p/mingw-w64/bugs/527/
     * - https://github.com/Alexpux/MINGW-packages/issues/2519
     * - https://github.com/ChaiScript/ChaiScript/issues/402
     * - https://developercommunity.visualstudio.com/content/problem/124121/thread-local-variables-fail-to-be-initialized-when.html
     * - https://software.intel.com/en-us/forums/intel-c-compiler/topic/684827
    */
    ipc::tls::pointer<ipc::unordered_map<msg_id_t, cache_t>> recv_cache_;

    conn_info_head(char const * name)
        : name_     {name}
        , cc_id_    {(cc_acc() == nullptr) ? 0 : cc_acc()->fetch_add(1, std::memory_order_relaxed)}
        , cc_waiter_{("__CC_CONN__" + name_).c_str()}
        , wt_waiter_{("__WT_CONN__" + name_).c_str()}
        , rd_waiter_{("__RD_CONN__" + name_).c_str()}
        , acc_h_    {("__AC_CONN__" + name_).c_str(), sizeof(acc_t)} {
    }

    void quit_waiting() {
        cc_waiter_.quit_waiting();
        wt_waiter_.quit_waiting();
        rd_waiter_.quit_waiting();
    }

    auto acc() {
        return static_cast<acc_t*>(acc_h_.get());
    }

    auto& recv_cache() {
        return *recv_cache_.create_once();
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
        });
        if (!ret ) return false; // timeout or fail
        if (!loop) break;
    }
    return true;
}

template <typename Policy,
          std::size_t DataSize  = ipc::data_length,
          std::size_t AlignSize = (ipc::detail::min)(DataSize, alignof(std::max_align_t))>
struct queue_generator {

    using queue_t = ipc::queue<msg_t<DataSize, AlignSize>, Policy>;

    struct conn_info_t : conn_info_head {
        queue_t que_;

        conn_info_t(char const * name)
            : conn_info_head{name}
            , que_{("__QU_CONN__" +
                    ipc::to_string(DataSize) + "__" +
                    ipc::to_string(AlignSize) + "__" + name).c_str()} {
        }

        void disconnect_receiver() {
            bool dis = que_.disconnect();
            this->quit_waiting();
            if (dis) {
                this->recv_cache().clear();
            }
        }
    };
};

template <typename Policy>
struct detail_impl {

using queue_t     = typename queue_generator<Policy>::queue_t;
using conn_info_t = typename queue_generator<Policy>::conn_info_t;

constexpr static conn_info_t* info_of(ipc::handle_t h) noexcept {
    return static_cast<conn_info_t*>(h);
}

constexpr static queue_t* queue_of(ipc::handle_t h) noexcept {
    return (info_of(h) == nullptr) ? nullptr : &(info_of(h)->que_);
}

/* API implementations */

static void disconnect(ipc::handle_t h) {
    auto que = queue_of(h);
    if (que == nullptr) {
        return;
    }
    que->shut_sending();
    assert(info_of(h) != nullptr);
    info_of(h)->disconnect_receiver();
}

static bool reconnect(ipc::handle_t * ph, bool start_to_recv) {
    assert(ph != nullptr);
    assert(*ph != nullptr);
    auto que = queue_of(*ph);
    if (que == nullptr) {
        return false;
    }
    if (start_to_recv) {
        que->shut_sending();
        if (que->connect()) { // wouldn't connect twice
            info_of(*ph)->cc_waiter_.broadcast();
            return true;
        }
        return false;
    }
    // start_to_recv == false
    if (que->connected()) {
        info_of(*ph)->disconnect_receiver();
    }
    return que->ready_sending();
}

static bool connect(ipc::handle_t * ph, char const * name, bool start_to_recv) {
    assert(ph != nullptr);
    if (*ph == nullptr) {
        *ph = ipc::mem::alloc<conn_info_t>(name);
    }
    return reconnect(ph, start_to_recv);
}

static void destroy(ipc::handle_t h) {
    disconnect(h);
    ipc::mem::free(info_of(h));
}

static std::size_t recv_count(ipc::handle_t h) noexcept {
    auto que = queue_of(h);
    if (que == nullptr) {
        return ipc::invalid_value;
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
    if (que->elems() == nullptr) {
        ipc::error("fail: send, queue_of(h)->elems() == nullptr\n");
        return false;
    }
    if (!que->ready_sending()) {
        ipc::error("fail: send, que->ready_sending() == false\n");
        return false;
    }
    if (que->elems()->connections(std::memory_order_relaxed) == 0) {
        ipc::error("fail: send, there is no receiver on this connection.\n");
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
    if (size > ipc::large_msg_limit) {
        auto   dat = apply_storage(size);
        void * buf = dat.second;
        if (buf != nullptr) {
            std::memcpy(buf, data, size);
            return try_push(static_cast<std::int32_t>(size) - 
                            static_cast<std::int32_t>(ipc::data_length), &(dat.first), 0);
        }
        // try using message fragment
        // ipc::log("fail: shm::handle for big message. msg_id: %zd, size: %zd\n", msg_id, size);
    }
    // push message fragment
    std::int32_t offset = 0;
    for (int i = 0; i < static_cast<int>(size / ipc::data_length); ++i, offset += ipc::data_length) {
        if (!try_push(static_cast<std::int32_t>(size) - offset - static_cast<std::int32_t>(ipc::data_length),
                      static_cast<ipc::byte_t const *>(data) + offset, ipc::data_length)) {
            return false;
        }
    }
    // if remain > 0, this is the last message fragment
    std::int32_t remain = static_cast<std::int32_t>(size) - offset;
    if (remain > 0) {
        if (!try_push(remain - static_cast<std::int32_t>(ipc::data_length),
                      static_cast<ipc::byte_t const *>(data) + offset, 
                      static_cast<std::size_t>(remain))) {
            return false;
        }
    }
    return true;
}

static bool send(ipc::handle_t h, void const * data, std::size_t size, std::size_t tm) {
    return send([tm](auto info, auto que, auto msg_id) {
        return [tm, info, que, msg_id](std::int32_t remain, void const * data, std::size_t size) {
            if (!wait_for(info->wt_waiter_, [&] {
                    return !que->push(
                        recycle_message<typename queue_t::value_t>, 
                        info->cc_id_, msg_id, remain, data, size);
                }, tm)) {
                ipc::log("force_push: msg_id = %zd, remain = %d, size = %zd\n", msg_id, remain, size);
                if (!que->force_push(
                        recycle_message<typename queue_t::value_t>, 
                        info->cc_id_, msg_id, remain, data, size)) {
                    return false;
                }
            }
            info->rd_waiter_.broadcast();
            return true;
        };
    }, h, data, size);
}

static bool try_send(ipc::handle_t h, void const * data, std::size_t size, std::size_t tm) {
    return send([tm](auto info, auto que, auto msg_id) {
        return [tm, info, que, msg_id](std::int32_t remain, void const * data, std::size_t size) {
            if (!wait_for(info->wt_waiter_, [&] {
                    return !que->push(
                        recycle_message<typename queue_t::value_t>, 
                        info->cc_id_, msg_id, remain, data, size);
                }, tm)) {
                return false;
            }
            info->rd_waiter_.broadcast();
            return true;
        };
    }, h, data, size);
}

static ipc::buff_t recv(ipc::handle_t h, std::size_t tm) {
    auto que = queue_of(h);
    if (que == nullptr) {
        ipc::error("fail: recv, queue_of(h) == nullptr\n");
        return {};
    }
    if (!que->connected()) {
        // hasn't connected yet, just return.
        return {};
    }
    auto& rc = info_of(h)->recv_cache();
    while (1) {
        // pop a new message
        typename queue_t::value_t msg;
        if (!wait_for(info_of(h)->rd_waiter_, [que, &msg] { return !que->pop(msg); }, tm)) {
            // pop failed, just return.
            return {};
        }
        info_of(h)->wt_waiter_.broadcast();
        if ((info_of(h)->acc() != nullptr) && (msg.conn_ == info_of(h)->cc_id_)) {
            continue; // ignore message to self
        }
        // msg.remain_ may minus & abs(msg.remain_) < data_length
        std::size_t remain = static_cast<std::int32_t>(ipc::data_length) + msg.remain_;
        // find cache with msg.id_
        auto cac_it = rc.find(msg.id_);
        if (cac_it == rc.end()) {
            if (remain <= ipc::data_length) {
                return make_cache(msg.data_, remain);
            }
            if (msg.storage_) {
                std::size_t buf_id = *reinterpret_cast<std::size_t*>(&msg.data_);
                void      * buf    = find_storage(buf_id, remain);
                if (buf != nullptr) {
                    return ipc::buff_t{buf, remain};
                }
                else ipc::log("fail: shm::handle for big message. msg_id: %zd, buf_id: %zd, size: %zd\n", msg.id_, buf_id, remain);
            }
            // gc
            if (rc.size() > 1024) {
                std::vector<msg_id_t> need_del;
                for (auto const & pair : rc) {
                    auto cmp = std::minmax(msg.id_, pair.first);
                    if (cmp.second - cmp.first > 8192) {
                        need_del.push_back(pair.first);
                    }
                }
                for (auto id : need_del) rc.erase(id);
            }
            // cache the first message fragment
            rc.emplace(msg.id_, cache_t { ipc::data_length, make_cache(msg.data_, remain) });
        }
        // has cached before this message
        else {
            auto& cac = cac_it->second;
            // this is the last message fragment
            if (msg.remain_ <= 0) {
                cac.append(&(msg.data_), remain);
                // finish this message, erase it from cache
                auto buff = std::move(cac.buff_);
                rc.erase(cac_it);
                return buff;
            }
            // there are remain datas after this message
            cac.append(&(msg.data_), ipc::data_length);
        }
    }
}

static ipc::buff_t try_recv(ipc::handle_t h) {
    return recv(h, 0);
}

}; // detail_impl<Policy>

template <typename Flag>
using policy_t = ipc::policy::choose<ipc::circ::elem_array, Flag>;

} // internal-linkage

namespace ipc {

template <typename Flag>
bool chan_impl<Flag>::connect(ipc::handle_t * ph, char const * name, unsigned mode) {
    return detail_impl<policy_t<Flag>>::connect(ph, name, mode & receiver);
}

template <typename Flag>
bool chan_impl<Flag>::reconnect(ipc::handle_t * ph, unsigned mode) {
    return detail_impl<policy_t<Flag>>::reconnect(ph, mode & receiver);
}

template <typename Flag>
void chan_impl<Flag>::disconnect(ipc::handle_t h) {
    detail_impl<policy_t<Flag>>::disconnect(h);
}

template <typename Flag>
void chan_impl<Flag>::destroy(ipc::handle_t h) {
    detail_impl<policy_t<Flag>>::destroy(h);
}

template <typename Flag>
char const * chan_impl<Flag>::name(ipc::handle_t h) {
    auto info = detail_impl<policy_t<Flag>>::info_of(h);
    return (info == nullptr) ? nullptr : info->name_.c_str();
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
bool chan_impl<Flag>::send(ipc::handle_t h, void const * data, std::size_t size, std::size_t tm) {
    return detail_impl<policy_t<Flag>>::send(h, data, size, tm);
}

template <typename Flag>
buff_t chan_impl<Flag>::recv(ipc::handle_t h, std::size_t tm) {
    return detail_impl<policy_t<Flag>>::recv(h, tm);
}

template <typename Flag>
bool chan_impl<Flag>::try_send(ipc::handle_t h, void const * data, std::size_t size, std::size_t tm) {
    return detail_impl<policy_t<Flag>>::try_send(h, data, size, tm);
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
