#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)

#include <locale>
#include <iostream>

#include "test.h"

#include "libipc/platform/to_tchar.h"

TEST(Platform, to_tchar) {
    char const *utf8 = "hello world, "
                       "\xE6\xB5\xA3\xE7\x8A\xB2\xE3\x82\xBD\xE9\x94\x9B\xE5\xB1\xBB\xE4"
                       "\xBA\xBE\xE9\x8A\x88\xE6\x92\xB1\xE4\xBC\x80\xE9\x8A\x87\xC2\xB0"
                       "\xE4\xBC\x85";
    wchar_t const *utf16 = L"hello world, \x6D63\x72B2\x30BD\x951B\x5C7B\x4EBE\x9288\x64B1\x4F00\x9287\xB0\x4F05";
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