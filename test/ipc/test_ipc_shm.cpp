
#include "gtest/gtest.h"

#include "libipc/shm.h"

TEST(shm, create_close) {
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

#if 0
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>

#include "test_util.h"

TEST(shm, sock) {
  auto reader = test::subproc([] {
    int lfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in ser {};
    ser.sin_family = AF_INET;
    ser.sin_addr.s_addr = htonl(INADDR_ANY);
    ser.sin_port = htons(8888);
    bind(lfd, (struct sockaddr *)&ser, sizeof(ser));
    printf("reader prepared...\n");
    struct sockaddr_in cli {};
    socklen_t cli_len = sizeof(cli);
    char c {'\0'};
    printf("reading...\n");
    for (;;) {
      recvfrom(lfd, &c, sizeof(c), 0, (struct sockaddr *)&cli, &cli_len);
      printf("read %c\n", c);
      sendto(lfd, &c, sizeof(c), 0, (struct sockaddr *)&cli, cli_len);
      if (c == 'Z') break;
    }
    close(lfd);
  });

  auto writer = test::subproc([] {
    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in ser {};
    ser.sin_family = AF_INET;
    ser.sin_addr.s_addr = htonl(INADDR_ANY);
    ser.sin_port = htons(8888);

    printf("writer prepared...\n");
    sleep(1);

    for (char c = 'A'; c <= 'Z'; ++c) {
      printf("write %c\n", c);
      sendto(sfd, &c, sizeof(c), 0, (struct sockaddr *)&ser, sizeof(ser));
      struct sockaddr_in cli {};
      socklen_t len = sizeof(cli);
      char n {};
      recvfrom(sfd, &n, sizeof(n), 0, (struct sockaddr *)&cli, &len);
      printf("echo %c\n", c);
    }
    close(sfd);
  });

  test::join_subproc(writer);
  test::join_subproc(reader);
}
#endif