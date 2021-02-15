
#include <signal.h>

#include <iostream>
#include <string>
#include <cstring>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstddef>

#include "libipc/ipc.h"
#include "libipc/shm.h"
#include "capo/random.hpp"

#include <unordered_map>
#include <vector>

using string = std::string;

template <class K, class V>
using map = std::unordered_map<K, V>;

using namespace ipc::shm;

namespace {

constexpr char const name__  [] = "ipc-kvs";
constexpr char const mode_s__[] = "s";
constexpr char const mode_c__[] = "c";
constexpr char const mode_t__[] = "t"; // test mode

// constexpr std::size_t const min_sz = 1;
// constexpr std::size_t const max_sz = 1024 * 1024 * 512;

std::atomic<bool> is_quit__{ false };
std::atomic<std::size_t> size_counter__{ 0 };

// using msg_que_t = ipc::chan<ipc::relat::single, ipc::relat::single, ipc::trans::unicast>;
// msg_que_t que__{ name__ };
// ipc::byte_t buff__[128];

capo::random<> rand__{ 
    static_cast<int>(1), 
    static_cast<int>(127)
};

ipc::channel shared_chan { name__, ipc::sender | ipc::receiver };

inline std::string str_of_size(std::size_t sz) noexcept {
    if (sz > 1024 * 1024) {
        return std::to_string(sz / (1024 * 1024)) + " MB";
    }
    if (sz > 1024) {
        return std::to_string(sz / 1024) + " KB";
    }
    return std::to_string(sz) + " bytes";
}

inline std::string speed_of(std::size_t sz) noexcept {
    return str_of_size(sz) + "/s";
}

void do_counting() {
    for (int i = 1; !is_quit__.load(std::memory_order_acquire); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100 ms
        if (i % 10) continue;
        i = 0;
        std::cout
            << speed_of(size_counter__.exchange(0, std::memory_order_relaxed))
            << std::endl;
    }
}

void kvs_server() {
    // map<string, char *> key_val_map;
    map<string, uint32_t> key_len_map;
    std::cout << "Running kvs server...\n";
    while (1) {
        // std::printf("2 recving\n");
        auto dd = shared_chan.recv();
        auto str = static_cast<char*>(dd.data());

        auto recv_stamp = std::chrono::system_clock::now();

        // for (int i = 0; i < strlen(str); i++){
        //     std::cout << (int)str[i] << " ";
        // }
        // std::cout << "\n";

        if (str == nullptr) {
            std::cout << "Receive null str\n";
            continue;
        }
        
        // request addres (1 byte) | resp address (1 byte) | get/put (1 byte) | request id (1 byte) | metadata len (1 byte)| metadata | optional value
        if (str[0] != 1) {
            std::cout << "Not for server\n";
            continue;
        }

        // std::printf("2 recv: %s\n", str);
        auto resp_address = str[1];
        bool is_read = (str[2] == 1);
        auto req_id = str[3];
        int meta_data_len = (int)str[4];

        string key_name(str + 5, meta_data_len);

        string resp;
        resp.push_back(resp_address);
        resp.push_back(req_id);

        // response address (1 byte) | request id (1 byte) | is_success (1 byte) | optional value
        if (is_read){
            // get request
            std::cout << "Getting " << key_name << " ...\n";
            if (key_len_map.find(key_name) != key_len_map.end()) {
                auto size_len = key_len_map[key_name];

                resp.push_back(1);
                // resp.push_back((char) size_len);
                // resp.push_back((char) size_len >> 8);
                // resp.push_back((char) size_len >> 16);
                // resp.push_back((char) size_len >> 24);
                resp += std::to_string(size_len);
            }
            else {
                std::cout << key_name << " not exists\n";
                resp.push_back(2);
            }
        }
        else{
            // put request
            std::cout << "Putting " << key_name << " ...\n";
            
            // auto size_len = (uint32_t) str[5 + meta_data_len]       |
            //                 (uint32_t) str[7 + meta_data_len] << 8  |
            //                 (uint32_t) str[8 + meta_data_len] << 16 |
            //                 (uint32_t) str[9 + meta_data_len] << 24;
            
            auto size_len = stoi(string(str + 5 + meta_data_len));

            // handle shm_hd(key_name.c_str(), size_len);
            // auto shm_ptr = (char *) shm_hd.get();
            auto shm_id = acquire(key_name.c_str(), size_len, open);

            // auto shm_ptr = (char *) get_mem(shm_id, nullptr);
            // for (int i = 0; i < strlen(shm_ptr); i++){
            //     std::cout << shm_ptr[i] << " ";
            // }
            // std::cout << "\n";
            

            if (shm_id == nullptr){
                std::cout << "Shm null ptr for " << key_name << "\n";
                resp.push_back(2);
            }
            else {
                auto shm_ptr = (char *) get_mem(shm_id, nullptr);
                // auto val_size = strlen(shm_ptr);
                // std::cout << "shm_size " << size_len << " val_size " << val_size << "\n";

                // key_val_map[key_name] = shm_ptr;
                key_len_map[key_name] = size_len;
                resp.push_back(1);
            }
        }

        auto ready_stamp = std::chrono::system_clock::now();
        auto handling_time = std::chrono::duration_cast<std::chrono::microseconds>(ready_stamp - recv_stamp).count();

        auto req_type = is_read ? "Get" : "Put";
        std::cout << "Handled " << req_type << " " << key_name << ", handling_time: " << handling_time << "\n";

        // try sending ack
        while (!shared_chan.send(resp)) {
            // waiting for connection
            shared_chan.wait_for_recv(2);
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        // std::cout << "Repeat \n";
        // auto val = key_val_map[key_name];
        // for (int i = 0; i < strlen(val); i++){
        //     std::cout << val[i] << " ";
        // }
        // std::cout << "\n";
    }

    std::cout << __func__ << ": quit...\n";
}

void kvs_client(char id, bool is_read, string info) {
    auto client_id = 2 + id;
    std::cout << "Launching client " << client_id << " ...\n";

    // request addres (1 byte) | resp address (1 byte) | get/put (1 byte) | request id (1 byte) | metadata len (1 byte)| metadata | optional value
    auto req_id = rand__();

    auto start_stamp = std::chrono::system_clock::now();
    string req;
    req.push_back(1);
    req.push_back(client_id);

    string key_name = "a" + info;

    if (is_read){
        req.push_back(1);
        req.push_back(req_id);

        req.push_back((char) key_name.size());
        req += key_name;
    }
    else {
        req.push_back(2);
        req.push_back(req_id);

        int data_len = stoi(info);

        req.push_back((char) key_name.size());
        req += key_name;

        auto shm_size = data_len + 1;
        // handle shm_hd(key_name.c_str(), shm_size);
        // auto shm_ptr = (char *) shm_hd.get();

        auto shm_id = acquire(key_name.c_str(), shm_size);
        auto shm_ptr = (char *) get_mem(shm_id, nullptr);
        // for (int i = 0; i < data_len; i++){
        //     shm_ptr[i] = '1';
        // }
        memset(shm_ptr, '1', data_len);
        shm_ptr[data_len] = '\0';

        // req.push_back((char) shm_size);
        // req.push_back((char) shm_size >> 8);
        // req.push_back((char) shm_size >> 16);
        // req.push_back((char) shm_size >> 24);
        req += std::to_string(shm_size);
        
        std::cout << "shm_size " << shm_size << "\n";
    }

    auto ready_stamp = std::chrono::system_clock::now();

    while (!shared_chan.send(req)) {
        // waiting for connection
        shared_chan.wait_for_recv(2);
    }

    // recv ack
    auto dd = shared_chan.recv();
    auto str = static_cast<char*>(dd.data());

    // for (int i = 0; i < strlen(str); i++){
    //     std::cout << (int)str[i] << " ";
    // }
    // std::cout << "\n";

    // response address (1 byte) | request id (1 byte) | is_success (1 byte) | optional value
    if (str == nullptr) {
        std::cout << "Ack error\n";
    }
    else if (client_id != (int) str[0]){
        std::cout << "Not my ack " << (int) str[0] << "\n";
        return;
    }
    else {
        auto ack_stamp = std::chrono::system_clock::now();

        if (str[1] == req_id) {
            if (is_read){

                // auto size_len = (uint32_t) str[3]       |
                //                 (uint32_t) str[4] << 8  |
                //                 (uint32_t) str[5] << 16 |
                //                 (uint32_t) str[6] << 24;
                
                auto size_len = stoi(string(str + 3));
                auto shm_id = acquire(key_name.c_str(), size_len);
                auto shm_ptr = (char *) get_mem(shm_id, nullptr);
                // for (int i = 0; i < strlen(shm_ptr); i++){
                //     std::cout << shm_ptr[i] << " ";
                // }
                // std::cout << "\n";
                auto ptr_stamp = std::chrono::system_clock::now();

                auto val_size = strlen(shm_ptr);
                auto val_stamp = std::chrono::system_clock::now();
                
                auto ready_time = std::chrono::duration_cast<std::chrono::microseconds>(ready_stamp - start_stamp).count();
                auto ack_time = std::chrono::duration_cast<std::chrono::microseconds>(ack_stamp - ready_stamp).count();
                auto ptr_time = std::chrono::duration_cast<std::chrono::microseconds>(ptr_stamp - ack_stamp).count();
                auto val_time = std::chrono::duration_cast<std::chrono::microseconds>(val_stamp - ptr_stamp).count();

                std::cout << "Receive Get " << key_name << ", val_size: " << val_size 
                                                        << ", shm_size: " << size_len 
                                                        << ", ready_time: " << ready_time 
                                                        << ", ack_time: " << ack_time 
                                                        << ", ptr_time: " << ptr_time 
                                                        << ", val_time: " << val_time 
                                                        <<"\n";
            }
            else {
                auto ready_time = std::chrono::duration_cast<std::chrono::microseconds>(ready_stamp - start_stamp).count();
                auto ack_time = std::chrono::duration_cast<std::chrono::microseconds>(ack_stamp - ready_stamp).count();

                std::cout << "Receive Put " << key_name << ", ready_time "<< ready_time 
                                                        << ", ack_time: " << ack_time 
                                                        <<"\n";
            }
        }
        else {
            std::cout << "Request id " << req_id << " not match " << (int)str[1] << "\n";
        }
    }
    std::cout << __func__ << ": quit...\n";
}

// char test_str[1024 * 1024 * 512];
void test(int len){
    // auto start_stamp = std::chrono::system_clock::now();

    // memset(test_str, '1', len);
    // test_str[len] = '\0';
    // auto memset_stamp = std::chrono::system_clock::now();
    
    // auto val_size = strlen(test_str);
    // auto size_stamp = std::chrono::system_clock::now();
    
    // auto memset_time = std::chrono::duration_cast<std::chrono::microseconds>(memset_stamp - start_stamp).count();
    // auto size_time = std::chrono::duration_cast<std::chrono::microseconds>(size_stamp - memset_stamp).count();
    // std::cout << "memset " << len << ", memset_time "<< memset_time << ", size_time " << size_time << "\n"; 
}

} // namespace

int main(int argc, char ** argv) {
    if (argc < 2) return 0;

    auto exit = [](int) {
        is_quit__.store(true, std::memory_order_release);
        shared_chan.disconnect();
    };
    ::signal(SIGINT  , exit);
    ::signal(SIGABRT , exit);
    ::signal(SIGSEGV , exit);
    ::signal(SIGTERM , exit);
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)
    ::signal(SIGBREAK, exit);
#else
    ::signal(SIGHUP  , exit);
#endif

    if (std::string{ argv[1] } == mode_s__) {
        kvs_server();
    }
    else if (std::string{ argv[1] } == mode_c__) {
        if (argc < 5) {
            std::cout << "Require indicating client id, request type, and info.\n";
            return 0;
        }
        int id = std::stoi(string{argv[2]});
        bool is_read = std::stoi(string{argv[3]}) == 1;
        string info{ argv[4] };
        kvs_client(id, is_read, info);
    }
    else if (std::string{ argv[1] } == mode_t__) {
        int len = std::stoi(string{argv[2]});
        test(len);
    }
    return 0;
}
