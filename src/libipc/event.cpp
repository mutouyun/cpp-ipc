
#include "libimp/log.h"

#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
# include "libipc/platform/win/event_impl.h"
#else
#endif

LIBIPC_NAMESPACE_BEG_

/// \brief C style event access interface implementation.

std::string evt_name(evt_t evt) noexcept {
  LIBIMP_LOG_();
  if (!is_valid(evt)) {
    log.error("handle is null.");
    return {};
  }
  return evt->name;
}

/// \brief The event object.

LIBIPC_NAMESPACE_END_
