#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)

#include <locale>
#include <iostream>

#include "test.h"

#include "libipc/platform/to_tchar.h"

TEST(Platform, to_tchar) {
    unsigned char const utf8[] = {
        0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x2c, 0x20, 0xe6, 0xb5, 0xa3,
        0xe7, 0x8a, 0xb2, 0xe3, 0x82, 0xbd, 0xe9, 0x94, 0x9b, 0xe5, 0xb1, 0xbb, 0xe4, 0xba, 0xbe, 0xe9,
        0x8a, 0x88, 0xe6, 0x92, 0xb1, 0xe4, 0xbc, 0x80, 0xe9, 0x8a, 0x87, 0xc2, 0xb0, 0xe4, 0xbc, 0x85,
        0x00,
    };
    char    const *sstr = reinterpret_cast<char    const *>(utf8);
    wchar_t const *wstr = reinterpret_cast<wchar_t const *>(u"hello world, 你好，こんにちは");
    {
        ipc::string str = ipc::detail::to_tchar<char>(sstr);
        EXPECT_STREQ(str.c_str(), sstr);
    }
    {
        ipc::wstring wtr = ipc::detail::to_tchar<wchar_t>(sstr);
        EXPECT_STREQ(wtr.c_str(), wstr);
    }
}

#endif/*WIN*/