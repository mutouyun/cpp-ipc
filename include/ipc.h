#pragma once

#include <vector>
#include <string>

#include "export.h"
#include "def.h"
#include "buffer.h"
#include "shm.h"

namespace ipc {

using handle_t = void*;
using buff_t   = buffer;

template <typename Flag>
struct IPC_EXPORT chan_impl {
    static handle_t connect   (char const * name, bool start);
    static void     disconnect(handle_t h);

    static std::size_t recv_count(handle_t h);
    static bool wait_for_recv(handle_t h, std::size_t r_count, std::size_t tm);

    static bool   send(handle_t h, void const * data, std::size_t size);
    static buff_t recv(handle_t h, std::size_t tm);

    static bool   try_send(handle_t h, void const * data, std::size_t size);
    static buff_t try_recv(handle_t h);
};

template <typename Flag>
class chan_wrapper {
private:
    using detail_t = chan_impl<Flag>;

    handle_t    h_ = nullptr;
    std::string n_;

public:
    chan_wrapper() = default;

    explicit chan_wrapper(char const * name, bool start = false) {
        this->connect(name, start);
    }

    chan_wrapper(chan_wrapper&& rhs) {
        swap(rhs);
    }

    ~chan_wrapper() {
        disconnect();
    }

    void swap(chan_wrapper& rhs) {
        std::swap(h_, rhs.h_);
        n_.swap(rhs.n_);
    }

    chan_wrapper& operator=(chan_wrapper rhs) {
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

    chan_wrapper clone() const {
        return chan_wrapper { name() };
    }

    bool connect(char const * name, bool start = true) {
        if (name == nullptr || name[0] == '\0') return false;
        this->disconnect();
        h_ = detail_t::connect((n_ = name).c_str(), start);
        return valid();
    }

    void disconnect() {
        if (!valid()) return;
        detail_t::disconnect(h_);
        h_ = nullptr;
        n_.clear();
    }

    std::size_t recv_count() const {
        return detail_t::recv_count(h_);
    }

    bool wait_for_recv(std::size_t r_count, std::size_t tm = invalid_value) const {
        return detail_t::wait_for_recv(h_, r_count, tm);
    }

    static bool wait_for_recv(char const * name, std::size_t r_count, std::size_t tm = invalid_value) {
        return chan_wrapper(name).wait_for_recv(r_count, tm);
    }

    bool send    (void        const * data, std::size_t size) { return detail_t::send(h_, data, size)             ; }
    bool send    (buff_t      const & buff)                   { return     this->send(buff.data(), buff.size())   ; }
    bool send    (std::string const & str)                    { return     this->send(str.c_str(), str.size() + 1); }

    bool try_send(void        const * data, std::size_t size) { return detail_t::try_send(h_, data, size)             ; }
    bool try_send(buff_t      const & buff)                   { return     this->try_send(buff.data(), buff.size())   ; }
    bool try_send(std::string const & str)                    { return     this->try_send(str.c_str(), str.size() + 1); }

    buff_t recv(std::size_t tm = invalid_value) {
        return detail_t::recv(h_, tm);
    }

    buff_t try_recv() {
        return detail_t::try_recv(h_);
    }
};

template <typename Flag>
using chan = chan_wrapper<Flag>;

/*
 * class route
 *
 * You could use one producer/server/sender for sending messages to a route,
 * then all the consumers/clients/receivers which are receiving with this route,
 * would receive your sent messages.
 *
 * A route could only be used in 1 to N
 * (one producer/writer to multi consumers/readers)
*/

using route = chan<ipc::wr<relat::single, relat::multi, trans::broadcast>>;

/*
 * class channel
 *
 * You could use multi producers/writers for sending messages to a channel,
 * then all the consumers/readers which are receiving with this channel,
 * would receive your sent messages.
*/

using channel = chan<ipc::wr<relat::multi, relat::multi, trans::broadcast>>;

} // namespace ipc
