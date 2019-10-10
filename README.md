### Shadesmar

An IPC library that uses the system's shared memory to pass messages. 
The communication paradigm is publish-subscibe similar to ROS and ROS2.
The library was built to be used within [Project MANAS](www.projectmanas.in).

#### Usage

Message Definition (`custom_message.h`):
```c++
#include <shadesmar/messages.h>

class InnerMessage : public shm::msg::BaseMsg {
  public:
    int inner_val{};
    std::string inner_str{};
    SHM_PACK(inner_val, inner_str);

    InnerMessage() = default;
};

class CustomMessage : public shm::msg::BaseMsg {
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
        msg.init_time(shm::msg::SYSTEM); // add system time as the timestamp
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
    
    // Using `spin` to execute asynchronously
    sub.spin();
}
```

#### Features

* Multiple subscribers and publishers

* Faster than using the network stack like in the case with ROS

* Read and write directly from GPU memory to shared memory

* Subscribers can operate directly on messages in shared memory without making copies

* Uses a circular buffer to pass messages between publishers and subscribers

* Minimal usage of locks, and uses sharable locks where possible

* Decentralized, without resource starvation
