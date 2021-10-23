<h1 align="center">
  <br>
  <img src="https://raw.githubusercontent.com/alephzero/logo/master/rendered/alephzero.svg" width="256px">
  <br>
  AlephZero
</h1>

<h3 align="center">Simple, Robust, Fast IPC.</h3>

<p align="center">
  <a href="https://github.com/alephzero/alephzero/actions?query=workflow%3ACI"><img src="https://github.com/alephzero/alephzero/workflows/CI/badge.svg"></a>
  <a href="https://codecov.io/gh/alephzero/alephzero"><img src="https://codecov.io/gh/alephzero/alephzero/branch/master/graph/badge.svg"></a>
  <a href="https://alephzero.readthedocs.io/en/latest/?badge=latest"><img src="https://readthedocs.org/projects/alephzero/badge/?version=latest"></a>
  <a href="http://unlicense.org"><img src="https://img.shields.io/badge/license-Unlicense-blue.svg"></a>
</p>

<p align="center">
  <a href="#overview">Overview</a> •
  <a href="#transport">Transport</a> •
  <a href="#protocol">Protocol</a> •
  <a href="#examples">Examples</a> •
  <a href="#installation">Installation</a> •
  <a href="#across-dockers">Across Dockers</a>
</p>

# Overview

[Presentation from March 25, 2020](https://docs.google.com/presentation/d/12KE9UucjZPtpVnM1NljxOqBolBBKECWJdrCoE2yJaBw/edit#slide=id.p)

AlephZero is a library for message based communication between programs running on the same machine.

## Simple

AlephZero's main goal is to be simple to use. Nothing is higher priority.

There is no "master" process in between your nodes that is needed to do handshakes or exchanges of any kind. All you need is the topic name.

See the <a href="#examples">Examples</a>.

## Robust

This is probably the main value of AlephZero, above similar libraries.

AlephZero uses a lot of tricks to ensure the state of all channels is consistent, even when programs die. This includes double-buffering the state of the communication channel and [robustifying](https://man7.org/linux/man-pages/man3/pthread_mutexattr_setrobust.3.html) the locks and notification channels.

## Fast

AlephZero uses shared memory across multiple processes to read and write messages, minimizing the involvement of the kernel. The kernel only really gets involved in notifying a process that a new message exists, and for that we use futex (fast user-space mutex).

TODO: Benchmarks

# Transport

AlephZero, at its core, is a simple allocator on top of a contiguous region of memory. Usually, shared-memory. The allocator of choice is a circular-linked-list, which is fast, simple, and sufficient for the protocol listed below. It also plays well with the robustness requirement.

This has a number of implications. For one, this means that old messages are kept around until the space is needed. The oldest messages are always discarded before any more recent messages.

# Protocol

Rather than exposing the low-level transport directly, AlephZero provides a few higher level protocol:

* <b>PubSub</b>: Broadcast published messages. Subscribers get notified.
* <b>RPC</b>: Request-response.
* <b>PRPC (Progressive RPC)</b>: Request-streaming response.
* <b>Sessions</b>: Bi-directional channel of communication. Not yet implemented. Let me know if you want this.

# Examples

Many more example and an interactive experience can be found at: https://github.com/alephzero/playground

For the curious, here are some simple snippets to get you started:

To begin with, we need to include AlephZero:
```cc
#include <a0.h>
```

## PubSub

You can have as many publisher and subscribers on the same topic as you wish. They just need to agree on the filename.

```cc
a0::Publisher p("my_pubsub_topic");
p.pub("foo");
```

You just published `"foo"` to the `"my_pubsub_topic"`.

To read those message, you can create a subscriber on the same topic:
```cc
a0::Subscriber sub(
    "my_pubsub_topic",
    A0_INIT_AWAIT_NEW,  // or MOST_RECENT or OLDEST
    A0_ITER_NEWEST,     // or NEXT
    [](a0::PacketView pkt_view) {
      std::cout << "Got: " << pkt_view.payload() << std::endl;
    });
```
The callback will trigger whenever a message is published.

The `Subscriber` object spawns a thread that will read the topic and call the callback.

The `A0_INIT` tells the subscriber where to start reading. 
* `A0_INIT_AWAIT_NEW`: Start with messages published after the creation of the subscriber.
* `A0_INIT_MOST_RECENT`: Start with the most recently published message. Useful for state and configuration. But be careful, this can be quite old!
* `A0_INIT_OLDEST`: Topics keep a history of 16MB (unless configures otherwise). Start with the oldest thing still in there.

The `A0_ITER` tells the subscriber how to continue reading messages. After each callback:
* `A0_ITER_NEXT`: grab the sequentially next message. When you don't want to miss a thing.
* `A0_ITER_NEWEST`: grab the newest available unread message. When you want to keep up with the firehose.

```cc
a0::SubscriberSync sub_sync(
    "my_pubsub_topic",
    A0_INIT_OLDEST, A0_ITER_NEXT);
while (sub_sync.has_next()) {
  auto pkt = sub_sync.next();
  std::cout << "Got: " << pkt.payload() << std::endl;
}
```

## RPC

Create an `RpcServer`:

```cc
a0::RpcServer server(
    "my_rpc_topic",
    /* onrequest = */ [](a0::RpcRequest req) {
        std::cout << "Got: " << req.pkt().payload() << std::endl;
        req.reply("echo " + std::string(req.pkt().payload()));
    },
    /* oncancel = */ nullptr);
```

Create an `RpcClient`:

```cc
a0::RpcClient client("my_rpc_topic");
client.send("client msg", [](a0::PacketView reply) {
  std::cout << "Got: " << reply.payload() << std::endl;
});
```

# Installation

## Install From Source

### Ubuntu Dependencies

```sh
apt install g++ make
```

### Alpine Dependencies

```sh
apk add g++ linux-headers make
```

### Download And Install

```sh
git clone https://github.com/alephzero/alephzero.git
cd alephzero
make install -j
```

## Install From Package

Coming soon-ish. Let me know if you want this and I'll prioritize it. External support is much appreciated.

## Integration

### Command Line

Add the following to g++ / clang commands.
```sh
-L${libdir} -lalephzero -lpthread
```

### Package-cfg

```sh
pkg-config --cflags --libs alephzero
```

### CMake

Coming soon-ish. Let me know if you want this and I'll prioritize it. External support is much appreciated.

### Bazel

Coming soon-ish. Let me know if you want this and I'll prioritize it.

# Across Dockers

For programs running across different dockers to be able to communicate, we need to have them match up on two flags: `--ipc` and `--pid`.

* `--ipc` shares the `/dev/shm` filesystem. This is necessary to open the same file topics.
* `--pid` shares the process id namespace. This is necessary for the locking and notification systems.

In the simplest case, you can set them both to `host` and talk through the system's global `/dev/shm` and process id namespace.
```sh
docker run --ipc=host --pid=host --name=foo foo_image
docker run --ipc=host --pid=host --name=bar bar_image
```

Or, you can mark one as `shareable` and have the others connect to it:
```sh
docker run --ipc=shareable --pid=shareable --name=foo foo_image
docker run --ipc=container:foo --pid=container:foo --name=bar bar_image
```
