
#include <string>
#include <cstring>

#include "gtest/gtest.h"

#include "libimp/codecvt.h"

TEST(codecvt, cvt_cstr) {
  char const *utf8 = "hello world, "
                     "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc"
                     "\x8c\xe3\x81\x93\xe3\x82\x93\xe3"
                     "\x81\xab\xe3\x81\xa1\xe3\x81\xaf";
  wchar_t const *utf16 = L"hello world, \u4f60\u597d\uff0c\u3053\u3093\u306b\u3061\u306f";
  {
    auto cvt_len = imp::cvt_cstr(utf8, std::strlen(utf8), (wchar_t *)nullptr, 0);
    EXPECT_NE(cvt_len, 0);
    std::wstring wstr(cvt_len, L'\0');
    EXPECT_EQ(imp::cvt_cstr(utf8, std::strlen(utf8), &wstr[0], wstr.size()), cvt_len);
    EXPECT_STREQ(wstr.c_str(), utf16);
  }
  {
    auto cvt_len = imp::cvt_cstr(utf16, std::wcslen(utf16), (char *)nullptr, 0);
    EXPECT_NE(cvt_len, 0);
    std::string str(cvt_len, '\0');
    EXPECT_EQ(imp::cvt_cstr(utf16, std::wcslen(utf16), &str[0], str.size()), cvt_len);
    EXPECT_STREQ(str.c_str(), utf8);
  }
  {
    auto cvt_len = imp::cvt_cstr(utf8, std::strlen(utf8), (char *)nullptr, 0);
    EXPECT_EQ(cvt_len, std::strlen(utf8));
    std::string str(cvt_len, '\0');
    EXPECT_EQ(imp::cvt_cstr(utf8, cvt_len, &str[0], str.size()), cvt_len);
    EXPECT_STREQ(str.c_str(), utf8);
  }
  {
    auto cvt_len = imp::cvt_cstr(utf16, std::wcslen(utf16), (wchar_t *)nullptr, 0);
    EXPECT_EQ(cvt_len, std::wcslen(utf16));
    std::wstring wstr(cvt_len, L'\0');
    EXPECT_EQ(imp::cvt_cstr(utf16, cvt_len, &wstr[0], wstr.size()), cvt_len);
    EXPECT_STREQ(wstr.c_str(), utf16);
  }
}