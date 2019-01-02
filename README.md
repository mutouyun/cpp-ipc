# cpp-ipc - C++ IPC Library

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mutouyun/cpp-ipc/blob/master/LICENSE) [![Build Status](https://travis-ci.org/mutouyun/cpp-ipc.svg?branch=master)](https://travis-ci.org/mutouyun/cpp-ipc)
 
A high-performance inter-process communication using shared memory on Linux/Windows.  
使用共享内存的跨平台（Linux/Windows，x86/x64/ARM）高性能IPC通讯库。
 
 * 需要支持C++17的编译器（msvc-2017/gcc-7/clang-4）
 * 除STL外，无其他依赖
 * 无锁（lock-free）或轻量级shared-spin-lock（`ipc::channel::connect`/`disconnect`）
 * 底层数据结构为循环数组（circular array）
 * `ipc::route`支持单生产多消费，`ipc::channel`支持多生产多消费
 
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
Performance data: [performence.xlsx](performence.xlsx)

## Reference

 * [http://www.drdobbs.com/lock-free-data-structures/184401865](http://www.drdobbs.com/lock-free-data-structures/184401865)
 * [https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular](https://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular)
 * [http://www.cnblogs.com/gaochundong/p/lock_free_programming.html](http://www.cnblogs.com/gaochundong/p/lock_free_programming.html)
