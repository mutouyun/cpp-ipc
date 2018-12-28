# cpp-ipc - C++ IPC Library

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mutouyun/cpp-ipc/blob/master/LICENSE) [![Build Status](https://travis-ci.org/mutouyun/cpp-ipc.svg?branch=master)](https://travis-ci.org/mutouyun/cpp-ipc)
 
A high-performance inter-process communication using shared memory on Linux/Windows.  
使用共享内存的跨平台（Linux/Windows，x86/x64/ARM）高性能IPC通讯库。
 
 * 需要支持C++17的编译器（msvc-2017/gcc-7/clang-4）
 * 除STL外，无其他依赖
 * 无锁（lock-free）或轻量级shared-spin-lock（`ipc::channel::connect`/`disconnect`）
 * 底层数据结构为循环数组（circular array），无动态内存分配
 * `ipc::route`支持单生产者多消费者（1vN)，`ipc::channel`支持多生产者多消费者（NvM）
 
## Performance

 | | Environment |
 | ------ | ------ |
 | Device | Lenovo ThinkPad T450 |
 | CPU | Intel(R) Core(TM) i5-4300U @ 2.5 GHz |
 | RAM | 16 GB |
 | OS | Windows 7 Ultimate x64 |
 | Compiler | MSVC 2017 15.9.3 |

UT & benchmark test function, see: [test](test)

### ipc::circ::queue

 | PROD-CONS: 1-N | DATAS: 12bits * 1000000 |
 | ------ | ------ |
 | `1-1` | `072.150 ms, 0.072150 us/d` |
 | `1-2` | `114.889 ms, 0.114889 us/d` |
 | `1-4` | `155.712 ms, 0.155712 us/d` |
 | `1-8` | `234.662 ms, 0.234662 us/d` |

### ipc::route

 | PROD-CONS: 1-N | DATAS: Random 2-256bits * 100000 |
 | ------ | ------ |
 | `RTT` | `185.862 ms, 1.85862 us/d` |
 | `1-1` | `117.126 ms, 1.17126 us/d` |
 | `1-2` | `174.284 ms, 1.74284 us/d` |
 | `1-4` | `329.550 ms, 3.29550 us/d` |
 | `1-8` | `494.970 ms, 4.94970 us/d` |

### ipc::channel

 | PROD-CONS: N-M | DATAS: Random 2-256bits * 100000 |
 | ------ | ------ |
 | `RTT` | `221.500 ms, 2.21500 us/d` |
 | `1-1` | `141.013 ms, 1.41013 us/d` |
 | `1-2` | `270.092 ms, 2.70092 us/d` |
 | `1-4` | `609.792 ms, 6.09792 us/d` |
 | `1-8` | `968.309 ms, 9.68309 us/d` |
 | `2-1` | `313.916 ms, 1.56958 us/d` |
 | `4-1` | `804.254 ms, 2.01064 us/d` |
 | `8-1` | `1800.42 ms, 2.25053 us/d` |
 | `2-2` | `550.972 ms, 2.75486 us/d` |
 | `4-4` | `1942.46 ms, 4.85616 us/d` |
 | `8-8` | `7684.91 ms, 9.60614 us/d` |

## Reference

 * [http://www.drdobbs.com/lock-free-data-structures/184401865](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [http://www.cnblogs.com/gaochundong/p/lock_free_programming.html](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)