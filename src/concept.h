#pragma once

#include <type_traits>  // std::enable_if

namespace ipc {

// concept helpers

template <bool Cond, typename R = void>
using require = typename std::enable_if<Cond, R>::type;

#ifdef IPC_CONCEPT_
#   error "IPC_CONCEPT_ has been defined."
#endif

#define IPC_CONCEPT_(NAME, WHAT)                                       \
template <typename T>                                                  \
class NAME {                                                           \
private:                                                               \
    template <typename Type>                                           \
    static std::true_type check(decltype(std::declval<Type>().WHAT)*); \
    template <typename Type>                                           \
    static std::false_type check(...);                                 \
public:                                                                \
    using type = decltype(check<T>(nullptr));                          \
    constexpr static auto value = type::value;                         \
}

} // namespace ipc
