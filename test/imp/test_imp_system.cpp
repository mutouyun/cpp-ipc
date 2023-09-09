
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

TEST(system, conf) {
  auto ret = imp::sys::conf(imp::sys::info::page_size);
  EXPECT_TRUE(ret);
  EXPECT_EQ(ret.value(), 4096);
}
