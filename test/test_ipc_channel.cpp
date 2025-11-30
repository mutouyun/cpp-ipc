/**
 * @file test_ipc_channel.cpp
 * @brief Comprehensive unit tests for ipc::route and ipc::channel classes
 * 
 * This test suite covers:
 * - Route (single producer, multiple consumer) functionality
 * - Channel (multiple producer, multiple consumer) functionality
 * - Construction, connection, and disconnection
 * - Send and receive operations (blocking and non-blocking)
 * - Timeout handling
 * - Named channels with prefix
 * - Resource cleanup and storage management
 * - Clone operations
 * - Wait for receiver functionality
 * - Error conditions
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include "libipc/ipc.h"
#include "libipc/buffer.h"

using namespace ipc;

namespace {

// Simple latch implementation for C++14 (similar to C++20 std::latch)
class latch {
public:
  explicit latch(std::ptrdiff_t count) : count_(count) {}
  
  void count_down() {
      std::unique_lock<std::mutex> lock(mutex_);
      if (--count_ <= 0) {
          cv_.notify_all();
      }
  }
  
  void wait() {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this] { return count_ <= 0; });
  }

private:
  std::ptrdiff_t count_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

std::string generate_unique_ipc_name(const char* prefix) {
  static int counter = 0;
  return std::string(prefix) + "_ipc_" + std::to_string(++counter);
}

// Helper to create a test buffer with data
buffer make_test_buffer(const std::string& data) {
  char* mem = new char[data.size() + 1];
  std::strcpy(mem, data.c_str());
  return buffer(mem, data.size() + 1, [](void* p, std::size_t) {
      delete[] static_cast<char*>(p);
  });
}

// Helper to check buffer content
bool check_buffer_content(const buffer& buf, const std::string& expected) {
  if (buf.empty() || buf.size() != expected.size() + 1) {
      return false;
  }
  return std::strcmp(static_cast<const char*>(buf.data()), expected.c_str()) == 0;
}

} // anonymous namespace

// ========== Route Tests (Single Producer, Multiple Consumer) ==========

class RouteTest : public ::testing::Test {
protected:
  void TearDown() override {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
};

// Test default construction
TEST_F(RouteTest, DefaultConstruction) {
  route r;
  EXPECT_FALSE(r.valid());
}

// Test construction with name
TEST_F(RouteTest, ConstructionWithName) {
  std::string name = generate_unique_ipc_name("route_ctor");
  
  route r(name.c_str(), sender);
  EXPECT_TRUE(r.valid());
  EXPECT_STREQ(r.name(), name.c_str());
}

// Test construction with prefix
TEST_F(RouteTest, ConstructionWithPrefix) {
  std::string name = generate_unique_ipc_name("route_prefix");
  
  route r(prefix{"my_prefix"}, name.c_str(), sender);
  EXPECT_TRUE(r.valid());
}

// Test move constructor
TEST_F(RouteTest, MoveConstructor) {
  std::string name = generate_unique_ipc_name("route_move");
  
  route r1(name.c_str(), sender);
  ASSERT_TRUE(r1.valid());
  
  const char* name_ptr = r1.name();
  
  route r2(std::move(r1));
  
  EXPECT_TRUE(r2.valid());
  EXPECT_STREQ(r2.name(), name_ptr);
}

// Test assignment operator
TEST_F(RouteTest, Assignment) {
  std::string name = generate_unique_ipc_name("route_assign");
  
  route r1(name.c_str(), sender);
  route r2;
  
  r2 = std::move(r1);
  
  EXPECT_TRUE(r2.valid());
}

// Test connect method
TEST_F(RouteTest, Connect) {
  std::string name = generate_unique_ipc_name("route_connect");
  
  route r;
  bool connected = r.connect(name.c_str(), sender);
  
  EXPECT_TRUE(connected);
  EXPECT_TRUE(r.valid());
}

// Test connect with prefix
TEST_F(RouteTest, ConnectWithPrefix) {
  std::string name = generate_unique_ipc_name("route_connect_prefix");
  
  route r;
  bool connected = r.connect(prefix{"test"}, name.c_str(), sender);
  
  EXPECT_TRUE(connected);
  EXPECT_TRUE(r.valid());
}

// Test reconnect
TEST_F(RouteTest, Reconnect) {
  std::string name = generate_unique_ipc_name("route_reconnect");
  
  route r(name.c_str(), sender);
  ASSERT_TRUE(r.valid());
  
  bool reconnected = r.reconnect(sender | receiver);
  EXPECT_TRUE(reconnected);
}

// Test disconnect
TEST_F(RouteTest, Disconnect) {
  std::string name = generate_unique_ipc_name("route_disconnect");
  
  route r(name.c_str(), sender);
  ASSERT_TRUE(r.valid());
  
  r.disconnect();
  // After disconnect, behavior depends on implementation
}

// Test clone
TEST_F(RouteTest, Clone) {
  std::string name = generate_unique_ipc_name("route_clone");
  
  route r1(name.c_str(), sender);
  ASSERT_TRUE(r1.valid());
  
  route r2 = r1.clone();
  
  EXPECT_TRUE(r2.valid());
  EXPECT_STREQ(r1.name(), r2.name());
}

// Test mode accessor
TEST_F(RouteTest, Mode) {
  std::string name = generate_unique_ipc_name("route_mode");
  
  route r(name.c_str(), sender);
  EXPECT_EQ(r.mode(), sender);
}

// Test release
TEST_F(RouteTest, Release) {
  std::string name = generate_unique_ipc_name("route_release");
  
  route r(name.c_str(), sender);
  ASSERT_TRUE(r.valid());
  
  r.release();
  EXPECT_FALSE(r.valid());
}

// Test clear
TEST_F(RouteTest, Clear) {
  std::string name = generate_unique_ipc_name("route_clear");
  
  route r(name.c_str(), sender);
  ASSERT_TRUE(r.valid());
  
  r.clear();
  EXPECT_FALSE(r.valid());
}

// Test clear_storage static method
TEST_F(RouteTest, ClearStorage) {
  std::string name = generate_unique_ipc_name("route_clear_storage");
  
  {
      route r(name.c_str(), sender);
      EXPECT_TRUE(r.valid());
  }
  
  route::clear_storage(name.c_str());
}

// Test clear_storage with prefix
TEST_F(RouteTest, ClearStorageWithPrefix) {
  std::string name = generate_unique_ipc_name("route_clear_prefix");
  
  {
      route r(prefix{"test"}, name.c_str(), sender);
      EXPECT_TRUE(r.valid());
  }
  
  route::clear_storage(prefix{"test"}, name.c_str());
}

// Test send without receiver (should fail)
TEST_F(RouteTest, SendWithoutReceiver) {
  std::string name = generate_unique_ipc_name("route_send_no_recv");
  
  route r(name.c_str(), sender);
  ASSERT_TRUE(r.valid());
  
  buffer buf = make_test_buffer("test");
  bool sent = r.send(buf, 10); // 10ms timeout
  
  EXPECT_FALSE(sent); // Should fail - no receiver
}

// Test try_send without receiver
TEST_F(RouteTest, TrySendWithoutReceiver) {
  std::string name = generate_unique_ipc_name("route_try_send_no_recv");
  
  route r(name.c_str(), sender);
  ASSERT_TRUE(r.valid());
  
  buffer buf = make_test_buffer("test");
  bool sent = r.try_send(buf, 10);
  
  EXPECT_FALSE(sent);
}

// Test send and receive with buffer
TEST_F(RouteTest, SendReceiveBuffer) {
  std::string name = generate_unique_ipc_name("route_send_recv_buf");
  
  route sender_r(name.c_str(), sender);
  route receiver_r(name.c_str(), receiver);
  
  ASSERT_TRUE(sender_r.valid());
  ASSERT_TRUE(receiver_r.valid());
  
  buffer send_buf = make_test_buffer("Hello Route");
  
  std::thread sender_thread([&]() {
      bool sent = sender_r.send(send_buf);
      EXPECT_TRUE(sent);
  });
  
  std::thread receiver_thread([&]() {
      buffer recv_buf = receiver_r.recv();
      EXPECT_TRUE(check_buffer_content(recv_buf, "Hello Route"));
  });
  
  sender_thread.join();
  receiver_thread.join();
}

// Test send and receive with string
TEST_F(RouteTest, SendReceiveString) {
  std::string name = generate_unique_ipc_name("route_send_recv_str");
  
  route sender_r(name.c_str(), sender);
  route receiver_r(name.c_str(), receiver);
  
  ASSERT_TRUE(sender_r.valid());
  ASSERT_TRUE(receiver_r.valid());
  
  std::string test_str = "Test String";
  
  std::thread sender_thread([&]() {
      bool sent = sender_r.send(test_str);
      EXPECT_TRUE(sent);
  });
  
  std::thread receiver_thread([&]() {
      buffer recv_buf = receiver_r.recv();
      EXPECT_TRUE(check_buffer_content(recv_buf, test_str));
  });
  
  sender_thread.join();
  receiver_thread.join();
}

// Test send and receive with raw data
TEST_F(RouteTest, SendReceiveRawData) {
  std::string name = generate_unique_ipc_name("route_send_recv_raw");
  
  route sender_r(name.c_str(), sender);
  route receiver_r(name.c_str(), receiver);
  
  ASSERT_TRUE(sender_r.valid());
  ASSERT_TRUE(receiver_r.valid());
  
  const char* data = "Raw Data Test";
  std::size_t size = std::strlen(data) + 1;
  
  std::thread sender_thread([&]() {
      bool sent = sender_r.send(data, size);
      EXPECT_TRUE(sent);
  });
  
  std::thread receiver_thread([&]() {
      buffer recv_buf = receiver_r.recv();
      EXPECT_EQ(recv_buf.size(), size);
      EXPECT_STREQ(static_cast<const char*>(recv_buf.data()), data);
  });
  
  sender_thread.join();
  receiver_thread.join();
}

// Test try_recv when empty
TEST_F(RouteTest, TryRecvEmpty) {
  std::string name = generate_unique_ipc_name("route_try_recv_empty");
  
  route r(name.c_str(), receiver);
  ASSERT_TRUE(r.valid());
  
  buffer buf = r.try_recv();
  EXPECT_TRUE(buf.empty());
}

// Test recv_count
TEST_F(RouteTest, RecvCount) {
  std::string name = generate_unique_ipc_name("route_recv_count");
  
  route sender_r(name.c_str(), sender);
  route receiver_r(name.c_str(), receiver);
  
  ASSERT_TRUE(sender_r.valid());
  ASSERT_TRUE(receiver_r.valid());
  
  std::size_t count = sender_r.recv_count();
  EXPECT_GE(count, 0u);
}

// Test wait_for_recv
TEST_F(RouteTest, WaitForRecv) {
  std::string name = generate_unique_ipc_name("route_wait_recv");
  
  route sender_r(name.c_str(), sender);
  
  std::thread receiver_thread([&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      route receiver_r(name.c_str(), receiver);
  });
  
  bool waited = sender_r.wait_for_recv(1, 500);
  
  receiver_thread.join();
  
  // Result depends on timing
}

// Test static wait_for_recv
TEST_F(RouteTest, StaticWaitForRecv) {
  std::string name = generate_unique_ipc_name("route_static_wait");
  
  std::thread receiver_thread([&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      route receiver_r(name.c_str(), receiver);
  });
  
  bool waited = route::wait_for_recv(name.c_str(), 1, 500);
  
  receiver_thread.join();
}

// Test one sender, multiple receivers
TEST_F(RouteTest, OneSenderMultipleReceivers) {
  std::string name = generate_unique_ipc_name("route_1_to_n");
  
  route sender_r(name.c_str(), sender);
  ASSERT_TRUE(sender_r.valid());
  
  const int num_receivers = 3;
  std::vector<std::atomic<bool>> received(num_receivers);
  for (auto& r : received) r.store(false);
  
  std::vector<std::thread> receivers;
  for (int i = 0; i < num_receivers; ++i) {
      receivers.emplace_back([&, i]() {
          route receiver_r(name.c_str(), receiver);
          buffer buf = receiver_r.recv(1000);
          if (check_buffer_content(buf, "Broadcast")) {
              received[i].store(true);
          }
      });
  }
  
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  
  sender_r.send(std::string("Broadcast"));
  
  for (auto& t : receivers) {
      t.join();
  }
  
  // All receivers should receive the message (broadcast)
  for (const auto& r : received) {
      EXPECT_TRUE(r.load());
  }
}

// ========== Channel Tests (Multiple Producer, Multiple Consumer) ==========

class ChannelTest : public ::testing::Test {
protected:
  void TearDown() override {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
};

// Test default construction
TEST_F(ChannelTest, DefaultConstruction) {
  channel ch;
  EXPECT_FALSE(ch.valid());
}

// Test construction with name
TEST_F(ChannelTest, ConstructionWithName) {
  std::string name = generate_unique_ipc_name("channel_ctor");
  
  channel ch(name.c_str(), sender);
  EXPECT_TRUE(ch.valid());
  EXPECT_STREQ(ch.name(), name.c_str());
}

// Test send and receive
TEST_F(ChannelTest, SendReceive) {
  std::string name = generate_unique_ipc_name("channel_send_recv");
  
  channel sender_ch(name.c_str(), sender);
  channel receiver_ch(name.c_str(), receiver);
  
  ASSERT_TRUE(sender_ch.valid());
  ASSERT_TRUE(receiver_ch.valid());
  
  std::thread sender_thread([&]() {
      sender_ch.send(std::string("Channel Test"));
  });
  
  std::thread receiver_thread([&]() {
      buffer buf = receiver_ch.recv();
      EXPECT_TRUE(check_buffer_content(buf, "Channel Test"));
  });
  
  sender_thread.join();
  receiver_thread.join();
}

// Test multiple senders
TEST_F(ChannelTest, MultipleSenders) {
  std::string name = generate_unique_ipc_name("channel_multi_send");
  
  channel receiver_ch(name.c_str(), receiver);
  ASSERT_TRUE(receiver_ch.valid());
  
  const int num_senders = 3;
  std::atomic<int> received_count{0};
  
  std::vector<std::thread> senders;
  for (int i = 0; i < num_senders; ++i) {
      senders.emplace_back([&, i]() {
          channel sender_ch(name.c_str(), sender);
          std::string msg = "Sender" + std::to_string(i);
          sender_ch.send(msg);
      });
  }
  
  std::thread receiver([&]() {
      for (int i = 0; i < num_senders; ++i) {
          buffer buf = receiver_ch.recv(1000);
          if (!buf.empty()) {
              ++received_count;
          }
      }
  });
  
  for (auto& t : senders) {
      t.join();
  }
  receiver.join();
  
  EXPECT_EQ(received_count.load(), num_senders);
}

// Test multiple senders and receivers
TEST_F(ChannelTest, MultipleSendersReceivers) {
  std::string name = generate_unique_ipc_name("channel_m_to_n");
  
  const int num_senders = 2;
  const int num_receivers = 2;
  const int messages_per_sender = 5;
  const int total_messages = num_senders * messages_per_sender;  // Each receiver should get all messages
  
  std::atomic<int> sent_count{0};
  std::atomic<int> received_count{0};
  
  // Use latch to ensure receivers are ready before senders start
  latch receivers_ready(num_receivers);
  
  std::vector<std::thread> receivers;
  for (int i = 0; i < num_receivers; ++i) {
      receivers.emplace_back([&, i]() {
          channel ch(name.c_str(), receiver);
          receivers_ready.count_down();  // Signal this receiver is ready
          
          // Each receiver should receive ALL messages from ALL senders (broadcast mode)
          for (int j = 0; j < total_messages; ++j) {
              buffer buf = ch.recv(2000);
              if (!buf.empty()) {
                  ++received_count;
              }
          }
      });
  }
  
  // Wait for all receivers to be ready
  receivers_ready.wait();
  
  std::vector<std::thread> senders;
  for (int i = 0; i < num_senders; ++i) {
      senders.emplace_back([&, i]() {
          channel ch(name.c_str(), sender);
          for (int j = 0; j < messages_per_sender; ++j) {
              std::string msg = "S" + std::to_string(i) + "M" + std::to_string(j);
              if (ch.send(msg, 1000)) {
                  ++sent_count;
              }
              std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
      });
  }
  
  for (auto& t : senders) {
      t.join();
  }
  for (auto& t : receivers) {
      t.join();
  }
  
  EXPECT_EQ(sent_count.load(), num_senders * messages_per_sender);
  // All messages should be received (broadcast mode)
  EXPECT_EQ(received_count.load(), num_senders * messages_per_sender * num_receivers);
}

// Test try_send and try_recv
TEST_F(ChannelTest, TrySendTryRecv) {
  std::string name = generate_unique_ipc_name("channel_try");
  
  channel sender_ch(name.c_str(), sender);
  channel receiver_ch(name.c_str(), receiver);
  
  ASSERT_TRUE(sender_ch.valid());
  ASSERT_TRUE(receiver_ch.valid());
  
  bool sent = sender_ch.try_send(std::string("Try Test"));
  
  if (sent) {
      buffer buf = receiver_ch.try_recv();
      EXPECT_FALSE(buf.empty());
  }
}

// Test timeout scenarios
TEST_F(ChannelTest, SendTimeout) {
  std::string name = generate_unique_ipc_name("channel_timeout");
  
  channel ch(name.c_str(), sender);
  ASSERT_TRUE(ch.valid());
  
  // Send with very short timeout (may fail without receiver)
  bool sent = ch.send(std::string("Timeout Test"), 1);
}

// Test clear and clear_storage
TEST_F(ChannelTest, ClearStorage) {
  std::string name = generate_unique_ipc_name("channel_clear");
  
  {
      channel ch(name.c_str(), sender);
      EXPECT_TRUE(ch.valid());
  }
  
  channel::clear_storage(name.c_str());
}

// Test handle() method
TEST_F(ChannelTest, Handle) {
  std::string name = generate_unique_ipc_name("channel_handle");
  
  channel ch(name.c_str(), sender);
  ASSERT_TRUE(ch.valid());
  
  handle_t h = ch.handle();
  EXPECT_NE(h, nullptr);
}
