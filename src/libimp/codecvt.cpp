#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
#include "libimp/platform/win/codecvt.h"
#else
#include "libimp/platform/posix/codecvt.h"
#endif

#include <algorithm>
#include <cstring>

LIBIMP_NAMESPACE_BEG_

namespace {

template <typename CharT>
std::size_t cpy_cstr(CharT *des, std::size_t dlen, CharT const *src, std::size_t slen) noexcept {
  if ((des == nullptr) || (dlen == 0)) {
    return slen;
  }
  std::size_t r = (std::min)(slen, dlen);
  std::memcpy(des, src, r * sizeof(CharT));
  return r;
}

} // namespace

template <>
std::size_t cvt_cstr(char const *src, std::size_t slen, char *des, std::size_t dlen) noexcept {
  return cpy_cstr(des, dlen, src, slen);
}

template <>
std::size_t cvt_cstr(wchar_t const *src, std::size_t slen, wchar_t *des, std::size_t dlen) noexcept {
  return cpy_cstr(des, dlen, src, slen);
}

LIBIMP_NAMESPACE_END_
