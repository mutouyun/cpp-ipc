# cpp-ipc(libipc) - C++ IPC Library

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mutouyun/cpp-ipc/blob/master/LICENSE) 
[![Build Status](https://github.com/mutouyun/cpp-ipc/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/mutouyun/cpp-ipc/actions)
[![Build status](https://ci.appveyor.com/api/projects/status/github/mutouyun/cpp-ipc?branch=master&svg=true)](https://ci.appveyor.com/project/mutouyun/cpp-ipc)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/cpp-ipc)
 
A high-performance inter-process communication using shared memory on Linux/Windows.  
使用共享内存的跨平台（Linux/Windows，x86/x64/ARM）高性能IPC通讯库。
 
 * 推荐支持C++17的编译器（msvc-2017/gcc-7/clang-4）
 * 除STL外，无其他依赖
 * 无锁（lock-free）或轻量级spin-lock
 * 底层数据结构为循环数组（circular array）
 * `ipc::route`支持单写多读，`ipc::channel`支持多写多读【**注意：目前同一条通道最多支持32个receiver，sender无限制**】
 * 默认采用广播模式收发数据，支持用户任意选择读写方案
 * 不会长时间忙等（重试一定次数后会使用信号量进行等待），支持超时
 * 支持[Vcpkg](https://github.com/microsoft/vcpkg/blob/master/README_zh_CN.md)方式安装，如`vcpkg install cpp-ipc`
 
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

UT & benchmark test function: [test](test)  
Performance data: [performance.xlsx](performance.xlsx)

## Reference

 * [Lock-Free Data Structures | Dr Dobb's](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [Yet another implementation of a lock-free circular array queue | CodeProject](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [Lock-Free 编程 | 匠心十年 - 博客园](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)
 * [无锁队列的实现 | 酷 壳 - CoolShell](https://coolshell.cn/articles/8239.html)
 * [Implementing Condition Variables with Semaphores](https://www.microsoft.com/en-us/research/wp-content/uploads/2004/12/ImplementingCVs.pdf)
