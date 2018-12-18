#include "shm.h"

#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

namespace ipc {
namespace shm {

void* acquire(char const * name, std::size_t size) {
    if (name == nullptr || name[0] == '\0' || size == 0) {
        return nullptr;
    }
    int fd = ::shm_open(name, O_CREAT | O_RDWR,
                              S_IRUSR | S_IWUSR |
                              S_IRGRP | S_IWGRP |
                              S_IROTH | S_IWOTH);
    if (fd == -1) {
        return nullptr;
    }
    if (::ftruncate(fd, static_cast<off_t>(size)) != 0) {
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

void release(void* mem, std::size_t size) {
    if (mem == nullptr) {
        return;
    }
    ::munmap(mem, size);
}

} // namespace shm
} // namespace ipc
