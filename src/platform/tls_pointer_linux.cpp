#include "tls_pointer.h"

#include <pthread.h> // pthread_...

namespace ipc {
namespace tls {

key_t create(destructor_t destructor) {
    pthread_key_t k;
    if (pthread_key_create(&k, destructor) == 0) {
        return static_cast<key_t>(k);
    }
    return invalid_value;
}

void release(key_t key) {
    pthread_key_delete(static_cast<pthread_key_t>(key));
}

bool set(key_t key, void* ptr) {
    return pthread_setspecific(static_cast<pthread_key_t>(key), ptr) == 0;
}

void* get(key_t key) {
    return pthread_getspecific(static_cast<pthread_key_t>(key));
}

} // namespace tls
} // namespace ipc
