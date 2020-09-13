#pragma once

#include <limits>
#include <utility>
#include <type_traits>

#include "libipc/def.h"

#include "libipc/circ/elem_def.h"
#include "libipc/platform/detail.h"

namespace ipc {
namespace circ {

template <typename Policy,
          std::size_t DataSize,
          std::size_t AlignSize = (ipc::detail::min)(DataSize, alignof(std::max_align_t))>
class elem_array : public ipc::circ::conn_head {
public:
    using base_t   = ipc::circ::conn_head;
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

public:
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

    template <typename Q, typename F>
    bool pop(Q* que, cursor_t* cur, F&& f) {
        if (cur == nullptr) return false;
        return head_.pop(que, *cur, std::forward<F>(f), block_);
    }
};

} // namespace circ
} // namespace ipc
