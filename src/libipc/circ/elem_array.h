#pragma once

#include <atomic>   // std::atomic<?>
#include <limits>
#include <utility>
#include <type_traits>

#include "libipc/def.h"
#include "libipc/rw_lock.h"

#include "libipc/circ/elem_def.h"
#include "libipc/platform/detail.h"

namespace ipc {
namespace circ {

template <typename Policy,
          std::size_t DataSize,
          std::size_t AlignSize = (ipc::detail::min)(DataSize, alignof(std::max_align_t))>
class elem_array : public ipc::circ::conn_head<Policy> {
public:
    using base_t   = ipc::circ::conn_head<Policy>;
    using policy_t = Policy;
    using cursor_t = decltype(std::declval<policy_t>().cursor());
    using elem_t   = typename policy_t::template elem_t<DataSize, AlignSize>;

    enum : std::size_t {
        head_size  = sizeof(base_t) + sizeof(policy_t),
        data_size  = DataSize,
        elem_max   = (std::numeric_limits<uint_t<8>>::max)() + 1, // default is 255 + 1
        elem_size  = sizeof(elem_t),
        block_size = elem_size * elem_max
    };

private:
    policy_t head_;
    elem_t   block_[elem_max] {};

    /**
     * \remarks 'warning C4348: redefinition of default parameter' with MSVC.
     * \see
     *  - https://stackoverflow.com/questions/12656239/redefinition-of-default-template-parameter
     *  - https://developercommunity.visualstudio.com/content/problem/425978/incorrect-c4348-warning-in-nested-template-declara.html
    */
    template <typename P, bool/* = relat_trait<P>::is_multi_producer*/>
    struct sender_checker;

    template <typename P>
    struct sender_checker<P, true> {
        constexpr static bool connect() noexcept {
            // always return true
            return true;
        }
        constexpr static void disconnect() noexcept {}
    };

    template <typename P>
    struct sender_checker<P, false> {
        bool connect() noexcept {
            return !flag_.test_and_set(std::memory_order_acq_rel);
        }
        void disconnect() noexcept {
            flag_.clear();
        }

    private:
        // in shm, it should be 0 whether it's initialized or not.
        std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    };

    template <typename P, bool/* = relat_trait<P>::is_multi_consumer*/>
    struct receiver_checker;

    template <typename P>
    struct receiver_checker<P, true> {
        constexpr static cc_t connect(base_t &conn) noexcept {
            return conn.connect();
        }
        constexpr static cc_t disconnect(base_t &conn, cc_t cc_id) noexcept {
            return conn.disconnect(cc_id);
        }
    };

    template <typename P>
    struct receiver_checker<P, false> : protected sender_checker<P, false> {
        cc_t connect(base_t &conn) noexcept {
            return sender_checker<P, false>::connect() ? conn.connect() : 0;
        }
        cc_t disconnect(base_t &conn, cc_t cc_id) noexcept {
            sender_checker<P, false>::disconnect();
            return conn.disconnect(cc_id);
        }
    };

    sender_checker  <policy_t, relat_trait<policy_t>::is_multi_producer> s_ckr_;
    receiver_checker<policy_t, relat_trait<policy_t>::is_multi_consumer> r_ckr_;

    // make these be private
    using base_t::connect;
    using base_t::disconnect;

public:
    bool connect_sender() noexcept {
        return s_ckr_.connect();
    }

    void disconnect_sender() noexcept {
        return s_ckr_.disconnect();
    }

    cc_t connect_receiver() noexcept {
        return r_ckr_.connect(*this);
    }

    cc_t disconnect_receiver(cc_t cc_id) noexcept {
        return r_ckr_.disconnect(*this, cc_id);
    }

    cursor_t cursor() const noexcept {
        return head_.cursor();
    }

    template <typename Q, typename F>
    bool push(Q* que, F&& f) {
        return head_.push(que, std::forward<F>(f), block_);
    }

    template <typename Q, typename F>
    bool force_push(Q* que, F&& f) {
        return head_.force_push(que, std::forward<F>(f), block_);
    }

    template <typename Q, typename F, typename R>
    bool pop(Q* que, cursor_t* cur, F&& f, R&& out) {
        if (cur == nullptr) return false;
        return head_.pop(que, *cur, std::forward<F>(f), std::forward<R>(out), block_);
    }
};

} // namespace circ
} // namespace ipc
