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

 | Environment | Value |
 | ------ | ------ |
 | CPU | AMD Ryzen™ 5 1400 Quad-Core |
 | RAM | 32 GB |
 | OS | Windows 10 Pro for Education x64 |
 | Compiler | MSVC 2017 15.9.4 x64 |

UT & benchmark test function, see: [test](test)

### ipc::circ::queue

 | PROD-CONS: 1-N | DATAS: 12bits * 1000000 |
 | ------ | ------ |
 | `1-1` | `47.4050 ms, 0.047405 us/d` |
 | `1-2` | `113.793 ms, 0.113793 us/d` |
 | `1-4` | `319.196 ms, 0.319196 us/d` |
 | `1-8` | `225.258 ms, 0.225258 us/d` |

### ipc::route

 | PROD-CONS: 1-N | DATAS: Random 2-256bits * 100000 |
 | ------ | ------ |
 | `RTT` | `103.280 ms, 1.03280 us/d` |
 | `1-1` | `78.6670 ms, 0.78667 us/d` |
 | `1-2` | `165.601 ms, 1.65601 us/d` |
 | `1-4` | `223.183 ms, 2.23183 us/d` |
 | `1-8` | `246.161 ms, 2.46161 us/d` |

### ipc::channel

 | PROD-CONS: N-M | DATAS: Random 2-256bits * 100000 |
 | ------ | ------ |
 | `RTT` | `184.711 ms, 1.84711 us/d` |
 | `1-1` | `122.186 ms, 1.22186 us/d` |
 | `1-2` | `226.518 ms, 2.26518 us/d` |
 | `1-4` | `369.239 ms, 3.69239 us/d` |
 | `1-8` | `620.199 ms, 6.20199 us/d` |
 | `2-1` | `287.960 ms, 1.43980 us/d` |
 | `4-1` | `542.050 ms, 1.35512 us/d` |
 | `8-1` | `1406.61 ms, 1.75826 us/d` |
 | `2-2` | `475.095 ms, 2.37547 us/d` |
 | `4-4` | `1455.05 ms, 3.63763 us/d` |
 | `8-8` | `5485.06 ms, 6.85633 us/d` |

## Reference

 * [http://www.drdobbs.com/lock-free-data-structures/184401865](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [http://www.cnblogs.com/gaochundong/p/lock_free_programming.html](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)
