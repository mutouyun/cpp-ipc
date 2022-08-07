/**
 * @file libimp/codecvt.h
 * @author mutouyun (orz@orzz.org)
 * @brief Character set conversion interface
 * @date 2022-08-07
 */
#pragma once

#include <string>
#include <cstddef>

#include "libimp/def.h"
#include "libimp/export.h"

LIBIMP_NAMESPACE_BEG_

/**
 * @brief The transform between local-character-set(UTF-8/GBK/...) and UTF-16/32
 * 
 * @param des  The target string pointer can be nullptr
 * @param dlen The target string length can be 0
 */
template <typename CharT, typename CharU>
LIBIMP_EXPORT std::size_t cvt_cstr(CharT const *src, std::size_t slen, CharU *des, std::size_t dlen) noexcept;

LIBIMP_NAMESPACE_END_
