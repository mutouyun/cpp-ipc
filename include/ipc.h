#pragma once

#include <vector>
#include <string>

#include "export.h"
#include "def.h"
#include "shm.h"

namespace ipc {

using handle_t = void*;
using buff_t   = std::vector<byte_t>;

IPC_EXPORT buff_t make_buff(void const * data, std::size_t size);

template <std::size_t N>
buff_t make_buff(byte_t const (& data)[N]) { return make_buff(data, N); }

IPC_EXPORT handle_t connect   (char const * name);
IPC_EXPORT void     disconnect(handle_t h);

IPC_EXPORT std::size_t recv_count(handle_t h);
IPC_EXPORT void        clear_recv(handle_t h);
IPC_EXPORT void        clear_recv(char const * name);

IPC_EXPORT bool   send(handle_t h, void const * data, std::size_t size);
IPC_EXPORT buff_t recv(handle_t h);

/*
 * This function could wait & recv messages from multi handles
 * which have the *SAME* connected name.
*/
IPC_EXPORT buff_t recv(handle_t const * hs, std::size_t size);

template <std::size_t N>
buff_t recv(handle_t const (& hs)[N]) { return recv(hs, N); }

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
class IPC_EXPORT route {
public:
    route();
    route(char const * name);
    route(route&& rhs);

    ~route();

    void swap(route& rhs);
    route& operator=(route rhs);

    bool         valid() const;
    char const * name () const;

    route clone() const;

    bool connect(char const * name);
    void disconnect();

    std::size_t recv_count() const;

    bool send(void const * data, std::size_t size);
    bool send(buff_t const & buff);
    bool send(std::string const & str);

    buff_t recv();

private:
    class route_;
    route_* p_;
};

///*
// * class channel
//*/
//class IPC_EXPORT channel {
//public:
//    channel();
//    channel(char const * name);
//    channel(channel&& rhs);

//    ~channel();

//    void swap(channel& rhs);
//    channel& operator=(channel rhs);

//    bool         valid() const;
//    char const * name () const;

//    channel clone() const;

//private:
//    class channel_;
//    channel_* p_;
//};

} // namespace ipc
