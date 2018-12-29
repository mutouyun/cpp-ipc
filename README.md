# cpp-ipc - C++ IPC Library

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mutouyun/cpp-ipc/blob/master/LICENSE) [![Build Status](https://travis-ci.org/mutouyun/cpp-ipc.svg?branch=master)](https://travis-ci.org/mutouyun/cpp-ipc)
 
A high-performance inter-process communication using shared memory on Linux/Windows.  
使用共享内存的跨平台（Linux/Windows，x86/x64/ARM）高性能IPC通讯库。
 
 * 需要支持C++17的编译器（msvc-2017/gcc-7/clang-4）
 * 除STL外，无其他依赖
 * 无锁（lock-free）或轻量级shared-spin-lock（`ipc::channel::connect`/`disconnect`）
 * 底层数据结构为循环数组（circular array）
 * `ipc::route`支持单生产多消费，`ipc::channel`支持多生产多消费
 
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
 | `RTT` | `228.383 ms, 2.28383 us/d` |
 | `1-1` | `144.371 ms, 1.44371 us/d` |
 | `1-2` | `249.874 ms, 2.49874 us/d` |
 | `1-4` | `525.721 ms, 5.25721 us/d` |
 | `1-8` | `729.373 ms, 7.29373 us/d` |
 | `2-1` | `311.176 ms, 1.55588 us/d` |
 | `4-1` | `744.733 ms, 1.86183 us/d` |
 | `8-1` | `1819.54 ms, 2.27442 us/d` |
 | `2-2` | `573.536 ms, 2.86768 us/d` |
 | `4-4` | `1629.51 ms, 4.07378 us/d` |
 | `8-8` | `6336.53 ms, 7.92066 us/d` |

## Reference

 * [http://www.drdobbs.com/lock-free-data-structures/184401865](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [http://www.cnblogs.com/gaochundong/p/lock_free_programming.html](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)
