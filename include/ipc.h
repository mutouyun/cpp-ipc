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

IPC_EXPORT bool   send(handle_t h, void const * data, std::size_t size);
IPC_EXPORT buff_t recv(handle_t h);

/*
 * This function could wait & recv messages from multi handles
 * which have the *SAME* connected name.
*/
IPC_EXPORT buff_t recv(handle_t const * hs, std::size_t size);

template <std::size_t N>
buff_t recv(handle_t const (& hs)[N]) { return recv(hs, N); }

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

} // namespace ipc
