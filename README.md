# cpp-ipc (libipc) - C++ IPC Library

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mutouyun/cpp-ipc/blob/master/LICENSE)
[![Build Status](https://github.com/mutouyun/cpp-ipc/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/mutouyun/cpp-ipc/actions)
[![Build status](https://ci.appveyor.com/api/projects/status/github/mutouyun/cpp-ipc?branch=master&svg=true)](https://ci.appveyor.com/project/mutouyun/cpp-ipc)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/cpp-ipc)

## A high-performance inter-process communication library using shared memory on Linux/Windows.

 * Compilers with C++17 support are recommended (msvc-2017/gcc-7/clang-4)
 * No other dependencies except STL.
 * Only lock-free or lightweight spin-lock is used.
 * Circular array is used as the underline data structure.
 * `ipc::route` supports single write and multiple read. `ipc::channel` supports multiple read and write. (**Note: currently, a channel supports up to 32 receivers, but there is no such a limit for the sender.**) 
 * Broadcasting is used by default, but user can choose any read/ write combinations.
 * No long time blind wait. (Semaphore will be used after a certain number of retries.) 
 * [Vcpkg](https://github.com/microsoft/vcpkg/blob/master/README.md) way of installation is supported. E.g. `vcpkg install cpp-ipc`

## Usage

See: [Wiki](https://github.com/mutouyun/cpp-ipc/wiki)

## Performance

 Environment | Value
 ------ | ------
 Device | Lenovo ThinkPad T450
 CPU | Intel® Core™ i5-4300U @ 2.5 GHz
 RAM | 16 GB
 OS | Windows 7 Ultimate x64
 Compiler | MSVC 2017 15.9.4

Unit & benchmark tests: [test](test)  
Performance data: [performance.xlsx](performance.xlsx)

## Reference

 * [Lock-Free Data Structures | Dr Dobb's](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [Yet another implementation of a lock-free circular array queue | CodeProject](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [Lock-Free 编程 | 匠心十年 - 博客园](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)
 * [无锁队列的实现 | 酷 壳 - CoolShell](https://coolshell.cn/articles/8239.html)
 * [Implementing Condition Variables with Semaphores](https://www.microsoft.com/en-us/research/wp-content/uploads/2004/12/ImplementingCVs.pdf)

------


## 使用共享内存的跨平台（Linux/Windows，x86/x64/ARM）高性能IPC通讯库

 * 推荐支持C++17的编译器（msvc-2017/gcc-7/clang-4）
 * 除STL外，无其他依赖
 * 无锁（lock-free）或轻量级spin-lock
 * 底层数据结构为循环数组（circular array）
 * `ipc::route`支持单写多读，`ipc::channel`支持多写多读【**注意：目前同一条通道最多支持32个receiver，sender无限制**】
 * 默认采用广播模式收发数据，支持用户任意选择读写方案
 * 不会长时间忙等（重试一定次数后会使用信号量进行等待），支持超时
 * 支持[Vcpkg](https://github.com/microsoft/vcpkg/blob/master/README_zh_CN.md)方式安装，如`vcpkg install cpp-ipc`

## 使用方法

详见：[Wiki](https://github.com/mutouyun/cpp-ipc/wiki)

## 性能

| 环境     | 值                               |
| -------- | -------------------------------- |
| 设备     | 联想 ThinkPad T450               |
| CPU      | 英特尔® Core™ i5-4300U @ 2.5 GHz |
| 内存     | 16 GB                            |
| 操作系统 | Windows 7 Ultimate x64           |
| 编译器   | MSVC 2017 15.9.4                 |

单元测试和Benchmark测试: [test](test)  
性能数据: [performance.xlsx](performance.xlsx)

## 参考

 * [Lock-Free Data Structures | Dr Dobb's](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [Yet another implementation of a lock-free circular array queue | CodeProject](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [Lock-Free 编程 | 匠心十年 - 博客园](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)
 * [无锁队列的实现 | 酷 壳 - CoolShell](https://coolshell.cn/articles/8239.html)
 * [Implementing Condition Variables with Semaphores](https://www.microsoft.com/en-us/research/wp-content/uploads/2004/12/ImplementingCVs.pdf)
