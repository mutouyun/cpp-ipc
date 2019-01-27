#pragma once

#include <pthread.h>

#include <cstring>
#include <atomic>
#include <string>
#include <utility>
#include <tuple>

#include "def.h"
#include "log.h"
#include "shm.h"
#include "rw_lock.h"
#include "id_pool.h"
#include "pool_alloc.h"

#include "platform/detail.h"

namespace ipc {
namespace detail {

class mutex {
    pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;

public:
    pthread_mutex_t& native() {
        return mutex_;
    }

    bool open() {
        int eno;
        // init mutex
        pthread_mutexattr_t mutex_attr;
        if ((eno = ::pthread_mutexattr_init(&mutex_attr)) != 0) {
            ipc::error("fail pthread_mutexattr_init[%d]\n", eno);
            return false;
        }
        IPC_UNUSED_ auto guard_mutex_attr = unique_ptr(&mutex_attr, ::pthread_mutexattr_destroy);
        if ((eno = ::pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED)) != 0) {
            ipc::error("fail pthread_mutexattr_setpshared[%d]\n", eno);
            return false;
        }
        if ((eno = ::pthread_mutex_init(&mutex_, &mutex_attr)) != 0) {
            ipc::error("fail pthread_mutex_init[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool close() {
        int eno;
        if ((eno = ::pthread_mutex_destroy(&mutex_)) != 0) {
            ipc::error("fail pthread_mutex_destroy[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool lock() {
        int eno;
        if ((eno = ::pthread_mutex_lock(&mutex_)) != 0) {
            ipc::error("fail pthread_mutex_lock[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool unlock() {
        int eno;
        if ((eno = ::pthread_mutex_unlock(&mutex_)) != 0) {
            ipc::error("fail pthread_mutex_unlock[%d]\n", eno);
            return false;
        }
        return true;
    }
};

class condition {
    pthread_cond_t cond_  = PTHREAD_COND_INITIALIZER;

public:
    bool open() {
        int eno;
        // init condition
        pthread_condattr_t cond_attr;
        if ((eno = ::pthread_condattr_init(&cond_attr)) != 0) {
            ipc::error("fail pthread_condattr_init[%d]\n", eno);
            return false;
        }
        IPC_UNUSED_ auto guard_cond_attr = unique_ptr(&cond_attr, ::pthread_condattr_destroy);
        if ((eno = ::pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED)) != 0) {
            ipc::error("fail pthread_condattr_setpshared[%d]\n", eno);
            return false;
        }
        if ((eno = ::pthread_cond_init(&cond_, &cond_attr)) != 0) {
            ipc::error("fail pthread_cond_init[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool close() {
        int eno;
        if ((eno = ::pthread_cond_destroy(&cond_)) != 0) {
            ipc::error("fail pthread_cond_destroy[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool wait(mutex& mtx) {
        int eno;
        if ((eno = ::pthread_cond_wait(&cond_, &mtx.native())) != 0) {
            ipc::error("fail pthread_cond_wait[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool notify() {
        int eno;
        if ((eno = ::pthread_cond_signal(&cond_)) != 0) {
            ipc::error("fail pthread_cond_signal[%d]\n", eno);
            return false;
        }
        return true;
    }

    bool broadcast() {
        int eno;
        if ((eno = ::pthread_cond_broadcast(&cond_)) != 0) {
            ipc::error("fail pthread_cond_broadcast[%d]\n", eno);
            return false;
        }
        return true;
    }
};

class event {
    using cnt_t = std::atomic<std::size_t>;

    struct info_t {
        cnt_t     cnt_;
        mutex     mutex_;
        condition cond_;
    } * info_ = nullptr;

    uint_t<16> wait_id_;

    std::string name() const {
        return "__IPC_WAIT__" + std::to_string(wait_id_);
    }

    void open() {
        auto n = name();
        info_ = static_cast<info_t*>(shm::acquire(n.c_str(), sizeof(info_t)));
        if (info_ == nullptr) {
            ipc::error("fail shm::acquire: %s\n", n.c_str());
            return;
        }
        if (info_->cnt_.fetch_add(1, std::memory_order_acq_rel) == 0) {
            if (!info_->mutex_.open()) return;
            if (!info_->cond_.open()) return;
        }
        info_->mutex_.lock();
    }

    void close() {
        info_->mutex_.unlock();
        if (info_->cnt_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            info_->cond_.close();
            info_->mutex_.close();
        }
        shm::release(info_, sizeof(info_t));
    }

public:
    event(std::size_t id)
        : wait_id_(static_cast<uint_t<16>>(id)) {
        open();
    }

    ~event() {
        close();
    }

    auto get_id() const noexcept {
        return wait_id_;
    }

    bool wait() {
        if (info_ == nullptr) return false;
        return info_->cond_.wait(info_->mutex_);
    }

    bool notify() {
        if (info_ == nullptr) return false;
        return info_->cond_.notify();
    }
};

class waiter {
    using evt_id_t = decltype(std::declval<event>().get_id());

    std::atomic<unsigned> counter_ { 0 };
    spin_lock evt_lc_;
    id_pool<sizeof(evt_id_t)> evt_ids_;

    std::size_t push_event(event const & evt) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(evt_lc_);
        std::size_t id = evt_ids_.acquire();
        if (id == invalid_value) {
            ipc::error("fail push_event[has too many waiters]\n");
            return id;
        }
        (*static_cast<evt_id_t*>(evt_ids_.at(id))) = evt.get_id();
        evt_ids_.mark_acquired(id);
        return id;
    }

    void pop_event(std::size_t id) {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(evt_lc_);
        evt_ids_.release(id);
    }

public:
    using handle_t = waiter*;

    constexpr static handle_t invalid() {
        return nullptr;
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') {
            return invalid();
        }
        if (counter_.fetch_add(1, std::memory_order_acq_rel) == 0) {
            evt_ids_.init();
        }
        return this;
    }

    void close(handle_t h) {
        if (h == invalid()) return;
        counter_.fetch_sub(1, std::memory_order_acq_rel);
    }

    static bool wait_all(std::tuple<waiter*, handle_t> const * all, std::size_t size) {
        if (all == nullptr || size == 0) {
            return false;
        }
        // calc a new wait-id & construct event object
        event evt { ipc::detail::calc_unique_id() };
        auto ids = static_cast<std::size_t*>(mem::alloc(sizeof(std::size_t) * size));
        for (std::size_t i = 0; i < size; ++i) {
            ids[i] = std::get<0>(all[i])->push_event(evt);
        }
        IPC_UNUSED_ auto guard = unique_ptr(ids, [all, size](std::size_t* ids) {
            for (std::size_t i = 0; i < size; ++i) {
                std::get<0>(all[i])->pop_event(ids[i]);
            }
            mem::free(ids, sizeof(std::size_t) * size);
        });
        // wait for event signal
        return evt.wait();
    }

    bool wait(handle_t h) {
        if (h == invalid()) return false;
        auto info = std::make_tuple(this, h);
        return wait_all(&info, 1);
    }

    void notify(handle_t h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(evt_lc_);
        evt_ids_.for_acquired([this](auto id) {
            event evt { *static_cast<evt_id_t*>(evt_ids_.at(id)) };
            evt.notify();
            return false;
        });
    }

    void broadcast(handle_t h) {
        if (h == invalid()) return;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(evt_lc_);
        evt_ids_.for_acquired([this](auto id) {
            event evt { *static_cast<evt_id_t*>(evt_ids_.at(id)) };
            evt.notify();
            return true;
        });
    }
};

} // namespace detail
} // namespace ipc
