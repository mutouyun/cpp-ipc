
#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "libimp/log.h"

TEST(log, detail) {
  EXPECT_EQ(imp::log::detail::has_fn_output_v<int>, imp::log::detail::out_none);

  struct foo {
    int info(std::string);
  };
  EXPECT_EQ(imp::log::detail::has_fn_output_v<foo>, imp::log::detail::out_none);

  struct bar {
    int info(char const *);
    void output(imp::log::level, std::string &&);
  };
  EXPECT_EQ(imp::log::detail::has_fn_output_v<bar>, imp::log::detail::out_string);

  struct str {
    str(std::string const &);
  };
  struct foo_bar {
    void output(imp::log::level, str);
  };
  EXPECT_EQ(imp::log::detail::has_fn_output_v<foo_bar>, imp::log::detail::out_string);

  auto vt_int = imp::log::detail::traits<int>::make_vtable();
  EXPECT_NE(vt_int, nullptr);
  EXPECT_NO_THROW(vt_int->output(nullptr, imp::log::level::error, ""));

  struct log {
    std::string what;
    void output(imp::log::level l, std::string &&s) {
      if (l == imp::log::level::error) what += s + '\n';
    }
  } ll;
  auto vt_log = imp::log::detail::traits<log>::make_vtable();
  EXPECT_NE(vt_log, nullptr);

  vt_log->output(&ll, imp::log::level::info, "123");
  vt_log->output(&ll, imp::log::level::info, "321");
  vt_log->output(&ll, imp::log::level::info, "654");
  vt_log->output(&ll, imp::log::level::info, "456");
  std::cout << ll.what << "\n";
  SUCCEED();
}

TEST(log, log_printer) {
  struct log {
    std::string i;
    std::string e;

    void output(imp::log::level l, std::string &&s) {
      if (l == imp::log::level::error) e += s + "\n";
      else if (l == imp::log::level::info) i += s + "\n";
    }
  } ll;

  imp::log::printer pt = ll;
  pt.output(imp::log::context<char const *>{imp::log::level::info , std::chrono::system_clock::now(), __func__, "hello "});
  pt.output(imp::log::context<char const *>{imp::log::level::error, std::chrono::system_clock::now(), __func__, "failed: "});
  pt.output(imp::log::context<char const *>{imp::log::level::info , std::chrono::system_clock::now(), __func__, "log-pt"});
  pt.output(imp::log::context<char const *>{imp::log::level::error, std::chrono::system_clock::now(), __func__, "whatever"});
  std::cout << ll.i << "\n";
  std::cout << ll.e << "\n";

  imp::log::printer ps = imp::log::std_out;
  ps.output(imp::log::context<char const *>{imp::log::level::info, std::chrono::system_clock::now(), __func__, "hello world"});
  SUCCEED();
}

TEST(log, gripper) {
  {
    imp::log::grip log {__func__};
    log.info("hello");
  }
  {
    LIBIMP_LOG_();
    log.info("hello 2");
  }
  SUCCEED();
}
