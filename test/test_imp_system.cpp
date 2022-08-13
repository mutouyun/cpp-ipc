
#include <iostream>

#include "gtest/gtest.h"
#include "fmt/format.h"

#include "libimp/system.h"
#include "libimp/detect_plat.h"
#include "libimp/codecvt.h"

#if defined(LIBIMP_OS_WIN)
#include <Windows.h>
#else
#endif

TEST(system, error_code) {
  std::cout << fmt::format("{}\n", imp::sys::error_code());

  imp::sys::error_code({false, 111});
  auto err = imp::sys::error_code();
  EXPECT_FALSE(err);
  EXPECT_EQ(err.value(), 111);

  imp::sys::error_code({});
  EXPECT_TRUE(imp::sys::error_code());
}

TEST(system, error_msg) {
#if defined(LIBIMP_OS_WIN)
  std::u16string u16_ok, u16_err;
  LANGID lId = ::GetSystemDefaultLangID();
  switch (lId) {
  case 0x0804:
    u16_ok  = u"操作成功完成。\r\n";
    u16_err = u"句柄无效。\r\n";
    break;
  case 0x0409:
    u16_ok  = u"The operation completed successfully.\r\n";
    u16_err = u"The handle is invalid.\r\n";
    break;
  default:
    return;
  }
  {
    std::string s_txt;
    imp::cvt_sstr(u16_ok, s_txt);
    EXPECT_EQ(imp::sys::error_msg({}), s_txt);
  }
  {
    std::string s_txt;
    imp::cvt_sstr(u16_err, s_txt);
    EXPECT_EQ(imp::sys::error_msg({false, ERROR_INVALID_HANDLE}), s_txt);
  }
#else
#endif
}