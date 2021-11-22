### Shadesmar

![C/C++ CI](https://github.com/Squadrick/shadesmar/workflows/C/C++%20CI/badge.svg)

An IPC library that uses the system's shared memory to pass messages. Supports
publish-subscribe and RPC.

Requires: Linux and x86.
Caution: Alpha software.

#### Features

* Multiple subscribers and publishers.

* Uses a circular buffer to pass messages between processes.

* Faster than using the network stack. High throughput, low latency for large
  messages.

* Decentralized, without [resource starvation](https://squadrick.github.io/journal/ipc-locks.html).

* Minimize or optimize data movement using custom copiers.

#### Usage

There's a single header file generated from the source code which can be
found [here](https://raw.githubusercontent.com/Squadrick/shadesmar/master/release/shadesmar.h).

If you want to generate the single header file yourself, clone the repo and run:
```
$ cd shadesmar
$ python3 simul/simul.py
```
This will generate the file in `include/`.

---

#### Publish-Subscribe

Publisher:
```c++
#include <shadesmar/pubsub/publisher.h>

int main() {
    shm::pubsub::Publisher pub("topic_name");
    const uint32_t data_size = 1024;
    void *data = malloc(data_size);
    
    for (int i = 0; i < 1000; ++i) {
        p.publish(data, data_size);
    }
}
```

Subscriber:
```c++
#include <shadesmar/pubsub/subscriber.h>

void callback(shm::memory::Memblock *msg) {
  // `msg->ptr` to access `data`
  // `msg->size` to access `data_size`

  // The memory will be free'd at the end of this callback.
  // Copy to another memory location if you want to persist the data.
  // Alternatively, if you want to avoid the copy, you can call
  // `msg->no_delete()` which prevents the memory from being deleted
  // at the end of the callback.
}

int main() {
    shm::pubsub::Subscriber sub("topic_name", callback);

    // Using `spin_once` with a manual loop
    while(true) {
        sub.spin_once();
    }
    // OR
    // Using `spin`
    sub.spin();
}
```

---

#### RPC

Client:
```c++
#include <shadesmar/rpc/client.h>

int main() {
  Client client("channel_name");
  shm::memory::Memblock req, resp;
  // Populate req.
  client.call(req, &resp);
  // Use resp here.

  // resp needs to be explicitly free'd.
  client.free_resp(&resp);
}
```

Server:

```c++
#include <shadesmar/rpc/server.h>

bool callback(const shm::memory::Memblock &req,
              shm::memory::Memblock *resp) {
  // resp->ptr is a void ptr, resp->size is the size of the buffer.
  // You can allocate memory here, which can be free'd in the clean-up lambda.
  return true;
}

void clean_up(shm::memory::Memblock *resp) {
  // This function is called *after* the callback is finished. Any memory
  // allocated for the response can be free'd here. A different copy of the
  // buffer is sent to the client, this can be safely cleaned.
}

int main() {
  shm::rpc::Server server("channel_name", callback, clean_up);

  // Using `serve_once` with a manual loop
  while(true) {
    server.serve_once();
  }
  // OR
  // Using `serve`
  server.serve();
}
