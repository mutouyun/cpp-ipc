#pragma once

#include <vector>
#include <string>

#include "export.h"
#include "def.h"
#include "buffer.h"
#include "shm.h"
#include "queue.h"

namespace ipc {

using handle_t = void*;
using buff_t   = buffer;

template <template <typename...> class Queue, typename Policy>
struct IPC_EXPORT channel_detail {
    static handle_t connect   (char const * name);
    static void     disconnect(handle_t h);

    static std::size_t recv_count(handle_t h);
    static void wait_for_recv(handle_t h, std::size_t r_count);

    static void clear_recv(handle_t h);
    static void clear_recv(char const * name);

    static bool   send(handle_t h, void const * data, std::size_t size);
    static buff_t recv(handle_t h);
};

template <typename Detail>
class channel_ipml {
private:
    handle_t    h_ = nullptr;
    std::string n_;

public:
    channel_ipml() = default;

    explicit channel_ipml(char const * name) {
        this->connect(name);
    }

    channel_ipml(channel_ipml&& rhs) {
        swap(rhs);
    }

    ~channel_ipml() {
        disconnect();
    }

    void swap(channel_ipml& rhs) {
        std::swap(h_, rhs.h_);
        n_.swap(rhs.n_);
    }

    channel_ipml& operator=(channel_ipml rhs) {
        swap(rhs);
        return *this;
    }

    char const * name() const {
        return n_.c_str();
    }

    handle_t handle() const {
        return h_;
    }

    bool valid() const {
        return (handle() != nullptr);
    }

    channel_ipml clone() const {
        return channel_ipml { name() };
    }

    bool connect(char const * name) {
        if (name == nullptr || name[0] == '\0') return false;
        this->disconnect();
        h_ = Detail::connect((n_ = name).c_str());
        return valid();
    }

    void disconnect() {
        if (!valid()) return;
        Detail::disconnect(h_);
        h_ = nullptr;
        n_.clear();
    }

    std::size_t recv_count() const {
        return Detail::recv_count(h_);
    }

    void wait_for_recv(std::size_t r_count) const {
        return Detail::wait_for_recv(h_, r_count);
    }

    static void wait_for_recv(char const * name, std::size_t r_count) {
        return channel_ipml(name).wait_for_recv(r_count);
    }

    void clear_recv() {
        Detail::clear_recv(h_);
    }

    static void clear_recv(char const * name) {
        Detail::clear_recv(name);
    }

    bool send(void const * data, std::size_t size) {
        return Detail::send(h_, data, size);
    }

    bool send(buff_t const & buff) {
        return this->send(buff.data(), buff.size());
    }

    bool send(std::string const & str) {
        return this->send(str.c_str(), str.size() + 1);
    }

    buff_t recv() {
        return Detail::recv(h_);
    }
};

/*
 * class route
 *
 * You could use one producer/server/sender for sending messages to a route,
 * then all the consumers/clients/receivers which are receiving with this route,
 * would receive your sent messages.
 *
 * A route could only be used in 1 to N
 * (one producer/server/sender to multi consumers/clients/receivers)
*/
using route = channel_ipml<channel_detail<
    ipc::queue, ipc::prod_cons_circ<relat::single, relat::multi, trans::broadcast>
>>;

/*
 * class channel
 *
 * You could use multi producers/servers/senders for sending messages to a channel,
 * then all the consumers/clients/receivers which are receiving with this channel,
 * would receive your sent messages.
*/

using channel = channel_ipml<channel_detail<
    ipc::queue, ipc::prod_cons_circ<relat::multi, relat::multi, trans::broadcast>
>>;

} // namespace ipc
