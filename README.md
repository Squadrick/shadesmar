### Shadesmar

An IPC library that uses the system's shared memory to pass messages. 
The communication paradigm is publish-subscibe similar to ROS and ROS2.
The library was built to be used within [Project MANAS](www.projectmanas.in).

#### Usage

Publisher:
```c++
#include <shadesmar/publisher.h>

int main() {
    shm::Publisher<int> pub("topic_name", 10);
    
    int msg = 0;
    
    for (int i = 0; i < 1000; ++i) {
        p.publish(msg);
        ++msg;
    }
}   
```

Subscriber:
```c++
#include <iostream>

#include <shadesmar/subscriber.h>

void callback(std::shared_ptr<int> msg) {
    std::cout << *msg << std::endl;
}

int main() {
    shm::Subscriber<int> sub("topic_name", callback);
    
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

* Can operate on raw C++ types, or serialization libraries like Cap'n Proto

* Support for type reflection if using serialization libraries