#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)

#include <locale>
#include <iostream>

#include "test.h"

#include "libipc/platform/to_tchar.h"

TEST(Platform, to_tchar) {
    char const *utf8 = "hello world, "
                       "\xe6\xb5\xa3\xe7\x8a\xb2\xe3\x82\xbd\xe9\x94\x9b\xe5\xb1\xbb\xe4"
                       "\xba\xbe\xe9\x8a\x88\xe6\x92\xb1\xe4\xbc\x80\xe9\x8a\x87\xc2\xb0"
                       "\xe4\xbc\x85";
    wchar_t const *utf16 = L"hello world, \u4f60\u597d\uff0c\u3053\u3093\u306b\u3061\u306f";
    {
        ipc::string str = ipc::detail::to_tchar<char>(utf8);
        EXPECT_STREQ(str.c_str(), utf8);
    }
    {
        ipc::wstring wtr = ipc::detail::to_tchar<wchar_t>(utf8);
        EXPECT_STREQ(wtr.c_str(), utf16);
    }
}

#endif/*WIN*/