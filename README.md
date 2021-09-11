### Shadesmar

![C/C++ CI](https://github.com/Squadrick/shadesmar/workflows/C/C++%20CI/badge.svg)

An IPC library that uses the system's shared memory to pass messages. Supports
publish-subscribe and RPC (under development).

Caution: Pre-alpha software. Linux only.

#### Features

* Multiple subscribers and publishers.

* Uses a circular buffer to pass messages between processes.

* Faster than using the network stack. High throughput, low latency for large
  messages.

* Decentralized, without [resource starvation](https://squadrick.github.io/journal/ipc-locks.html).

* Minimize or optimize data movement using custom copiers.

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
