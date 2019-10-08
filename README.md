### Shadesmar

An IPC library that uses the system's shared memory to pass messages. 
The communication paradigm is publish-subscibe similar to ROS and ROS2.
The library was built to be used within [Project MANAS](www.projectmanas.in).

#### Usage

Publisher:
```c++
#include <shadesmar/publisher.h>

int main() {
    shm::Publisher<int, 16 /* buffer size */ > pub("topic_name");
    
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

void callback(const std::shared_ptr<int>& msg) {
    std::cout << *msg << std::endl;
}

int main() {
    shm::Subscriber<int, 16 /* buffer size */ > sub("topic_name", callback);
    
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