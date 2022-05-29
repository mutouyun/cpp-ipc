
#include "gtest/gtest.h"

#include "libimp/log.h"

TEST(log, detail) {
  EXPECT_FALSE(imp::detail_log::has_fn_output_v<int>);

  struct foo {
    int info(std::string);
  };
  EXPECT_FALSE(imp::detail_log::has_fn_output_v<foo>);

  struct bar {
    int info(char const *);
    void output(imp::log::level, std::string &&);
  };
  EXPECT_TRUE(imp::detail_log::has_fn_output_v<bar>);

  struct str {
    str(std::string const &);
  };
  struct foo_bar {
    void output(imp::log::level, str);
  };
  EXPECT_TRUE(imp::detail_log::has_fn_output_v<foo_bar>);

  auto vt_int = imp::detail_log::traits<int>::make_vtable();
  EXPECT_NE(vt_int, nullptr);
  vt_int->output(nullptr, imp::log::level::debug, "123");

  struct log {
    std::string what;
    void output(imp::log::level l, std::string &&s) {
      if (l == imp::log::level::error) what += s;
    }
  } ll;
  auto vt_log = imp::detail_log::traits<log>::make_vtable();
  EXPECT_NE(vt_log, nullptr);
  vt_log->output(&ll, imp::log::level::info , "123");
  vt_log->output(&ll, imp::log::level::error, "321");
  vt_log->output(&ll, imp::log::level::info , "654");
  vt_log->output(&ll, imp::log::level::error, "456");
  EXPECT_EQ(ll.what, "321456");
}

TEST(log, log_printer) {
  struct log {
    std::string i;
    std::string e;

    void output(imp::log::level l, std::string &&s) {
      if (l == imp::log::level::error) e += s;
      else if (l == imp::log::level::info) i += s;
    }
  } ll;

  imp::log::printer pt = ll;
  pt.output(imp::log::level::info , "hello ");
  pt.output(imp::log::level::error, "failed: ");
  pt.output(imp::log::level::info , "log-pt");
  pt.output(imp::log::level::error, "whatever");
  EXPECT_EQ(ll.i, "hello log-pt");
  EXPECT_EQ(ll.e, "failed: whatever");

  imp::log::printer ps = imp::log::std_out;
  ps.output(imp::log::level::info, "hello world\n");
}

TEST(log, gripper) {
  imp::log::gripper log {imp::log::std_out, __func__};
  log.info("hello");
}