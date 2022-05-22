
#include "gtest/gtest.h"

#include "libimp/log.h"

TEST(log, detail) {
  EXPECT_FALSE(imp::detail_log::has_fn_info_v<int>);
  EXPECT_FALSE(imp::detail_log::has_fn_error_v<int>);

  struct foo {
    int info(std::string);
  };
  EXPECT_TRUE (imp::detail_log::has_fn_info_v<foo>);
  EXPECT_FALSE(imp::detail_log::has_fn_error_v<foo>);

  struct bar {
    int info(char const *);
    void error(std::string &&);
  };
  EXPECT_FALSE(imp::detail_log::has_fn_info_v<bar>);
  EXPECT_TRUE (imp::detail_log::has_fn_error_v<bar>);

  struct str {
    str(std::string const &);
  };
  struct foo_bar {
    void info(std::string const &);
    void error(str);
  };
  EXPECT_TRUE(imp::detail_log::has_fn_info_v<foo_bar>);
  EXPECT_TRUE(imp::detail_log::has_fn_error_v<foo_bar>);

  auto vt_int = imp::detail_log::traits<int>::make_vtable();
  EXPECT_NE(vt_int, nullptr);
  vt_int->info (nullptr, "123");
  vt_int->error(nullptr, "321");

  struct log {
    std::string what;
    void error(std::string &&s) {
      what += s;
    }
  } ll;
  auto vt_log = imp::detail_log::traits<log>::make_vtable();
  EXPECT_NE(vt_log, nullptr);
  vt_log->info (&ll, "123");
  vt_log->error(&ll, "321");
  vt_log->info (&ll, "654");
  vt_log->error(&ll, "456");
  EXPECT_EQ(ll.what, "321456");
}

TEST(log, log_printer) {
  struct log {
    std::string i;
    std::string e;
    void info(std::string &&s) {
      i += s;
    }
    void error(std::string &&s) {
      e += s;
    }
  } ll;

  imp::log_printer pt = ll;
  pt.info("hello ");
  pt.error("failed: ");
  pt.info("log-pt");
  pt.error("whatever");
  EXPECT_EQ(ll.i, "hello log-pt");
  EXPECT_EQ(ll.e, "failed: whatever");

  imp::log_printer ps = imp::log_std;
  ps.info("hello world\n");
}

TEST(log, gripper) {
  imp::log::gripper log {imp::log_std, __func__};
  log.info("hello");
}