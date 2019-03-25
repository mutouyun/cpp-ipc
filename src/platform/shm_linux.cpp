#include "shm.h"

#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <atomic>
#include <string>
#include <utility>
#include <cstring>

#include "def.h"
#include "log.h"
#include "memory/resource.h"

namespace {

struct info_t {
    std::atomic_size_t acc_;
    char name_[ipc::name_length];
};

constexpr std::size_t calc_size(std::size_t size) {
    return ((((size - 1) / alignof(info_t)) + 1) * alignof(info_t)) + sizeof(info_t);
}

inline auto& acc_of(void* mem, std::size_t size) {
    return reinterpret_cast<info_t*>(static_cast<ipc::byte_t*>(mem) + size - sizeof(info_t))->acc_;
}

inline auto& str_of(void* mem, std::size_t size) {
    return reinterpret_cast<info_t*>(static_cast<ipc::byte_t*>(mem) + size - sizeof(info_t))->name_;
}

} // internal-linkage

namespace ipc {
namespace shm {

id_t acquire(char const * name, std::size_t size) {
    if (name == nullptr || name[0] == '\0' || size == 0) {
        return nullptr;
    }
    std::string op_name = std::string{"__IPC_SHM__"} + name;
    if (op_name.size() >= ipc::name_length) {
        ipc::error("name is too long!: [%d]%s\n", static_cast<int>(op_name.size()), op_name.c_str());
        return nullptr;
    }
    int fd = ::shm_open(op_name.c_str(), O_CREAT | O_RDWR,
                                         S_IRUSR | S_IWUSR |
                                         S_IRGRP | S_IWGRP |
                                         S_IROTH | S_IWOTH);
    if (fd == -1) {
        ipc::error("fail shm_open[%d]: %s\n", errno, name);
        return nullptr;
    }
    size = calc_size(size);
    if (::ftruncate(fd, static_cast<off_t>(size)) != 0) {
        ipc::error("fail ftruncate[%d]: %s\n", errno, name);
        ::close(fd);
        ::shm_unlink(op_name.c_str());
        return nullptr;
    }
    void* mem = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ::close(fd);
    if (mem == MAP_FAILED) {
        ipc::error("fail mmap[%d]: %s\n", errno, name);
        ::shm_unlink(op_name.c_str());
        return nullptr;
    }
    if (acc_of(mem, size).fetch_add(1, std::memory_order_release) == 0) {
        std::memcpy(&str_of(mem, size), op_name.c_str(), op_name.size());
    }
    return static_cast<id_t>(mem);
}

void * to_mem(id_t id) {
    return static_cast<void *>(id);
}

void release(id_t id, void * mem, std::size_t size) {
    if (mem == nullptr) {
        return;
    }
    if (mem != to_mem(id)) {
        return;
    }
    size = calc_size(size);
    if (acc_of(mem, size).fetch_sub(1, std::memory_order_acquire) == 1) {
        char name[ipc::name_length] = {};
        std::memcpy(name, &str_of(mem, size), sizeof(name));
        ::munmap(mem, size);
        ::shm_unlink(name);
    }
    else ::munmap(mem, size);
}

} // namespace shm
} // namespace ipc
