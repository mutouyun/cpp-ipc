/*
    The Capo Library
    Code covered by the MIT License
    Author: mutouyun (http://orzz.org)
*/

#pragma once

#include <type_traits>  // std::decay
#include <utility>      // std::forward

namespace capo
{
    template <template <typename...> class Ret, typename... T>
    using return_t = Ret<typename std::decay<T>::type...>;

    template <template <typename...> class Ret, typename... T>
    inline return_t<Ret, T...> make(T&&... args)
    {
        return return_t<Ret, T...>(std::forward<T>(args)...);
    }
}