
#include "gtest/gtest.h"

#include "libipc/shm.h"

TEST(shm, open_close) {
  EXPECT_FALSE(ipc::shm_open("hello-ipc-shm", 1024, ipc::mode::none));

  auto shm1 = ipc::shm_open("hello-ipc-shm", 1024, ipc::mode::create | ipc::mode::open);
  EXPECT_TRUE(shm1);
  EXPECT_FALSE(ipc::shm_open("hello-ipc-shm", 1024, ipc::mode::create));

  auto pt1 = ipc::shm_get(*shm1);
  EXPECT_TRUE(ipc::shm_size(*shm1) >= 1024);
  EXPECT_NE(pt1, nullptr);
  *(int *)pt1 = 0;

  auto shm2 = ipc::shm_open(ipc::shm_name(*shm1), 0, ipc::mode::create | ipc::mode::open);
  EXPECT_TRUE(shm2);
  auto shm3 = ipc::shm_open(ipc::shm_name(*shm1), 128, ipc::mode::open);
  EXPECT_TRUE(shm3);
  auto shm4 = ipc::shm_open(ipc::shm_name(*shm1), 256, ipc::mode::create | ipc::mode::open);
  EXPECT_TRUE(shm4);
  EXPECT_EQ(ipc::shm_size(*shm1), ipc::shm_size(*shm2));
  EXPECT_EQ(ipc::shm_size(*shm1), ipc::shm_size(*shm3));
  EXPECT_EQ(ipc::shm_size(*shm1), ipc::shm_size(*shm4));
  auto pt2 = ipc::shm_get(*shm1);
  EXPECT_NE(pt2, nullptr);
  EXPECT_EQ(*(int *)pt2, 0);
  *(int *)pt1 = 1234;
  EXPECT_EQ(*(int *)pt2, 1234);

  EXPECT_TRUE(ipc::shm_close(*shm4));
  EXPECT_TRUE(ipc::shm_close(*shm3));
  EXPECT_TRUE(ipc::shm_close(*shm2));
  EXPECT_TRUE(ipc::shm_close(*shm1));
  EXPECT_FALSE(ipc::shm_close(nullptr));
}

TEST(shm, shared_memory) {
  ipc::shared_memory shm;
  EXPECT_FALSE(shm.valid());
  EXPECT_EQ(shm.size(), 0);
  EXPECT_EQ(shm.name(), "");
  EXPECT_EQ(shm.get()    , nullptr);
  EXPECT_EQ(*shm         , nullptr);
  EXPECT_EQ(shm.as<int>(), nullptr);
  shm.close();

  EXPECT_TRUE(shm.open("hello-ipc-shared-memory", 2048, ipc::mode::create | ipc::mode::open));
  EXPECT_TRUE(shm.valid());
  EXPECT_TRUE(shm.size() >= 2048);
  EXPECT_EQ(shm.name(), "hello-ipc-shared-memory");
  EXPECT_NE(shm.get()    , nullptr);
  EXPECT_NE(*shm         , nullptr);
  EXPECT_NE(shm.as<int>(), nullptr);
  *shm.as<int>() = 4321;

  auto shm_r = ipc::shm_open(shm.name());
  ASSERT_TRUE(shm_r);
  EXPECT_EQ(*static_cast<int *>(ipc::shm_get(*shm_r)), 4321);

  shm = ipc::shared_memory("hello-ipc-shared-memory-2", 512);
  EXPECT_TRUE(shm.valid());
  EXPECT_TRUE(shm.size() >= 512);
  EXPECT_EQ(shm.name(), "hello-ipc-shared-memory-2");
  EXPECT_NE(shm.get()    , nullptr);
  EXPECT_NE(*shm         , nullptr);
  EXPECT_NE(shm.as<int>(), nullptr);

  *static_cast<int *>(ipc::shm_get(*shm_r)) = 1234;
  *shm.as<int>() = 4444;
  EXPECT_EQ(*static_cast<int *>(ipc::shm_get(*shm_r)), 1234);
  EXPECT_EQ(*shm.as<int>(), 4444);

  EXPECT_TRUE(ipc::shm_close(*shm_r));
}

#include <libimp/detect_plat.h>
#if /*defined(LIBIMP_OS_LINUX)*/ 0
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "test_util.h"

TEST(shm, pipe) {
  auto writer = test::subproc([] {
    mkfifo("/tmp/shm-pipe.w", S_IFIFO|0666);
    // mkfifo("/tmp/shm-pipe.r", S_IFIFO|0666);
    int wfd = open("/tmp/shm-pipe.w", O_WRONLY);
    // int rfd = open("/tmp/shm-pipe.r", O_RDONLY);
    printf("writer prepared...\n");
    for (char c = 'A'; c <= 'Z'; ++c) {
      sleep(1);
      printf("\nwrite %c\n", c);
      write(wfd, &c, sizeof(c));
      // char n {};
      // read(rfd, &n, sizeof(n));
      // printf("write echo %c\n", n);
    }
    close(wfd);
    // close(rfd);
  });

  auto reader_maker = [](int k) {
    return [k] {
      printf("r%d prepared...\n", k);
      sleep(1);
      int wfd = open("/tmp/shm-pipe.w", O_RDONLY);
      // int rfd = open("/tmp/shm-pipe.r", O_WRONLY);
      for (;;) {
        char n {};
        read(wfd, &n, sizeof(n));
        printf("r%d %c\n", k, n);
        // write(rfd, &n, sizeof(n));
        if (n == 'Z') break;
      }
      close(wfd);
      // close(rfd);
    };
  };

  auto r1 = test::subproc(reader_maker(1));
  auto r2 = test::subproc(reader_maker(2));

  test::join_subproc(writer);
  test::join_subproc(r1);
  test::join_subproc(r2);
}

#include <sys/inotify.h>
#include <stdio.h>

TEST(shm, event) {
  fclose(fopen("/tmp/shm-event", "w"));

  auto sender = test::subproc([] {
    printf("sender prepared...\n");
    auto fd = fopen("/tmp/shm-event", "w");
    for (int i = 0; i < 10; ++i) {
      sleep(1);
      printf("\nwrite %d\n", i);
      char c {'A'};
      fwrite(&c, 1, 1, fd);
      fflush(fd);
    }
    fclose(fd);
  });

  auto reader_maker = [](int k) {
    return [k] {
      printf("r%d prepared...\n", k);
      sleep(1);
      int ifd = inotify_init();
      int iwd = inotify_add_watch(ifd, "/tmp/shm-event", IN_MODIFY);
      for (int i = 0; i < 10; ++i) {
        struct inotify_event e {};
        read(ifd, &e, sizeof(e));
        printf("r%d %u\n", k, e.mask);
      }
      inotify_rm_watch(ifd, iwd);
      close(ifd);
    };
  };

  auto r1 = test::subproc(reader_maker(1));
  auto r2 = test::subproc(reader_maker(2));

  test::join_subproc(sender);
  test::join_subproc(r1);
  test::join_subproc(r2);
}
#endif