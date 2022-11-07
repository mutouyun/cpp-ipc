
#include <cstdint>
#include <limits>

#include "libipc/def.h"

LIBIPC_NAMESPACE_BEG_
namespace concurrent {

/// @brief The queue index type.
using index_t = std::uint32_t;

namespace state {

/// @brief The state flag type for the queue element.
using flag_t = std::uint64_t;

enum : flag_t {
  /// @brief The invalid state value.
  invalid_value = (std::numeric_limits<flag_t>::max)(),
};

} // namespace state

} // namespace concurrent
LIBIPC_NAMESPACE_END_
