
#include <thread>

#include "gtest/gtest.h"

#include <libipc/event.h>

#if !defined(LIBIMP_OS_LINUX)
TEST(event, open_close) {
  auto evt = ipc::evt_open("test");
  ASSERT_TRUE(evt);
  ASSERT_EQ(ipc::evt_name(*evt), "test");
  ASSERT_TRUE(ipc::evt_close(*evt));
}

TEST(event, wait_timeout) {
  auto evt = ipc::evt_open("test");
  ASSERT_TRUE(evt);
  ASSERT_FALSE(*ipc::evt_wait(*evt, 0));
  ASSERT_TRUE(ipc::evt_close(*evt));
}

TEST(event, set_wait) {
  auto evt = ipc::evt_open("test");
  ASSERT_TRUE(evt);
  ASSERT_TRUE(ipc::evt_set(*evt));
  ASSERT_TRUE(*ipc::evt_wait(*evt, 0));
  ASSERT_TRUE(ipc::evt_close(*evt));
}

TEST(event, unicast) {
  auto evt = ipc::evt_open("test");
  ASSERT_TRUE(evt);
  int success = 0;
  auto wait = [evt, &success] {
    if (*ipc::evt_wait(*evt, 200)) ++success;
  };
  auto th1 = std::thread(wait);
  auto th2 = std::thread(wait);
  auto th3 = std::thread(wait);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(ipc::evt_set(*evt));
  th1.join();
  th2.join();
  th3.join();
  ASSERT_TRUE(ipc::evt_close(*evt));
  ASSERT_EQ(success, 1);
}

TEST(event, wait_multi_timeout) {
  auto evt1 = ipc::evt_open("test1");
  ASSERT_TRUE(evt1);
  auto evt2 = ipc::evt_open("test2");
  ASSERT_TRUE(evt2);
  auto evt3 = ipc::evt_open("test3");
  ASSERT_TRUE(evt3);
  ASSERT_FALSE(*ipc::evt_wait(imp::make_span({*evt1, *evt2, *evt3}), 0));
  ASSERT_TRUE(ipc::evt_close(*evt1));
  ASSERT_TRUE(ipc::evt_close(*evt2));
  ASSERT_TRUE(ipc::evt_close(*evt3));
}

TEST(event, set_wait_multi) {
  auto evt1 = ipc::evt_open("test1");
  ASSERT_TRUE(evt1);
  auto evt2 = ipc::evt_open("test2");
  ASSERT_TRUE(evt2);
  auto evt3 = ipc::evt_open("test3");
  ASSERT_TRUE(evt3);
  ASSERT_TRUE(ipc::evt_set(*evt2));
  ASSERT_TRUE(*ipc::evt_wait(imp::make_span({*evt1, *evt2, *evt3}), 0));
  ASSERT_TRUE(ipc::evt_close(*evt1));
  ASSERT_TRUE(ipc::evt_close(*evt2));
  ASSERT_TRUE(ipc::evt_close(*evt3));
}

TEST(event, unicast_multi) {
  auto evt1 = ipc::evt_open("test1");
  ASSERT_TRUE(evt1);
  auto evt2 = ipc::evt_open("test2");
  ASSERT_TRUE(evt2);
  auto evt3 = ipc::evt_open("test3");
  ASSERT_TRUE(evt3);
  int success = 0;
  auto wait = [evt1, evt2, evt3, &success] {
    if (*ipc::evt_wait(imp::make_span({*evt1, *evt2, *evt3}), 200)) ++success;
  };
  auto th1 = std::thread(wait);
  auto th2 = std::thread(wait);
  auto th3 = std::thread(wait);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(ipc::evt_set(*evt3));
  th1.join();
  th2.join();
  th3.join();
  ASSERT_TRUE(ipc::evt_close(*evt1));
  ASSERT_TRUE(ipc::evt_close(*evt2));
  ASSERT_TRUE(ipc::evt_close(*evt3));
  ASSERT_EQ(success, 1);
}
#endif
