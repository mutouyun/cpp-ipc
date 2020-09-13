/*
    The Capo Library
    Code covered by the MIT License
    Author: mutouyun (http://orzz.org)
*/

#pragma once

////////////////////////////////////////////////////////////////

#ifdef CAPO_UNUSED_
#   error "CAPO_UNUSED_ has been defined."
#endif

#if defined(_MSC_VER)
#   define CAPO_UNUSED_ __pragma(warning(suppress: 4100 4101 4189))
#elif defined(__GNUC__)
#   define CAPO_UNUSED_ __attribute__((__unused__))
#else
#   define CAPO_UNUSED_
#endif

////////////////////////////////////////////////////////////////