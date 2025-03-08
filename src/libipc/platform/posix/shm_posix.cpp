
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <atomic>
#include <string>
#include <utility>
#include <cstring>

#include "libipc/shm.h"
#include "libipc/def.h"
#include "libipc/pool_alloc.h"

#include "libipc/utility/log.h"
#include "libipc/memory/resource.h"

namespace {

struct info_t {
    std::atomic<std::int32_t> acc_;
};

struct id_info_t {
    int         fd_   = -1;
    void*       mem_  = nullptr;
    std::size_t size_ = 0;
    ipc::string name_;
};

constexpr std::size_t calc_size(std::size_t size) {
    return ((((size - 1) / alignof(info_t)) + 1) * alignof(info_t)) + sizeof(info_t);
}

inline auto& acc_of(void* mem, std::size_t size) {
    return reinterpret_cast<info_t*>(static_cast<ipc::byte_t*>(mem) + size - sizeof(info_t))->acc_;
}

} // internal-linkage

namespace ipc {
namespace shm {

id_t acquire(char const * name, std::size_t size, unsigned mode) {
    if (!is_valid_string(name)) {
        ipc::error("fail acquire: name is empty\n");
        return nullptr;
    }
    // For portable use, a shared memory object should be identified by name of the form /somename.
    // see: https://man7.org/linux/man-pages/man3/shm_open.3.html
    ipc::string op_name = ipc::string{"/"} + name;
    // Open the object for read-write access.
    int flag = O_RDWR;
    switch (mode) {
    case open:
        size = 0;
        break;
    // The check for the existence of the object, 
    // and its creation if it does not exist, are performed atomically.
    case create:
        flag |= O_CREAT | O_EXCL;
        break;
    // Create the shared memory object if it does not exist.
    default:
        flag |= O_CREAT;
        break;
    }
    int fd = ::shm_open(op_name.c_str(), flag, S_IRUSR | S_IWUSR | 
                                               S_IRGRP | S_IWGRP | 
                                               S_IROTH | S_IWOTH);
    if (fd == -1) {
        // only open shm not log error when file not exist
        if (open != mode || ENOENT != errno) {
            ipc::error("fail shm_open[%d]: %s\n", errno, op_name.c_str());
        }
        return nullptr;
    }
    ::fchmod(fd, S_IRUSR | S_IWUSR | 
                 S_IRGRP | S_IWGRP | 
                 S_IROTH | S_IWOTH);
    auto ii = mem::alloc<id_info_t>();
    ii->fd_   = fd;
    ii->size_ = size;
    ii->name_ = std::move(op_name);
    return ii;
}

std::int32_t get_ref(id_t id) {
    if (id == nullptr) {
        return 0;
    }
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ == nullptr || ii->size_ == 0) {
        return 0;
    }
    return acc_of(ii->mem_, ii->size_).load(std::memory_order_acquire);
}

void sub_ref(id_t id) {
    if (id == nullptr) {
        ipc::error("fail sub_ref: invalid id (null)\n");
        return;
    }
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ == nullptr || ii->size_ == 0) {
        ipc::error("fail sub_ref: invalid id (mem = %p, size = %zd)\n", ii->mem_, ii->size_);
        return;
    }
    acc_of(ii->mem_, ii->size_).fetch_sub(1, std::memory_order_acq_rel);
}

void * get_mem(id_t id, std::size_t * size) {
    if (id == nullptr) {
        ipc::error("fail get_mem: invalid id (null)\n");
        return nullptr;
    }
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ != nullptr) {
        if (size != nullptr) *size = ii->size_;
        return ii->mem_;
    }
    int fd = ii->fd_;
    if (fd == -1) {
        ipc::error("fail get_mem: invalid id (fd = -1)\n");
        return nullptr;
    }
    if (ii->size_ == 0) {
        struct stat st;
        if (::fstat(fd, &st) != 0) {
            ipc::error("fail fstat[%d]: %s, size = %zd\n", errno, ii->name_.c_str(), ii->size_);
            return nullptr;
        }
        ii->size_ = static_cast<std::size_t>(st.st_size);
        if ((ii->size_ <= sizeof(info_t)) || (ii->size_ % sizeof(info_t))) {
            ipc::error("fail get_mem: %s, invalid size = %zd\n", ii->name_.c_str(), ii->size_);
            return nullptr;
        }
    }
    else {
        ii->size_ = calc_size(ii->size_);
        if (::ftruncate(fd, static_cast<off_t>(ii->size_)) != 0) {
            ipc::error("fail ftruncate[%d]: %s, size = %zd\n", errno, ii->name_.c_str(), ii->size_);
            return nullptr;
        }
    }
    void* mem = ::mmap(nullptr, ii->size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        ipc::error("fail mmap[%d]: %s, size = %zd\n", errno, ii->name_.c_str(), ii->size_);
        return nullptr;
    }
    ::close(fd);
    ii->fd_  = -1;
    ii->mem_ = mem;
    if (size != nullptr) *size = ii->size_;
    acc_of(mem, ii->size_).fetch_add(1, std::memory_order_release);
    return mem;
}

std::int32_t release(id_t id) noexcept {
    if (id == nullptr) {
        ipc::error("fail release: invalid id (null)\n");
        return -1;
    }
    std::int32_t ret = -1;
    auto ii = static_cast<id_info_t*>(id);
    if (ii->mem_ == nullptr || ii->size_ == 0) {
        ipc::error("fail release: invalid id (mem = %p, size = %zd), name = %s\n", 
                    ii->mem_, ii->size_, ii->name_.c_str());
    }
    else if ((ret = acc_of(ii->mem_, ii->size_).fetch_sub(1, std::memory_order_acq_rel)) <= 1) {
        ::munmap(ii->mem_, ii->size_);
        if (!ii->name_.empty()) {
            ::shm_unlink(ii->name_.c_str());
        }
    }
    else ::munmap(ii->mem_, ii->size_);
    mem::free(ii);
    return ret;
}

void remove(id_t id) noexcept {
    if (id == nullptr) {
        ipc::error("fail remove: invalid id (null)\n");
        return;
    }
    auto ii = static_cast<id_info_t*>(id);
    auto name = std::move(ii->name_);
    release(id);
    if (!name.empty()) {
        ::shm_unlink(name.c_str());
    }
}

void remove(char const * name) noexcept {
    if (!is_valid_string(name)) {
        ipc::error("fail remove: name is empty\n");
        return;
    }
    ::shm_unlink(name);
}

} // namespace shm
} // namespace ipc
