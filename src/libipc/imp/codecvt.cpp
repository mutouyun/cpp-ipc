
#include <algorithm>
#include <type_traits>
#include <cstring>
#include <cstdint>

#include "libipc/imp/codecvt.h"
#include "libipc/imp/detect_plat.h"

#if defined(LIBIPC_OS_WIN)
# include "libipc/platform/win/codecvt.h"
#endif

namespace ipc {

/**
 * \brief The transform between local-character-set(UTF-8/GBK/...) and UTF-16/32.
 * 
 * Modified from UnicodeConverter.
 * Copyright (c) 2010. Jianhui Qin (http://blog.csdn.net/jhqin).
 * 
 * \remarks codecvt_utf8_utf16/std::wstring_convert is deprecated.
 * \see https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/
 *      https://stackoverflow.com/questions/42946335/deprecated-header-codecvt-replacement
 *      https://en.cppreference.com/w/cpp/locale/codecvt/in
*/
namespace {

/// \brief X-bit unicode transformation format
enum class ufmt {
  utf8,
  utf16,
  utf32,
};

template <typename T, ufmt, typename = void>
struct utf_compatible : std::false_type {};

template <typename T>
struct utf_compatible<T, ufmt::utf8, 
  std::enable_if_t<std::is_fundamental<T>::value && (sizeof(T) == 1)>> : std::true_type {};

template <typename T>
struct utf_compatible<T, ufmt::utf16, 
  std::enable_if_t<std::is_fundamental<T>::value && (sizeof(T) == 2)>> : std::true_type {};

template <typename T>
struct utf_compatible<T, ufmt::utf32, 
  std::enable_if_t<std::is_fundamental<T>::value && (sizeof(T) == 4)>> : std::true_type {};

template <typename T, ufmt Fmt>
constexpr bool utf_compatible_v = utf_compatible<T, Fmt>::value;

/**
 * \brief UTF-32 --> UTF-8
 */
template <typename T, typename U>
auto cvt_char(T src, U* des, std::size_t dlen) noexcept
  -> std::enable_if_t<utf_compatible_v<T, ufmt::utf32> &&
                      utf_compatible_v<U, ufmt::utf8>, std::size_t> {
  if (src == 0) return 0;

  constexpr std::uint8_t prefix[] = {
    0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
  };
  constexpr std::uint32_t codeup[] = {
    0x80,      // U+00000000 - U+0000007F
    0x800,     // U+00000080 - U+000007FF
    0x10000,   // U+00000800 - U+0000FFFF
    0x200000,  // U+00010000 - U+001FFFFF
    0x4000000, // U+00200000 - U+03FFFFFF
    0x80000000 // U+04000000 - U+7FFFFFFF
  };

  std::size_t i, len = sizeof(codeup) / sizeof(std::uint32_t);
  for(i = 0; i < len; ++i) {
    if (static_cast<std::uint32_t>(src) < codeup[i]) break;
  }
  if (i == len) return 0; // the src is invalid

  len = i + 1;
  if (des != nullptr) {
    if (dlen > i) for (; i > 0; --i) {
      des[i] = static_cast<U>((src & 0x3F) | 0x80);
      src >>= 6;
    }
    des[0] = static_cast<U>(src | prefix[len - 1]);
  }
  return len;
}

/**
 * \brief UTF-8 --> UTF-32
 */
template <typename T, typename U>
auto cvt_char(T const *src, std::size_t slen, U &des) noexcept
  -> std::enable_if_t<utf_compatible_v<T, ufmt::utf8> &&
                      utf_compatible_v<U, ufmt::utf32>, std::size_t> {
  if ((src == nullptr) || (*src) == 0) return 0;
  if (slen == 0) return 0;

  std::uint8_t b = (std::uint8_t)*(src++);

  if (b < 0x80) {
    des = b;
    return 1;
  }

  if (b < 0xC0 || b > 0xFD) return 0; // the src is invalid

  std::size_t len;
  if (b < 0xE0) {
    des = b & 0x1F;
    len = 2;
  } else if (b < 0xF0) {
    des = b & 0x0F;
    len = 3;
  } else if (b < 0xF8) {
    des = b & 0x07;
    len = 4;
  } else if (b < 0xFC) {
    des = b & 0x03;
    len = 5;
  } else {
    des = b & 0x01;
    len = 6;
  }

  if (slen < len) return 0;
  std::size_t i = 1;
  for(; i < len; ++i) {
    b = *(src++);
    if ((b < 0x80) || (b > 0xBF)) return 0; // the src is invalid
    des = (des << 6) + (b & 0x3F);
  }
  return len;
}

/**
 * \brief UTF-32 --> UTF-16
 */
template <typename T, typename U>
auto cvt_char(T src, U *des, std::size_t dlen) noexcept
  -> std::enable_if_t<utf_compatible_v<T, ufmt::utf32> &&
                      utf_compatible_v<U, ufmt::utf16>, std::size_t> {
  if (src == 0) return 0;

  if (src <= 0xFFFF) {
    if ((des != nullptr) && (dlen != 0)) {
      (*des) = static_cast<U>(src);
    }
    return 1;
  } else if (src <= 0xEFFFF) {
    if ((des != nullptr) && (dlen > 1)) {
      des[0] = static_cast<U>(0xD800 + (src >> 10) - 0x40);  // high
      des[1] = static_cast<U>(0xDC00 + (src & 0x03FF));      // low
    }
    return 2;
  }
  return 0;
}

/**
 * \brief UTF-16 --> UTF-32
*/
template <typename T, typename U>
auto cvt_char(T const *src, std::size_t slen, U &des)
  -> std::enable_if_t<utf_compatible_v<T, ufmt::utf16> &&
                      utf_compatible_v<U, ufmt::utf32>, std::size_t> {
  if ((src == nullptr) || (*src) == 0) return 0;
  if (slen == 0) return 0;

  std::uint16_t w1 = src[0];
  if ((w1 >= 0xD800) && (w1 <= 0xDFFF)) {
    if (w1 < 0xDC00) {
      if (slen < 2) return 0;
      std::uint16_t w2 = src[1];
      if ((w2 >= 0xDC00) && (w2 <= 0xDFFF)) {
        des = (w2 & 0x03FF) + (((w1 & 0x03FF) + 0x40) << 10);
        return 2;
      }
    }
    return 0; // the src is invalid
  }
  des = w1;
  return 1;
}

/**
 * \brief UTF-16 --> UTF-8
*/
template <typename T, typename U>
auto cvt_char(T src, U *des, std::size_t dlen) noexcept
  -> std::enable_if_t<utf_compatible_v<T, ufmt::utf16> &&
                      utf_compatible_v<U, ufmt::utf8>, std::size_t> {
  // make utf-16 to utf-32
  std::uint32_t tmp;
  if (cvt_char(&src, 1, tmp) != 1) return 0;
  // make utf-32 to utf-8
  return cvt_char(tmp, des, dlen);
}

/**
 * \brief UTF-8 --> UTF-16
*/
template <typename T, typename U>
auto cvt_char(T const *src, std::size_t slen, U &des)
  -> std::enable_if_t<utf_compatible_v<T, ufmt::utf8> &&
                      utf_compatible_v<U, ufmt::utf16>, std::size_t> {
  // make utf-8 to utf-32
  std::uint32_t tmp;
  std::size_t len = cvt_char(src, slen, tmp);
  if (len == 0) return 0;
  // make utf-32 to utf-16
  if (cvt_char(tmp, &des, 1) != 1) return 0;
  return len;
}

/**
 * \brief UTF-32 string --> UTF-8/16 string
*/
template <typename T, typename U>
auto cvt_cstr_utf(T const *src, std::size_t slen, U *des, std::size_t dlen) noexcept
  -> std::enable_if_t<utf_compatible_v<T, ufmt::utf32> &&
                     (utf_compatible_v<U, ufmt::utf16> || utf_compatible_v<U, ufmt::utf8>), std::size_t> {
  if ((src == nullptr) || ((*src) == 0) || (slen == 0)) {
    // source string is empty
    return 0;
  }
  std::size_t num = 0, len = 0;
  for (std::size_t i = 0; (i < slen) && ((*src) != 0); ++src, ++i) {
    len = cvt_char(*src, des, dlen);
    if (len == 0) return 0;
    if (des != nullptr) {
      des += len;
      if (dlen < len) {
        dlen = 0;
      } else {
        dlen -= len;
      }
    }
    num += len;
  }
  return num;
}

/**
 * \brief UTF-8/16 string --> UTF-32 string
*/
template <typename T, typename U>
auto cvt_cstr_utf(T const *src, std::size_t slen, U *des, std::size_t dlen) noexcept
  -> std::enable_if_t<utf_compatible_v<U, ufmt::utf32> &&
                     (utf_compatible_v<T, ufmt::utf16> || utf_compatible_v<T, ufmt::utf8>), std::size_t> {
  if ((src == nullptr) || ((*src) == 0) || (slen == 0)) {
    // source string is empty
    return 0;
  }
  std::size_t num = 0;
  for (std::size_t i = 0; (i < slen) && ((*src) != 0);) {
    std::uint32_t tmp;
    std::size_t len = cvt_char(src, slen - i, tmp);
    if (len == 0) return 0;
    if ((des != nullptr) && (dlen > 0)) {
      (*des) = tmp;
      ++des;
      dlen -= 1;
    }
    src += len;
    i   += len;
    num += 1;
  }
  return num;
}

/**
 * \brief UTF-8/16 string --> UTF-16/8 string
*/
template <typename T, typename U>
auto cvt_cstr_utf(T const *src, std::size_t slen, U *des, std::size_t dlen) noexcept
  -> std::enable_if_t<(utf_compatible_v<T, ufmt::utf8>  && utf_compatible_v<U, ufmt::utf16>) ||
                      (utf_compatible_v<T, ufmt::utf16> && utf_compatible_v<U, ufmt::utf8>), std::size_t> {
  if ((src == nullptr) || ((*src) == 0) || (slen == 0)) {
    // source string is empty
    return 0;
  }
  std::size_t num = 0;
  for (std::size_t i = 0; (i < slen) && ((*src) != 0);) {
    // make utf-x to utf-32
    std::uint32_t tmp;
    std::size_t len = cvt_char(src, slen - i, tmp);
    if (len == 0) return 0;
    src += len;
    i   += len;
    // make utf-32 to utf-y
    len = cvt_char(tmp, des, dlen);
    if (len == 0) return 0;
    if (des != nullptr) {
      des += len;
      if (dlen < len) {
        dlen = 0;
      } else {
        dlen -= len;
      }
    }
    num += len;
  }
  return num;
}

template <typename T, typename U>
auto cvt_cstr_utf(T const *src, std::size_t slen, U *des, std::size_t dlen) noexcept
  -> std::enable_if_t<(sizeof(T) == sizeof(U)), std::size_t> {
  if ((des == nullptr) || (dlen == 0)) {
    return slen;
  }
  std::size_t r = (std::min)(slen, dlen);
  std::memcpy(des, src, r * sizeof(T));
  return r;
}

} // namespace

#define LIBIPC_DEF_CVT_CSTR_($CHAR_T, $CHAR_U) \
  template <> \
  std::size_t cvt_cstr($CHAR_T const *src, std::size_t slen, $CHAR_U *des, std::size_t dlen) noexcept { \
    return cvt_cstr_utf(src, slen, des, dlen); \
  }
// #define LIBIPC_DEF_CVT_CSTR_($CHAR_T, $CHAR_U)

LIBIPC_DEF_CVT_CSTR_(char    , char)
LIBIPC_DEF_CVT_CSTR_(char    , char16_t)
LIBIPC_DEF_CVT_CSTR_(char    , char32_t)
LIBIPC_DEF_CVT_CSTR_(wchar_t , wchar_t)
LIBIPC_DEF_CVT_CSTR_(char16_t, char16_t)
LIBIPC_DEF_CVT_CSTR_(char16_t, char)
LIBIPC_DEF_CVT_CSTR_(char16_t, char32_t)
LIBIPC_DEF_CVT_CSTR_(char32_t, char32_t)
LIBIPC_DEF_CVT_CSTR_(char32_t, char)
LIBIPC_DEF_CVT_CSTR_(char32_t, char16_t)
#if !defined(LIBIPC_OS_WIN)
LIBIPC_DEF_CVT_CSTR_(char    , wchar_t)
LIBIPC_DEF_CVT_CSTR_(wchar_t , char)
LIBIPC_DEF_CVT_CSTR_(wchar_t , char16_t)
LIBIPC_DEF_CVT_CSTR_(wchar_t , char32_t)
LIBIPC_DEF_CVT_CSTR_(char16_t, wchar_t)
LIBIPC_DEF_CVT_CSTR_(char32_t, wchar_t)
#endif // !defined(LIBIPC_OS_WIN)

#if defined(LIBIPC_CPP_20)
LIBIPC_DEF_CVT_CSTR_(char8_t , char8_t)
LIBIPC_DEF_CVT_CSTR_(char8_t , char)
LIBIPC_DEF_CVT_CSTR_(char8_t , char16_t)
LIBIPC_DEF_CVT_CSTR_(char8_t , char32_t)
LIBIPC_DEF_CVT_CSTR_(char    , char8_t)
LIBIPC_DEF_CVT_CSTR_(char16_t, char8_t)
LIBIPC_DEF_CVT_CSTR_(char32_t, char8_t)
#if !defined(LIBIPC_OS_WIN)
LIBIPC_DEF_CVT_CSTR_(char8_t , wchar_t)
LIBIPC_DEF_CVT_CSTR_(wchar_t , char8_t)
#endif // !defined(LIBIPC_OS_WIN)
#endif // defined(LIBIPC_CPP_20)

#undef LIBIPC_DEF_CVT_CSTR_

} // namespace ipc
