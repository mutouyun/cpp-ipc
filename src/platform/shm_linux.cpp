#include "shm.h"

#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

namespace ipc {
namespace shm {

handle_t acquire(std::string const & name, std::size_t size) {
    if (name.empty() || size == 0) {
        return nullptr;
    }
    int fd = ::shm_open(name.c_str(), O_CREAT | O_RDWR,
                        S_IRUSR | S_IWUSR |
                        S_IRGRP | S_IWGRP |
                        S_IROTH | S_IWOTH);
    if (fd == -1) {
        return nullptr;
    }
    if (::ftruncate(fd, size) != 0) {
        ::close(fd);
        return nullptr;
    }
    void* mem = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ::close(fd);
    if (mem == MAP_FAILED) {
        return nullptr;
    }
    return mem;
}

void release(handle_t h, std::size_t size) {
    if (h == nullptr) {
        return;
    }
    ::munmap(h, size);
}

void* open(handle_t h) {
    return h;
}

void close(void* /*mem*/) {
}

} // namespace shm
} // namespace ipc
