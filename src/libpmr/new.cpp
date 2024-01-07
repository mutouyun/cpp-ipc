
#include "libpmr/new.h"

LIBPMR_NAMESPACE_BEG_

auto get_thread_block_pool_map() noexcept 
  -> std::unordered_map<std::size_t, block_collector *> & {
  thread_local std::unordered_map<std::size_t, block_collector *> instances;
  return instances;
}

LIBPMR_NAMESPACE_END_
