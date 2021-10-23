#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)

#include <locale>
#include <iostream>
//#include <fstream>

#include "test.h"

#include "libipc/platform/win/to_tchar.h"

TEST(Platform, to_tchar) {
    char const *utf8 = "hello world, "
                       "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc"
                       "\x8c\xe3\x81\x93\xe3\x82\x93\xe3"
                       "\x81\xab\xe3\x81\xa1\xe3\x81\xaf";
    wchar_t const *utf16 = L"hello world, \u4f60\u597d\uff0c\u3053\u3093\u306b\u3061\u306f";
    {
        ipc::string str = ipc::detail::to_tchar<char>(utf8);
        EXPECT_STREQ(str.c_str(), utf8);
    }
    {
        ipc::wstring wtr = ipc::detail::to_tchar<wchar_t>(utf8);
        EXPECT_STREQ(wtr.c_str(), utf16);
        //std::ofstream out("out.txt", std::ios::binary|std::ios::out);
        //out.write((char const *)wtr.c_str(), wtr.size() * sizeof(wchar_t));
    }
}

#endif/*WIN*/