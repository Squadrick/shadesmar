### Shadesmar

[Soulcast](https://stormlightarchive.fandom.com/wiki/Soulcasting) [hoid](https://stormlightarchive.fandom.com/wiki/Hoid) pointers.

An IPC library that uses the system's shared memory to pass messages. 
The communication paradigm is either publish-subscibe or RPC similar to ROS and ROS2.
The library was built to be used within [Project MANAS](http://projectmanas.in/).

Required packages: Boost, Msgpack


#### Features

* Multiple subscribers and publishers.

* Multithreaded RPC support.

* Uses a circular buffer to pass messages between processes.

* Faster than using the network stack like in the case with ROS.

* Read and write directly from GPU memory to shared memory.

* Decentralized, without [resource starvation](https://squadrick.github.io/journal/ipc-locks.html).

* Allows for both serialized message passing (using `msgpack`) and to 
pass raw bytes.

* No need to define external IDL files for messages. Use C++ classes as
message definition.

---

#### Publish-Subscribe (serialized messages)

Message Definition (`custom_message.h`):
```c++
#include <shadesmar/message.h>

class InnerMessage : public shm::BaseMsg {
  public:
    int inner_val{};
    std::string inner_str{};
    SHM_PACK(inner_val, inner_str);

    InnerMessage() = default;
};

class CustomMessage : public shm::BaseMsg {
  public:
    int val{};
    std::vector<int> arr;
    InnerMessage im;
    SHM_PACK(val, arr, im);

    explicit CustomMessage(int n) {
      val = n;
      for (int i = 0; i < 1000; ++i) {
        arr.push_back(val);
      }
    }

    // MUST BE INCLUDED
    CustomMessage() = default;
};
```

Publisher:
```c++
#include <shadesmar/pubsub/publisher.h>
#include <custom_message.h>

int main() {
    shm::pubsub::Publisher<CustomMessage, 16 /* buffer size */ > pub("topic_name");

    CustomMessage msg;
    msg.val = 0;
    
    for (int i = 0; i < 1000; ++i) {
        msg.init_time(shm::SYSTEM); // add system time as the timestamp
        p.publish(msg);
        msg.val++;
    }
}   
```

Subscriber:
```c++
#include <iostream>
#include <shadesmar/pubsub/subscriber.h>
#include <custom_message.h>

void callback(const std::shared_ptr<CustomMessage>& msg) {
    std::cout << msg->val << std::endl;
}

int main() {
    shm::pubsub::Subscriber<CustomMessage, 16 /* buffer size */ > sub("topic_name", callback);
    
    // Using `spinOnce` with a manual loop
    while(true) {
        sub.spinOnce();
    }
    // OR
    // Using `spin`
    sub.spin();
}
```

---

#### Publish-Subscribe (raw bytes)

Publisher:
```c++
#include <shadesmar/memory/copier.h>
#include <shadesmar/pubsub/publisher.h>

int main() {
    shm::memory::DefaultCopier cpy;
    shm::pubsub::PublisherBin<16 /* buffer size */ > pub("topic_name", &cpy);
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

void callback(shm::memory::Ptr *msg) {
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
    shm::pubsub::SubscriberBin<16 /* buffer size */ > sub("topic_name", &cpy, callback);
    
    // Using `spinOnce` with a manual loop
    while(true) {
        sub.spinOnce();
    }
    // OR
    // Using `spin`
    sub.spin();
}
```

---

#### RPC

Server:
```c++
#include <shadesmar/rpc/server.h>

int add(int a, int b) {
  return a + b;
}

int main() {
  shm::rpc::Function<int(int, int)> rpc_fn("add_fn", add);

  while (true) {
    rpc_fn.serve_once();
  }

  // OR...

  rpc_fn.serve();
}
```

Client:
```c++
#include <shadesmar/rpc/client.h>

int main() {
  shm::rpc::FunctionCaller rpc_fn("add_fn");

  std::cout << rpc_fn(4, 5).as<int>() << std::endl;
}
```

---

**Note**: 

* `shm::pubsub::Subscriber` has a boolean parameter called `extra_copy`. `extra_copy=true` is faster for smaller (<1MB) messages, and `extra_copy=false` is faster for larger (>1MB) messages. For message of 10MB, the throughput for `extra_copy=false` is nearly 50% more than `extra_copy=true`. See `_read_with_copy()` and `_read_without_copy()` in `include/shadesmar/pubsub/topic.h` for more information.

* `queue_size` must be powers of 2. This is due to the underlying shared memory allocator which uses a red-black tree. See `include/shadesmar/memory/allocator.h` for more information.

* You may get this error while publishing: `Increase max_buffer_size`. This occurs when the default memory allocated to the topic buffer cannot store all the messages. The default buffer size for every topic is 256MB. You can access and modify `shm::memory::max_buffer_size`. The value must be set before creating a publisher.
