#include <array>
#include <string>

#include "ipc.h"
#include "shm.h"
#include "rw_lock.h"
#include "id_pool.h"

#include "platform/detail.h"

namespace {

using namespace ipc;

struct ch_info_t {
    rw_lock lc_;
    id_pool ch_acc_; // only support 255 channels with one name
};

struct ch_multi_routes {
    shm::handle h_;
    route       r_;

    std::size_t id_;
    bool        marked_ = false;

    std::array<route, id_pool::max_count> rts_;

    ch_info_t& info() {
        return *static_cast<ch_info_t*>(h_.get());
    }

    auto& acc() {
        return info().ch_acc_;
    }

    void mark_id() {
        if (marked_) return;
        marked_ = true;
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(info().lc_);
        acc().mark_acquired(id_);
    }

    auto& sender() {
        mark_id();
        return r_;
    }

    bool valid() const {
        return h_.valid() && r_.valid();
    }

    bool connect(char const * name) {
        if (name == nullptr || name[0] == '\0') {
            return false;
        }
        disconnect();
        if (!h_.acquire((std::string { name } + "_").c_str(), sizeof(ch_info_t))) {
            return false;
        }
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(info().lc_);
            if (acc().invalid()) acc().init();
            id_ = acc().acquire();
        }
        if (id_ == invalid_value) {
            return false;
        }
        r_.connect((name + std::to_string(id_)).c_str());
        return valid();
    }

    void disconnect() {
        if (!valid()) return;
        {
            IPC_UNUSED_ auto guard = ipc::detail::unique_lock(info().lc_);
            acc().release(id_);
        }
        for (auto& rt : rts_) {
            rt.disconnect();
        }
        r_.disconnect();
        h_.release();
    }
};

} // internal-linkage

namespace ipc {

ipc::handle_t channel_detail<prod_cons_routes>::connect(char const * /*name*/) {
    return nullptr;
}

void channel_detail<prod_cons_routes>::disconnect(ipc::handle_t /*h*/) {
}

std::size_t channel_detail<prod_cons_routes>::recv_count(ipc::handle_t /*h*/) {
    return 0;
}

bool channel_detail<prod_cons_routes>::wait_for_recv(ipc::handle_t /*h*/, std::size_t /*r_count*/) {
    return false;
}

bool channel_detail<prod_cons_routes>::send(ipc::handle_t /*h*/, void const * /*data*/, std::size_t /*size*/) {
    return false;
}

buff_t channel_detail<prod_cons_routes>::recv(ipc::handle_t /*h*/) {
    return {};
}

} // namespace ipc
