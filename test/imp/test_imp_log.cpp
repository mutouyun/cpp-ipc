
#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "libimp/log.h"

TEST(log, gripper) {
  {
    LIBIMP_LOG_();
    log.info("hello");
  }
  {
    LIBIMP_LOG_();
    log.info("hello 2");
  }
  SUCCEED();
}

TEST(log, custom) {
  struct log {
    std::string i;
    std::string e;
  } ll_data;
  auto ll = [&ll_data](auto &&ctx) mutable {
    auto s = imp::fmt(ctx.params);
    if (ctx.level == imp::log::level::error) ll_data.e += s + " ";
    else
    if (ctx.level == imp::log::level::info ) ll_data.i += s + " ";
  };

  LIBIMP_LOG_(ll);

  log.info ("hello");
  log.error("failed:");
  log.info ("log-pt");
  log.error("whatever");

  EXPECT_EQ(ll_data.i, "hello log-pt ");
  EXPECT_EQ(ll_data.e, "failed: whatever ");
}
