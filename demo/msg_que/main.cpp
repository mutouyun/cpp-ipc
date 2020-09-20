#include <iostream>
#include <string>

#include "libipc/ipc.h"

namespace {

constexpr char const name__[]   = "ipc-msg-que";
constexpr char const mode_s__[] = "s";
constexpr char const mode_r__[] = "r";

using msg_que_t = ipc::chan<ipc::relat::single, ipc::relat::single, ipc::trans::unicast>;

} // namespace

int main(int argc, char ** argv) {
    if (argc < 2) return 0;
    return 0;
}
