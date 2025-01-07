
#include <string>
#include <cstring>

#include "test.h"

#include "libipc/imp/codecvt.h"

TEST(codecvt, cvt_cstr) {
  char const utf8[] = "hello world, 你好，こんにちは";
  wchar_t const utf16[] = L"hello world, 你好，こんにちは";
  {
    auto cvt_len = ipc::cvt_cstr(utf8, std::strlen(utf8), (wchar_t *)nullptr, 0);
    EXPECT_NE(cvt_len, 0);
    std::wstring wstr(cvt_len, L'\0');
    EXPECT_EQ(ipc::cvt_cstr(utf8, std::strlen(utf8), &wstr[0], wstr.size()), cvt_len);
    EXPECT_EQ(wstr, utf16);
  }
  {
    auto cvt_len = ipc::cvt_cstr(utf16, std::wcslen(utf16), (char *)nullptr, 0);
    EXPECT_NE(cvt_len, 0);
    std::string str(cvt_len, '\0');
    EXPECT_EQ(ipc::cvt_cstr(utf16, std::wcslen(utf16), &str[0], str.size()), cvt_len);
    EXPECT_EQ(str, utf8);
  }
  {
    auto cvt_len = ipc::cvt_cstr(utf8, std::strlen(utf8), (char *)nullptr, 0);
    EXPECT_EQ(cvt_len, std::strlen(utf8));
    std::string str(cvt_len, '\0');
    EXPECT_EQ(ipc::cvt_cstr(utf8, cvt_len, &str[0], str.size()), cvt_len);
    EXPECT_EQ(str, utf8);
  }
  {
    auto cvt_len = ipc::cvt_cstr(utf16, std::wcslen(utf16), (wchar_t *)nullptr, 0);
    EXPECT_EQ(cvt_len, std::wcslen(utf16));
    std::wstring wstr(cvt_len, u'\0');
    EXPECT_EQ(ipc::cvt_cstr(utf16, cvt_len, &wstr[0], wstr.size()), cvt_len);
    EXPECT_EQ(wstr, utf16);
  }
}
