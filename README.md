# cpp-ipc - C++ IPC Library

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mutouyun/cpp-ipc/blob/master/LICENSE) [![Build Status](https://travis-ci.org/mutouyun/cpp-ipc.svg?branch=master)](https://travis-ci.org/mutouyun/cpp-ipc)
 
A high-performance inter-process communication using shared memory on Linux/Windows.  
使用共享内存的跨平台（Linux/Windows，x86/x64/ARM）高性能IPC通讯库。
 
 * 需要支持C++17的编译器（msvc-2017/gcc-7/clang-4）
 * 除STL外，无其他依赖
 * 无锁（lock-free）或轻量级shared-spin-lock（`ipc::channel::connect`/`disconnect`）
 * 底层数据结构为循环数组（circular array）
 * `ipc::route`支持单生产者多消费者（1vN)，`ipc::channel`支持多生产者多消费者（NvM）
 
## Performance

 | | Environment |
 | ------ | ------ |
 | Device | Lenovo ThinkPad T450 |
 | CPU | Intel(R) Core(TM) i5-4300U @ 2.5 GHz |
 | RAM | 16 GB |
 | OS | Windows 7 Ultimate x64 |
 | Compiler | MSVC 2017 15.9.4 |

UT & benchmark test function, see: [test](test)

### ipc::circ::queue

 | PROD-CONS: 1-N | DATAS: 12bits * 1000000 |
 | ------ | ------ |
 | `1-1` | `75.9460 ms, 0.075946 us/d` |
 | `1-2` | `106.582 ms, 0.106582 us/d` |
 | `1-4` | `136.317 ms, 0.136317 us/d` |
 | `1-8` | `148.197 ms, 0.148197 us/d` |

### ipc::route

 | PROD-CONS: 1-N | DATAS: Random 2-256bits * 100000 |
 | ------ | ------ |
 | `RTT` | `165.798 ms, 1.65798 us/d` |
 | `1-1` | `137.251 ms, 1.37251 us/d` |
 | `1-2` | `197.695 ms, 1.97695 us/d` |
 | `1-4` | `396.113 ms, 3.96113 us/d` |
 | `1-8` | `487.245 ms, 4.87245 us/d` |

### ipc::channel

 | PROD-CONS: N-M | DATAS: Random 2-256bits * 100000 |
 | ------ | ------ |
 | `RTT` | `276.056 ms, 2.76056 us/d` |
 | `1-1` | `145.875 ms, 1.45875 us/d` |
 | `1-2` | `298.616 ms, 2.98616 us/d` |
 | `1-4` | `633.239 ms, 6.33239 us/d` |
 | `1-8` | `1113.56 ms, 11.1356 us/d` |
 | `2-1` | `459.178 ms, 2.29589 us/d` |
 | `4-1` | `768.695 ms, 1.92174 us/d` |
 | `8-1` | `1820.28 ms, 2.27535 us/d` |
 | `2-2` | `576.380 ms, 2.88190 us/d` |
 | `4-4` | `2060.98 ms, 5.15245 us/d` |
 | `8-8` | `7355.66 ms, 9.19458 us/d` |

## Reference

 * [http://www.drdobbs.com/lock-free-data-structures/184401865](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [http://www.cnblogs.com/gaochundong/p/lock_free_programming.html](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)