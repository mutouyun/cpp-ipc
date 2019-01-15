#pragma once

namespace ipc {
namespace detail {

class waiter {
public:
    using handle_t = void*;

    constexpr static handle_t invalid() {
        return nullptr;
    }

    handle_t open(char const * name) {
        if (name == nullptr || name[0] == '\0') return invalid();
        return invalid();
    }

    static void close(handle_t /*h*/) {
    }

    bool wait(handle_t /*h*/) {
        return false;
    }

    void notify(handle_t /*h*/) {
    }

    void broadcast(handle_t /*h*/) {
    }
};

} // namespace detail
} // namespace ipc
