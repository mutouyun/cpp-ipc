
#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "libimp/system.h"
#include "libimp/detect_plat.h"
#include "libimp/codecvt.h"
#include "libimp/fmt.h"

#if defined(LIBIMP_OS_WIN)
#include <Windows.h>
#else
#endif

TEST(system, error_no) {
  imp::sys::error_no(111);
  auto err = imp::sys::error_no();
  EXPECT_FALSE(err == 0);
  EXPECT_EQ(err, 111);

  imp::sys::error_no({});
  EXPECT_TRUE(imp::sys::error_no() == 0);

  auto e_msg = imp::sys::error_str(imp::sys::error_no());
  std::cout << e_msg << "\n";
}

TEST(system, error_str) {
#if defined(LIBIMP_OS_WIN)
  std::wstring u16_ok, u16_err;
  LANGID lId = ::GetSystemDefaultLangID();
  switch (lId) {
  case 0x0804:
    u16_ok  = L"操作成功完成。\r\n";
    u16_err = L"句柄无效。\r\n";
    break;
  case 0x0409:
    u16_ok  = L"The operation completed successfully.\r\n";
    u16_err = L"The handle is invalid.\r\n";
    break;
  default:
    return;
  }
  {
    std::string s_txt;
    imp::cvt_sstr(u16_ok, s_txt);
    EXPECT_EQ(imp::sys::error_str({}), s_txt);
  }
  {
    std::string s_txt;
    imp::cvt_sstr(u16_err, s_txt);
    EXPECT_EQ(imp::sys::error_str(ERROR_INVALID_HANDLE), s_txt);
  }
#else
  EXPECT_EQ(imp::sys::error_str(1234), "Unknown error 1234");
  EXPECT_EQ(imp::sys::error_str({}), "Success");
  EXPECT_EQ(imp::sys::error_str(EINVAL), "Invalid argument");
#endif
}

TEST(system, conf) {
  auto ret = imp::sys::conf(imp::sys::info::page_size);
  EXPECT_TRUE(ret);
  EXPECT_EQ(ret.value(), 4096);
}