
#include "libipc/mem/new.h"

namespace ipc {
namespace mem {

/// \brief Select the incremental level based on the size.
constexpr inline std::size_t regular_level(std::size_t s) noexcept {
  return (s <= 128  ) ? 0 :
         (s <= 1024 ) ? 1 :
         (s <= 8192 ) ? 2 :
         (s <= 65536) ? 3 : 4;
}

/// \brief Use block pools to handle memory less than 64K.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_resource_base : public block_pool<BlockSize, BlockPoolExpansion>
                          , public block_collector {
public:
  void *allocate(std::size_t /*bytes*/) noexcept override {
    return block_pool<BlockSize, BlockPoolExpansion>::allocate();
  }

  void deallocate(void *p, std::size_t /*bytes*/) noexcept override {
    block_pool<BlockSize, BlockPoolExpansion>::deallocate(p);
  }
};

/// \brief Use `new`/`delete` to handle memory larger than 64K.
template <>
class block_resource_base<0, 0> : public new_delete_resource
                                , public block_collector {
public:
  void *allocate(std::size_t bytes) noexcept override {
    return new_delete_resource::allocate(bytes);
  }

  void deallocate(void *p, std::size_t bytes) noexcept override {
    new_delete_resource::deallocate(p, bytes);
  }
};

/// \brief Defines block pool memory resource based on block pool.
template <std::size_t BlockSize, std::size_t BlockPoolExpansion>
class block_pool_resource : public block_resource_base<BlockSize, BlockPoolExpansion> {
public:
  static block_collector &get() noexcept {
    thread_local block_pool_resource instance;
    return instance;
  }
};

/// \brief Matches the appropriate memory block resource based on a specified size.
block_collector &get_regular_resource(std::size_t s) noexcept {
  std::size_t l = regular_level(s);
  switch (l) {
  case 0:
    switch (round_up<std::size_t>(s, 16)) {
    case 16 : return block_pool_resource<16 , 512>::get();
    case 32 : return block_pool_resource<32 , 512>::get();
    case 48 : return block_pool_resource<48 , 512>::get();
    case 64 : return block_pool_resource<64 , 512>::get();
    case 80 : return block_pool_resource<80 , 512>::get();
    case 96 : return block_pool_resource<96 , 512>::get();
    case 112: return block_pool_resource<112, 512>::get();
    case 128: return block_pool_resource<128, 512>::get();
    default : break;
    }
    break;
  case 1:
    switch (round_up<std::size_t>(s, 128)) {
    case 256 : return block_pool_resource<256 , 256>::get();
    case 384 : return block_pool_resource<384 , 256>::get();
    case 512 : return block_pool_resource<512 , 256>::get();
    case 640 : return block_pool_resource<640 , 256>::get();
    case 768 : return block_pool_resource<768 , 256>::get();
    case 896 : return block_pool_resource<896 , 256>::get();
    case 1024: return block_pool_resource<1024, 256>::get();
    default  : break;
    }
    break;
  case 2:
    switch (round_up<std::size_t>(s, 1024)) {
    case 2048: return block_pool_resource<2048, 128>::get();
    case 3072: return block_pool_resource<3072, 128>::get();
    case 4096: return block_pool_resource<4096, 128>::get();
    case 5120: return block_pool_resource<5120, 128>::get();
    case 6144: return block_pool_resource<6144, 128>::get();
    case 7168: return block_pool_resource<7168, 128>::get();
    case 8192: return block_pool_resource<8192, 128>::get();
    default  : break;
    }
    break;
  case 3:
    switch (round_up<std::size_t>(s, 8192)) {
    case 16384: return block_pool_resource<16384, 64>::get();
    case 24576: return block_pool_resource<24576, 64>::get();
    case 32768: return block_pool_resource<32768, 64>::get();
    case 40960: return block_pool_resource<40960, 64>::get();
    case 49152: return block_pool_resource<49152, 64>::get();
    case 57344: return block_pool_resource<57344, 64>::get();
    case 65536: return block_pool_resource<65536, 64>::get();
    default   : break;
    }
    break;
  default:
    break;
  }
  return block_pool_resource<0, 0>::get();
}

void *alloc(std::size_t bytes) noexcept {
  return get_regular_resource(bytes).allocate(bytes);
}

void free(void *p, std::size_t bytes) noexcept {
  return get_regular_resource(bytes).deallocate(p, bytes);
}

} // namespace mem
} // namespace ipc
