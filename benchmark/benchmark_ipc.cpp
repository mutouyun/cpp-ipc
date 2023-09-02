#include <libimp/detect_plat.h>
#if defined(LIBIMP_OS_LINUX)

#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <netinet/in.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>

#include <atomic>

#include "benchmark/benchmark.h"

#include "test_util.h"

#include <libipc/shm.h>

namespace {

using flag_t = std::atomic_bool;

struct eventfd_reader {
  pid_t pid_ {-1};
  int   rfd_ {eventfd(0, 0/*EFD_CLOEXEC*/)};
  int   wfd_ {eventfd(0, 0/*EFD_CLOEXEC*/)};

  eventfd_reader() {
    auto shm = ipc::shared_memory("shm-eventfd_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(false, std::memory_order_relaxed);
    pid_ = test::subproc([this] {
      auto shm = ipc::shared_memory("shm-eventfd_reader", sizeof(flag_t));
      auto flag = shm.as<flag_t>();
      printf("eventfd_reader rfd = %d, wfd = %d\n", rfd_, wfd_);
      while (!flag->load(std::memory_order_relaxed)) {
        uint64_t n = 0;
        read(wfd_, &n, sizeof(uint64_t));
        n = 1;
        write(rfd_, &n, sizeof(uint64_t));
      }
      printf("eventfd_reader exit.\n");
    });
  }

  ~eventfd_reader() {
    auto shm = ipc::shared_memory("shm-eventfd_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(true, std::memory_order_seq_cst);
    uint64_t n = 1;
    write(wfd_, &n, sizeof(uint64_t));
    test::join_subproc(pid_);
    close(rfd_);
    close(wfd_);
  }
} evtfd_r__;

struct mqueue_reader {
  pid_t pid_ {-1};

  mqueue_reader() {
    auto shm = ipc::shared_memory("shm-mqueue_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(false, std::memory_order_relaxed);

    pid_ = test::subproc([this] {
      auto shm = ipc::shared_memory("shm-mqueue_reader", sizeof(flag_t));
      auto flag = shm.as<flag_t>();
      printf("mqueue_reader start.\n");
      mq_unlink("/mqueue-wfd");
      mq_unlink("/mqueue-rfd");
      struct mq_attr attr = {0, 10, 1, 0};
      mqd_t wfd = mq_open("/mqueue-wfd", O_CREAT | O_RDONLY, 0666, &attr);
      mqd_t rfd = mq_open("/mqueue-rfd", O_CREAT | O_WRONLY, 0666, &attr);
      if (wfd < 0 || rfd < 0) {
        printf("mq_open error. errno = %d\n", errno);
        return;
      }
      while (!flag->load(std::memory_order_relaxed)) {
        char n {};
        // read
        if (mq_receive(wfd, &n, sizeof(n), nullptr) < 0) {
          printf("mq_receive error. errno = %d\n", errno);
          return;
        }
        // write
        if (mq_send(rfd, &n, sizeof(n), 0) < 0) {
          printf("mq_send error. errno = %d\n", errno);
          return;
        }
      }
      printf("mqueue_reader exit.\n");
      mq_close(wfd);
      mq_close(rfd);
    });
  }

  ~mqueue_reader() {
    auto shm = ipc::shared_memory("shm-mqueue_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(true, std::memory_order_seq_cst);
    {
      mqd_t wfd = mq_open("/mqueue-wfd", O_EXCL | O_WRONLY, 0666, NULL);
      if (wfd < 0) {
        printf("mq_open error. errno = %d\n", errno);
        return;
      }
      char n {};
      if (mq_send(wfd, &n, sizeof(n), 0) < 0) {
        printf("mq_send error. errno = %d\n", errno);
        return;
      }
      mq_close(wfd);
    }
    test::join_subproc(pid_);
  }
} mq_r__;

struct pipe_reader {
  pid_t pid_ {-1};

  pipe_reader() {
    auto shm = ipc::shared_memory("shm-pipe_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(false, std::memory_order_relaxed);

    pid_ = test::subproc([this] {
      auto shm = ipc::shared_memory("shm-pipe_reader", sizeof(flag_t));
      auto flag = shm.as<flag_t>();
      printf("pipe_reader start.\n");
      mkfifo("/tmp/shm-pipe.w", S_IFIFO|0666);
      mkfifo("/tmp/shm-pipe.r", S_IFIFO|0666);
      int wfd = open("/tmp/shm-pipe.w", O_RDWR);
      int rfd = open("/tmp/shm-pipe.r", O_RDWR);
      while (!flag->load(std::memory_order_relaxed)) {
        char n {};
        // read
        read(wfd, &n, sizeof(n));
        // write
        write(rfd, &n, sizeof(n));
      }
      printf("pipe_reader exit.\n");
      close(wfd);
      close(rfd);
    });
  }

  ~pipe_reader() {
    auto shm = ipc::shared_memory("shm-pipe_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(true, std::memory_order_seq_cst);
    {
      mqd_t wfd = open("/tmp/shm-pipe.w", O_WRONLY);
      char n {};
      write(wfd, &n, sizeof(n));
      close(wfd);
    }
    test::join_subproc(pid_);
  }
} pipe_r__;

struct sock_reader {
  pid_t pid_ {-1};

  sock_reader() {
    auto shm = ipc::shared_memory("shm-sock_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(false, std::memory_order_relaxed);

    pid_ = test::subproc([this] {
      auto shm = ipc::shared_memory("shm-sock_reader", sizeof(flag_t));
      auto flag = shm.as<flag_t>();
      printf("sock_reader start.\n");
      int lfd = socket(AF_UNIX, SOCK_STREAM, 0);
      struct sockaddr_un serun {};
      serun.sun_family = AF_UNIX;
      strcpy(serun.sun_path, "shm-sock.ser");
      unlink(serun.sun_path);
      bind(lfd, (struct sockaddr *)&serun, offsetof(struct sockaddr_un, sun_path) + strlen(serun.sun_path));
      listen(lfd, 16);
      while (!flag->load(std::memory_order_relaxed)) {
        struct sockaddr_un cliun {};
        socklen_t cliun_len = sizeof(cliun);
        int cfd = accept(lfd, (struct sockaddr *)&cliun, &cliun_len);
        if (cfd < 0) {
          printf("accept error.\n");
          continue;
        }
        while (!flag->load(std::memory_order_relaxed)) {
          char c {};
          auto r = read(cfd, &c, sizeof(c));
          if (r <= 0) break;
          write(cfd, &c, sizeof(c));
        }
        close(cfd);
      }
      printf("sock_reader exit.\n");
    });
  }

  ~sock_reader() {
    auto shm = ipc::shared_memory("shm-sock_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(true, std::memory_order_seq_cst);
    {
      auto sfd = start_client();
      char c {};
      write(sfd, &c, sizeof(c));
      close(sfd);
    }
    test::join_subproc(pid_);
  }

  int start_client() {
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un cliun {};
    cliun.sun_family = AF_UNIX;
    strcpy(cliun.sun_path, "shm-sock.cli");
    unlink(cliun.sun_path);
    bind(sfd, (struct sockaddr *)&cliun, offsetof(struct sockaddr_un, sun_path) + strlen(cliun.sun_path));
    struct sockaddr_un serun {};
    serun.sun_family = AF_UNIX;
    strcpy(serun.sun_path, "shm-sock.ser");
    connect(sfd, (struct sockaddr *)&serun, offsetof(struct sockaddr_un, sun_path) + strlen(serun.sun_path));
    return sfd;
  }
} sock_r__;

struct udp_reader {
  pid_t pid_ {-1};

  udp_reader() {
    auto shm = ipc::shared_memory("shm-udp_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(false, std::memory_order_relaxed);

    pid_ = test::subproc([this] {
      auto shm = ipc::shared_memory("shm-udp_reader", sizeof(flag_t));
      auto flag = shm.as<flag_t>();
      printf("udp_reader start.\n");
      int lfd = socket(AF_INET, SOCK_DGRAM, 0);
      struct sockaddr_in ser {};
      ser.sin_family = AF_INET;
      ser.sin_addr.s_addr = htonl(INADDR_ANY);
      ser.sin_port = htons(8888);
      bind(lfd, (struct sockaddr *)&ser, sizeof(ser));
      while (!flag->load(std::memory_order_relaxed)) {
        struct sockaddr_in cli {};
        socklen_t cli_len = sizeof(cli);
        char c {};
        recvfrom(lfd, &c, sizeof(c), 0, (struct sockaddr *)&cli, &cli_len);
        sendto(lfd, &c, sizeof(c), 0, (struct sockaddr *)&cli, cli_len);
      }
      printf("udp_reader exit.\n");
    });
  }

  ~udp_reader() {
    auto shm = ipc::shared_memory("shm-udp_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(true, std::memory_order_seq_cst);
    {
      auto sfd = socket(AF_INET, SOCK_DGRAM, 0);
      struct sockaddr_in ser {};
      ser.sin_family = AF_INET;
      ser.sin_addr.s_addr = htonl(INADDR_ANY);
      ser.sin_port = htons(8888);
      char c {};
      sendto(sfd, &c, sizeof(c), 0, (struct sockaddr *)&ser, sizeof(ser));
      close(sfd);
    }
    test::join_subproc(pid_);
  }
} udp_r__;

struct inotify_reader {
  pid_t pid_ {-1};

  inotify_reader() {
    auto shm = ipc::shared_memory("shm-inotify_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(false, std::memory_order_relaxed);
    fclose(fopen("/tmp/shm-inotify.0", "w"));
    fclose(fopen("/tmp/shm-inotify.1", "w"));
    pid_ = test::subproc([this] {
      auto shm  = ipc::shared_memory("shm-inotify_reader", sizeof(flag_t));
      auto flag = shm.as<flag_t>();
      auto shm_rt = ipc::shared_memory("shm-inotify_reader.0", sizeof(flag_t));
      auto shm_wt = ipc::shared_memory("shm-inotify_reader.1", sizeof(flag_t));
      auto f_rt = shm_rt.as<flag_t>();
      auto f_wt = shm_wt.as<flag_t>();
      int ifd = inotify_init();
      int iwd = inotify_add_watch(ifd, "/tmp/shm-inotify.0", IN_OPEN | IN_CLOSE);
      // auto wf = fopen("/tmp/shm-inotify.1", "w");
      printf("inotify_reader start.\n");
      while (!flag->load(std::memory_order_relaxed)) {
        // read
        if (!f_rt->exchange(false, std::memory_order_acquire)) {
          struct inotify_event e {};
          read(ifd, &e, sizeof(e));
        }
        // write
        f_wt->store(true, std::memory_order_release);
        fclose(fopen("/tmp/shm-inotify.1", "r"));
        // char c {'A'};
        // fwrite(&c, 1, 1, wf);
        // fflush(wf);
      }
      // fclose(wf);
      inotify_rm_watch(ifd, iwd);
      close(ifd);
      printf("inotify_reader exit.\n");
    });
  }

  ~inotify_reader() {
    auto shm = ipc::shared_memory("shm-inotify_reader", sizeof(flag_t));
    shm.as<flag_t>()->store(true, std::memory_order_seq_cst);
    auto shm_rt = ipc::shared_memory("shm-inotify_reader.0", sizeof(flag_t));
    shm_rt.as<flag_t>()->store(true, std::memory_order_seq_cst);
    fclose(fopen("/tmp/shm-inotify.0", "r"));
    // auto rf = fopen("/tmp/shm-inotify.0", "w");
    // char c {'A'};
    // fwrite(&c, 1, 1, rf);
    // fflush(rf);
    // fclose(rf);
    test::join_subproc(pid_);
  }
} inotify_r__;

void ipc_eventfd_rtt(benchmark::State &state) {
  for (auto _ : state) {
    uint64_t n = 1;
    write(evtfd_r__.wfd_, &n, sizeof(n));
    read (evtfd_r__.rfd_, &n, sizeof(n));
  }
}

void ipc_mqueue_rtt(benchmark::State &state) {
  mqd_t wfd = mq_open("/mqueue-wfd", O_EXCL | O_WRONLY, 0666, NULL);
  mqd_t rfd = mq_open("/mqueue-rfd", O_EXCL | O_RDONLY, 0666, NULL);
  if (wfd < 0 || rfd < 0) {
    printf("mq_open error. errno = %d\n", errno);
    return;
  }
  for (auto _ : state) {
    char n {};
    // write
    if (mq_send(wfd, &n, sizeof(n), 0) < 0) {
      printf("mq_send error. errno = %d\n", errno);
      return;
    }
    // read
    if (mq_receive(rfd, &n, sizeof(n), nullptr) < 0) {
      printf("mq_receive error. errno = %d\n", errno);
      return;
    }
  }
  mq_close(wfd);
  mq_close(rfd);
}

void ipc_npipe_rtt(benchmark::State &state) {
  int wfd = open("/tmp/shm-pipe.w", O_WRONLY);
  int rfd = open("/tmp/shm-pipe.r", O_RDONLY);
  for (auto _ : state) {
    char n {};
    // write
    write(wfd, &n, sizeof(n));
    // read
    read(rfd, &n, sizeof(n));
  }
  close(wfd);
  close(rfd);
}

void ipc_sock_rtt(benchmark::State &state) {
  auto sfd = sock_r__.start_client();
  for (auto _ : state) {
    char n {};
    write(sfd, &n, sizeof(n));
    read (sfd, &n, sizeof(n));
  }
  close(sfd);
}

void ipc_udp_rtt(benchmark::State &state) {
  auto sfd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in ser {};
  ser.sin_family = AF_INET;
  ser.sin_addr.s_addr = htonl(INADDR_ANY);
  ser.sin_port = htons(8888);
  for (auto _ : state) {
    char n {'A'};
    // write
    sendto(sfd, &n, sizeof(n), 0, (struct sockaddr *)&ser, sizeof(ser));
    // read
    struct sockaddr_in cli {};
    socklen_t len = sizeof(cli);
    recvfrom(sfd, &n, sizeof(n), 0, (struct sockaddr *)&cli, &len);
  }
  close(sfd);
}

void ipc_inotify_rtt(benchmark::State &state) {
  auto shm_rt = ipc::shared_memory("shm-inotify_reader.0", sizeof(flag_t));
  auto shm_wt = ipc::shared_memory("shm-inotify_reader.1", sizeof(flag_t));
  auto f_rt = shm_rt.as<flag_t>();
  auto f_wt = shm_wt.as<flag_t>();
  int ifd = inotify_init();
  int iwd = inotify_add_watch(ifd, "/tmp/shm-inotify.1", IN_OPEN | IN_CLOSE);
  // auto rf = fopen("/tmp/shm-inotify.0", "w");
  for (auto _ : state) {
    // write
    f_rt->store(true, std::memory_order_release);
    fclose(fopen("/tmp/shm-inotify.0", "r"));
    // char c {'A'};
    // fwrite(&c, 1, 1, rf);
    // fflush(rf);
    // read
    if (!f_wt->exchange(false, std::memory_order_acquire)) {
      struct inotify_event e {};
      read(ifd, &e, sizeof(e));
    }
  }
  // fclose(rf);
  inotify_rm_watch(ifd, iwd);
  close(ifd);
}

} // namespace

BENCHMARK(ipc_eventfd_rtt);
BENCHMARK(ipc_mqueue_rtt);
BENCHMARK(ipc_npipe_rtt);
BENCHMARK(ipc_sock_rtt);
BENCHMARK(ipc_udp_rtt);
BENCHMARK(ipc_inotify_rtt);
#endif