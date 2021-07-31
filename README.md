### Shadesmar

![C/C++ CI](https://github.com/Squadrick/shadesmar/workflows/C/C++%20CI/badge.svg)

[Soulcast](https://stormlightarchive.fandom.com/wiki/Soulcasting) [hoid](https://stormlightarchive.fandom.com/wiki/Hoid) pointers.

An IPC library that uses the system's shared memory to pass messages. 
The communication paradigm is either publish-subscibe or RPC.

Caution: Pre-alpha software.


#### Features

* Multiple subscribers and publishers.

* Uses a circular buffer to pass messages between processes.

* Faster than using the network stack.

* Decentralized, without [resource starvation](https://squadrick.github.io/journal/ipc-locks.html).

---

#### Publish-Subscribe

Publisher:
```c++
#include <shadesmar/memory/copier.h>
#include <shadesmar/pubsub/publisher.h>

int main() {
    shm::memory::DefaultCopier cpy;
    shm::pubsub::Publisher pub("topic_name", &cpy);
    const uint32_t data_size = 1024;
    void *data = malloc(data_size);
    
    for (int i = 0; i < 1000; ++i) {
        p.publish(msg, data_size);
    }
}
```

Subscriber:
```c++
#include <shadesmar/memory/copier.h>
#include <shadesmar/pubsub/subscriber.h>

void callback(shm::memory::Memblock *msg) {
  // `msg->ptr` to access `data`
  // `msg->size` to access `size`

  // The memory will be free'd at the end of this callback.
  // Copy to another memory location if you want to persist the data.
  // Alternatively, if you want to avoid the copy, you can call
  // `msg->no_delete()` which prevents the memory from being deleted
  // at the end of the callback.
}

int main() {
    shm::memory::DefaultCopier cpy;
    shm::pubsub::Subscriber sub("topic_name", &cpy, callback);

    // Using `spinOnce` with a manual loop
    while(true) {
        sub.spin_once();
    }
    // OR
    // Using `spin`
    sub.spin();
}
```
