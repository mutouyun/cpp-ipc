# cpp-ipc(libipc) - C++ IPC Library

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/mutouyun/cpp-ipc/blob/master/LICENSE)
[![Build Status](https://github.com/mutouyun/cpp-ipc/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/mutouyun/cpp-ipc/actions)
[![CodeCov](https://codecov.io/github/mutouyun/cpp-ipc/graph/badge.svg?token=MNOAOLNELH)](https://codecov.io/github/mutouyun/cpp-ipc)
[![Vcpkg package](https://img.shields.io/badge/Vcpkg-package-blueviolet)](https://github.com/microsoft/vcpkg/tree/master/ports/cpp-ipc)
 
A high-performance inter-process communication using shared memory on Linux/Windows.  
使用共享内存的跨平台（Linux/Windows，x86/x64/ARM）高性能IPC通讯库。

---

* 【**重构中……**】

- [ ] 重构代码结构：
    - [x] imp - 基础库
    - [x] pmr - 内存管理
    - [x] concur - 并发
    - [ ] sock - 简单的 socket 抽象
    - [ ] te - 类型擦除
- [ ] IPC：实现基本组件
    - [x] 共享内存（需要作为后续组件的基础）
    - [x] 原子锁
    - [ ] 互斥量
    - [ ] 条件变量
    - [ ] 信号量
    - [ ] 事件（支持 I/O 多路复用）
- [ ] IPC：实现主体功能
    - [ ] 基于共享内存的变长循环内存池（作为大对象的底层存储）
    - [ ] 节点间的相互发现机制（基于 sock，避免连接）
    - [ ] 实现单对单、单对多、多对多收发模型
- [ ] IPC：API/性能优化
    - [ ] 支持零拷贝发送
    - [ ] 支持多路读取等待
    - [ ] 优化无锁队列实现