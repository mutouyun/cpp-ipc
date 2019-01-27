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
#include <mutex>

#include "def.h"
#include "log.h"
#include "memory/resource.h"

namespace {

using acc_t = std::atomic_size_t;

constexpr acc_t* acc_of(void* mem) {
    return static_cast<acc_t*>(mem);
}

constexpr void* mem_of(void* mem) {
    return static_cast<acc_t*>(mem) - 1;
}

inline auto* m2h() {
    static struct {
        std::mutex lc_;
        ipc::mem::unordered_map<void*, std::string> cache_;
    } m2h_;
    return &m2h_;
}

} // internal-linkage

namespace ipc {
namespace shm {

void* acquire(char const * name, std::size_t size) {
    if (name == nullptr || name[0] == '\0' || size == 0) {
        return nullptr;
    }
    std::string op_name = std::string{"__IPC_SHM__"} + name;
    int fd = ::shm_open(op_name.c_str(), O_CREAT | O_RDWR,
                                         S_IRUSR | S_IWUSR |
                                         S_IRGRP | S_IWGRP |
                                         S_IROTH | S_IWOTH);
    if (fd == -1) {
        ipc::error("fail shm_open[%d]: %s\n", errno, name);
        return nullptr;
    }
    size += sizeof(acc_t);
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
    auto acc = acc_of(mem);
    acc->fetch_add(1, std::memory_order_release);
    {
        IPC_UNUSED_ auto guard = ipc::detail::unique_lock(m2h()->lc_);
        m2h()->cache_.emplace(++acc, std::move(op_name));
    }
    return acc;
}

void release(void* mem, std::size_t size) {
    if (mem == nullptr) {
        return;
    }
    IPC_UNUSED_ auto guard = ipc::detail::unique_lock(m2h()->lc_);
    auto& cc = m2h()->cache_;
    auto it = cc.find(mem);
    if (it == cc.end()) {
        return;
    }
    mem = mem_of(mem);
    size += sizeof(acc_t);
    if (acc_of(mem)->fetch_sub(1, std::memory_order_acquire) == 1) {
        ::munmap(mem, size);
        ::shm_unlink(it->second.c_str());
    }
    else ::munmap(mem, size);
    cc.erase(it);
}

} // namespace shm
} // namespace ipc
