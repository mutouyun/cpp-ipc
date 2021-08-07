
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)

#include <locale>
#include <iostream>

#include "test.h"

#include "libipc/platform/to_tchar.h"

TEST(Platform, to_tchar) {
    char    const *utf8 =  "hello world, 你好，こんにちは";
    wchar_t const *wstr = L"hello world, 你好，こんにちは";
    {
        ipc::string str = ipc::detail::to_tchar<char>(utf8);
        EXPECT_STREQ(str.c_str(), utf8);
    }
    {
        ipc::wstring wtr = ipc::detail::to_tchar<wchar_t>(utf8);
        //std::wcout.imbue(std::locale());
        //std::wcout << wtr << "\n";
        EXPECT_STREQ(wtr.c_str(), wstr);
    }
}

#endif/*WIN*/
