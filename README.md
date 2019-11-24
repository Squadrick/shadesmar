### Shadesmar

An IPC library that uses the system's shared memory to pass messages. 
The communication paradigm is publish-subscibe similar to ROS and ROS2.
The library was built to be used within [Project MANAS](www.projectmanas.in).

Required packages: Boost, Msgpack


#### Features

* Multiple subscribers and publishers.

* Faster than using the network stack like in the case with ROS.

* Read and write directly from GPU memory to shared memory.

* Subscribers can operate directly on messages in shared memory without making copies.

* Uses a circular buffer to pass messages between publishers and subscribers.

* Minimal usage of locks, and uses sharable locks where possible. 

* Decentralized, without [resource starvation](https://squadrick.github.io/journal/ipc-locks.html).

* Allows for both serialized message passing (using `msgpack`) and to 
pass raw bytes.

---

#### Usage (serialized messages)

Message Definition (`custom_message.h`):
```c++
#include <shadesmar/messages.h>

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
#include <shadesmar/publisher.h>
#include <custom_message.h>

int main() {
    shm::Publisher<CustomMessage, 16 /* buffer size */ > pub("topic_name");

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
#include <shadesmar/subscriber.h>
#include <custom_message.h>

void callback(const std::shared_ptr<CustomMessage>& msg) {
    std::cout << msg->val << std::endl;
}

int main() {
    shm::Subscriber<CustomMessage, 16 /* buffer size */ > sub("topic_name", callback);
    
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

#### Usage (raw bytes)

Publisher:
```c++
#include <shadesmar/publisher.h>

int main() {
    shm::PublisherBin<16 /* buffer size */ > pub("topic_name");
    const uint32_t data_size = 1024;
    void *data = malloc(data_size);
    
    for (int i = 0; i < 1000; ++i) {
        p.publish(msg, data_size);
    }
}  
```

Subscriber:
```c++
#include <shadesmar/subscriber.h>

void callback(std::unique_ptr<uint8_t[]>& data, size_t data_size) {
  // use `data` here

  // to get the raw underlying pointer, use data.get()
  // the memory will be free'd at the end of this callback
  // copy to another memory location if you want to persist the data
}

int main() {
    shm::SubscriberBin<16 /* buffer size */ > sub("topic_name", callback);
    
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

**Note**: 

* `shm::Subscriber` has a boolean parameter called `extra_copy`. `extra_copy=true` is faster for smaller (<1MB) messages, and `extra_copy=false` is faster for larger (>1MB) messages. For message of 10MB, the throughput for `extra_copy=false` is nearly 50% more than `extra_copy=true`. See `read_with_copy()` and `read_without_copy()` in `include/shadesmar/memory.h` for more information.

* `queue_size` must be powers of 2. This is due to the underlying shared memory allocator which uses a red-black tree. See `include/shadesmar/allocator.h` for more information.

* You may get this error while publishing: `Increase max_buffer_size`. This occurs when the default memory allocated to the topic buffer cannot store all the messages. The default buffer size for every topic is 256MB. You can access and modify `shm::max_buffer_size`. The value must be set before creating a publisher.
