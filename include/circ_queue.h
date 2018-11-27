#pragma once

#include "circ_elem_array.h"

namespace ipc {
namespace circ {

template <typename T>
class queue {
private:
    elem_array<sizeof(T)> elems_;
};

} // namespace circ
} // namespace ipc
