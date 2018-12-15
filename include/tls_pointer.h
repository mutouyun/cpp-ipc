#pragma once

#include <cstdint>
#include <utility>
#include <limits>

namespace ipc {
namespace tls {

using key_t        = std::uint64_t;
using destructor_t = void (*)(void*);

enum : key_t {
    invalid_value = (std::numeric_limits<key_t>::max)()
};

key_t create (destructor_t destructor = nullptr);
void  release(key_t key);

bool  set(key_t key, void* ptr);
void* get(key_t key);

////////////////////////////////////////////////////////////////
/// Thread-local pointer
////////////////////////////////////////////////////////////////

/*
    <Remarks>

 1. In Windows, if you do not compile thread_local_ptr.cpp,
    use thread_local_ptr will cause memory leaks.

 2. You need to set the thread_local_ptr's storage manually:
    ```
        thread_local_ptr<int> p;
        if (!p) p = new int(123);
    ```
    Just like an ordinary pointer. Or you could just call create:
    ```
        thread_local_ptr<int> p;
        p.create(123);
    ```
*/

template <typename T>
class pointer {
    key_t key_;

public:
    using value_type = T;

    pointer() {
        key_ = tls::create([](void* p) { delete static_cast<T*>(p); });
    }

    ~pointer() {
        tls::release(key_);
    }

    template <typename... P>
    T* create(P&&... params) {
        auto ptr = static_cast<T*>(*this);
        if (ptr == nullptr) {
            return (*this) = new T { std::forward<P>(params)... };
        }
        return ptr;
    }

    T* operator=(T* ptr) {
        set(key_, ptr);
        return ptr;
    }

    operator T*() const { return static_cast<T*>(get(key_)); }

    T&       operator* ()       { return *static_cast<T*>(*this); }
    const T& operator* () const { return *static_cast<T*>(*this); }

    T*       operator->()       { return  static_cast<T*>(*this); }
    const T* operator->() const { return  static_cast<T*>(*this); }
};

} // namespace tls
} // namespace ipc
