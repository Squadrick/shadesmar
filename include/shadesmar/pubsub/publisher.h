/* MIT License

Copyright (c) 2020 Dheeraj R Reddy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==============================================================================*/

#ifndef INCLUDE_SHADESMAR_PUBSUB_PUBLISHER_H_
#define INCLUDE_SHADESMAR_PUBSUB_PUBLISHER_H_

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "shadesmar/memory/copier.h"
#include "shadesmar/pubsub/topic.h"

namespace shm::pubsub {

class Publisher {
 public:
  explicit Publisher(const std::string &topic_name);
  Publisher(const std::string &topic_name,
            std::shared_ptr<memory::Copier> copier);
  Publisher(const Publisher &) = delete;
  Publisher(Publisher &&);
  bool publish(void *data, size_t size);

 private:
  std::string topic_name_;
  std::unique_ptr<Topic> topic_;
};

Publisher::Publisher(const std::string &topic_name) : topic_name_(topic_name) {
  topic_ = std::make_unique<Topic>(topic_name);
}

Publisher::Publisher(const std::string &topic_name,
                     std::shared_ptr<memory::Copier> copier)
    : topic_name_(topic_name) {
  topic_ = std::make_unique<Topic>(topic_name, copier);
}

Publisher::Publisher(Publisher &&other) {
  topic_name_ = other.topic_name_;
  topic_ = std::move(other.topic_);
}

bool Publisher::publish(void *data, size_t size) {
  memory::Memblock memblock(data, size);
  return topic_->write(memblock);
}

}  // namespace shm::pubsub
#endif  // INCLUDE_SHADESMAR_PUBSUB_PUBLISHER_H_
