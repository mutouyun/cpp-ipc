#pragma once

#include <atomic>
#include <thread>
#include <cstring>
#include <utility>
#include <type_traits>

#include "def.h"
#include "rw_lock.h"

#include "circ/elem_def.h"
#include "platform/detail.h"

namespace ipc {
namespace circ {

////////////////////////////////////////////////////////////////
/// element-array implementation
////////////////////////////////////////////////////////////////

template <std::size_t DataSize, typename Policy>
class elem_array : public ipc::circ::conn_head {
public:
    using base_t   = ipc::circ::conn_head;
    using policy_t = Policy;
#if __cplusplus >= 201703L
    using elem_t   = ipc::circ::elem_t<policy_t::template elem_param<DataSize>>;
#else /*__cplusplus < 201703L*/
    using elem_t   = ipc::circ::elem_t<policy_t::template elem_param<DataSize>::value>;
#endif/*__cplusplus < 201703L*/

    enum : std::size_t {
        head_size  = sizeof(base_t) + sizeof(policy_t),
        data_size  = DataSize,
        elem_max   = (std::numeric_limits<uint_t<8>>::max)() + 1, // default is 255 + 1
        elem_size  = sizeof(elem_t),
        block_size = elem_size * elem_max
    };

private:
    policy_t head_;
    elem_t   block_[elem_max];

public:
    auto cursor() const noexcept { return head_.cursor(); }

    template <typename F>
    bool push(F&& f) {
        return head_.push(this, std::forward<F>(f), block_);
    }

    template <typename F>
    bool pop(decltype(std::declval<policy_t>().cursor())* cur, F&& f) {
        if (cur == nullptr) return false;
        return head_.pop(this, *cur, std::forward<F>(f), block_);
    }
};

} // namespace circ
} // namespace ipc
