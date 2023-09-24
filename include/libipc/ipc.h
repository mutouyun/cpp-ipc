#pragma once

#include <string>

#include "libipc/export.h"
#include "libipc/def.h"
#include "libipc/buffer.h"
#include "libipc/shm.h"

namespace ipc {

using handle_t = void*;
using buff_t   = buffer;

enum : unsigned {
    sender,
    receiver
};

template <typename Flag>
struct IPC_EXPORT chan_impl {
    static ipc::handle_t inited();

    static bool connect   (ipc::handle_t * ph, char const * name, unsigned mode);
    static bool connect   (ipc::handle_t * ph, prefix, char const * name, unsigned mode);
    static bool reconnect (ipc::handle_t * ph, unsigned mode);
    static void disconnect(ipc::handle_t h);
    static void destroy   (ipc::handle_t h);

    static char const * name(ipc::handle_t h);

    static std::size_t recv_count(ipc::handle_t h);
    static bool wait_for_recv(ipc::handle_t h, std::size_t r_count, std::uint64_t tm);

    static bool   send(ipc::handle_t h, void const * data, std::size_t size, std::uint64_t tm);
    static buff_t recv(ipc::handle_t h, std::uint64_t tm);

    static bool   try_send(ipc::handle_t h, void const * data, std::size_t size, std::uint64_t tm);
    static buff_t try_recv(ipc::handle_t h);
};

template <typename Flag>
class chan_wrapper {
private:
    using detail_t = chan_impl<Flag>;

    ipc::handle_t h_ = detail_t::inited();
    unsigned mode_   = ipc::sender;
    bool connected_  = false;

public:
    chan_wrapper() noexcept = default;

    explicit chan_wrapper(char const * name, unsigned mode = ipc::sender)
        : connected_{this->connect(name, mode)} {
    }

    chan_wrapper(prefix pref, char const * name, unsigned mode = ipc::sender)
        : connected_{this->connect(pref, name, mode)} {
    }

    chan_wrapper(chan_wrapper&& rhs) noexcept
        : chan_wrapper{} {
        swap(rhs);
    }

    ~chan_wrapper() {
        detail_t::destroy(h_);
    }

    void swap(chan_wrapper& rhs) noexcept {
        std::swap(h_        , rhs.h_);
        std::swap(mode_     , rhs.mode_);
        std::swap(connected_, rhs.connected_);
    }

    chan_wrapper& operator=(chan_wrapper rhs) noexcept {
        swap(rhs);
        return *this;
    }

    char const * name() const noexcept {
        return detail_t::name(h_);
    }

    ipc::handle_t handle() const noexcept {
        return h_;
    }

    bool valid() const noexcept {
        return (handle() != nullptr);
    }

    unsigned mode() const noexcept {
        return mode_;
    }

    chan_wrapper clone() const {
        return chan_wrapper { name(), mode_ };
    }

    /**
     * Building handle, then try connecting with name & mode flags.
    */
    bool connect(char const * name, unsigned mode = ipc::sender | ipc::receiver) {
        if (name == nullptr || name[0] == '\0') return false;
        detail_t::disconnect(h_); // clear old connection
        return connected_ = detail_t::connect(&h_, name, mode_ = mode);
    }
    bool connect(prefix pref, char const * name, unsigned mode = ipc::sender | ipc::receiver) {
        if (name == nullptr || name[0] == '\0') return false;
        detail_t::disconnect(h_); // clear old connection
        return connected_ = detail_t::connect(&h_, pref, name, mode_ = mode);
    }

    /**
     * Try connecting with new mode flags.
    */
    bool reconnect(unsigned mode) {
        if (!valid()) return false;
        if (connected_ && (mode_ == mode)) return true;
        return connected_ = detail_t::reconnect(&h_, mode_ = mode);
    }

    void disconnect() {
        if (!valid()) return;
        detail_t::disconnect(h_);
        connected_ = false;
    }

    std::size_t recv_count() const {
        return detail_t::recv_count(h_);
    }

    bool wait_for_recv(std::size_t r_count, std::uint64_t tm = invalid_value) const {
        return detail_t::wait_for_recv(h_, r_count, tm);
    }

    static bool wait_for_recv(char const * name, std::size_t r_count, std::uint64_t tm = invalid_value) {
        return chan_wrapper(name).wait_for_recv(r_count, tm);
    }

    /**
     * If timeout, this function would call 'force_push' to send the data forcibly.
    */
    bool send(void const * data, std::size_t size, std::uint64_t tm = default_timeout) {
        return detail_t::send(h_, data, size, tm);
    }
    bool send(buff_t const & buff, std::uint64_t tm = default_timeout) {
        return this->send(buff.data(), buff.size(), tm);
    }
    bool send(std::string const & str, std::uint64_t tm = default_timeout) {
        return this->send(str.c_str(), str.size() + 1, tm);
    }

    /**
     * If timeout, this function would just return false.
    */
    bool try_send(void const * data, std::size_t size, std::uint64_t tm = default_timeout) {
        return detail_t::try_send(h_, data, size, tm);
    }
    bool try_send(buff_t const & buff, std::uint64_t tm = default_timeout) {
        return this->try_send(buff.data(), buff.size(), tm);
    }
    bool try_send(std::string const & str, std::uint64_t tm = default_timeout) {
        return this->try_send(str.c_str(), str.size() + 1, tm);
    }

    buff_t recv(std::uint64_t tm = invalid_value) {
        return detail_t::recv(h_, tm);
    }

    buff_t try_recv() {
        return detail_t::try_recv(h_);
    }
};

template <relat Rp, relat Rc, trans Ts>
using chan = chan_wrapper<ipc::wr<Rp, Rc, Ts>>;

/**
 * \class route
 *
 * \note You could use one producer/server/sender for sending messages to a route,
 *       then all the consumers/clients/receivers which are receiving with this route,
 *       would receive your sent messages.
 *       A route could only be used in 1 to N (one producer/writer to multi consumers/readers).
*/
using route = chan<relat::single, relat::multi, trans::broadcast>;

/**
 * \class channel
 *
 * \note You could use multi producers/writers for sending messages to a channel,
 *       then all the consumers/readers which are receiving with this channel,
 *       would receive your sent messages.
*/
using channel = chan<relat::multi, relat::multi, trans::broadcast>;

} // namespace ipc
