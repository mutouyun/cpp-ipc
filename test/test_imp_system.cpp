
#include "gtest/gtest.h"

#include "libimp/system.h"
#include "libimp/detect_plat.h"
#include "libimp/codecvt.h"

#if defined(LIBIMP_OS_WIN)
#include <WinError.h>
#else
#endif

TEST(system, error_code) {
  EXPECT_TRUE(imp::sys::error_code());

  imp::sys::error_code({false, 111});
  auto err = imp::sys::error_code();
  EXPECT_FALSE(err);
  EXPECT_EQ(err.value(), 111);

  imp::sys::error_code({});
  EXPECT_TRUE(imp::sys::error_code());
}

TEST(system, error_msg) {
#if defined(LIBIMP_OS_WIN)
  {
    std::u16string u16_txt = u"操作成功完成。\r\n";
    std::string s_txt;
    imp::cvt_sstr(u16_txt, s_txt);
    EXPECT_EQ(imp::sys::error_msg({}), s_txt);
  }
  {
    std::u16string u16_txt = u"句柄无效。\r\n";
    std::string s_txt;
    imp::cvt_sstr(u16_txt, s_txt);
    EXPECT_EQ(imp::sys::error_msg({false, ERROR_INVALID_HANDLE}), s_txt);
  }
#else
#endif
}