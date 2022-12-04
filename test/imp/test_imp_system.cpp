﻿
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

TEST(system, error_code) {
  imp::sys::error_code({false, 111});
  auto err = imp::sys::error_code();
  EXPECT_FALSE(err);
  EXPECT_EQ(err.value(), 111);

  imp::sys::error_code({});
  EXPECT_TRUE(imp::sys::error_code());

  imp::sys::error e_obj {err};
  EXPECT_EQ(err.value(), e_obj.value());
  auto e_msg = imp::fmt(imp::sys::error_msg(imp::sys::error_code()));
  EXPECT_EQ(e_msg, imp::fmt(imp::sys::error()));
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
    EXPECT_EQ(imp::sys::error_str({false, ERROR_INVALID_HANDLE}), s_txt);
  }
#else
  EXPECT_EQ(imp::sys::error_str({false, 1234}), "Unknown error 1234");
  EXPECT_EQ(imp::sys::error_str({}), "Success");
  EXPECT_EQ(imp::sys::error_str({false, EINVAL}), "Invalid argument");
#endif
}

TEST(system, conf) {
  auto ret = imp::sys::conf(imp::sys::info::page_size);
  EXPECT_TRUE(ret);
  EXPECT_EQ(ret.value(), 4096);
}