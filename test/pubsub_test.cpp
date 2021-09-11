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

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/pubsub/publisher.h"
#include "shadesmar/pubsub/subscriber.h"
#endif

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("single_message") {
  std::string topic = "single_message";

  int message = 3;
  shm::pubsub::Publisher pub(topic);

  int answer;
  auto callback = [&answer](shm::memory::Memblock *memblock) {
    answer = *(reinterpret_cast<int *>(memblock->ptr));
  };
  shm::pubsub::Subscriber sub(topic, callback);

  pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
  sub.spin_once();

  REQUIRE(answer == message);
}

TEST_CASE("multiple_messages") {
  std::string topic = "multiple_messages";

  std::vector<int> messages = {1, 2, 3, 4, 5};
  shm::pubsub::Publisher pub(topic);

  std::vector<int> answers;
  auto callback = [&answers](shm::memory::Memblock *memblock) {
    int answer = *(reinterpret_cast<int *>(memblock->ptr));
    answers.push_back(answer);
  };
  shm::pubsub::Subscriber sub(topic, callback);

  for (int message : messages) {
    std::cout << "Publishing: " << message << std::endl;
    pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
  }
  for (int i = 0; i < messages.size(); ++i) {
    sub.spin_once();
  }

  REQUIRE(answers == messages);
}

TEST_CASE("alternating_pub_sub") {
  std::string topic = "alternating_pub_sub";

  int message = 3;
  shm::pubsub::Publisher pub(topic);

  int answer;
  auto callback = [&answer](shm::memory::Memblock *memblock) {
    answer = *(reinterpret_cast<int *>(memblock->ptr));
  };
  shm::pubsub::Subscriber sub(topic, callback);

  for (int i = 0; i < 10; i++) {
    pub.publish(reinterpret_cast<void *>(&i), sizeof(int));
    sub.spin_once();
    REQUIRE(answer == i);
  }
}

TEST_CASE("single_pub_multiple_sub") {
  std::string topic = "single_pub_multiple_sub";

  std::vector<int> messages = {1, 2, 3, 4, 5};
  shm::pubsub::Publisher pub(topic);

  int n_subs = 3;
  std::vector<std::vector<int>> vec_answers;
  for (int i = 0; i < n_subs; ++i) {
    std::vector<int> answers;
    vec_answers.push_back(answers);
  }

  std::vector<shm::pubsub::Subscriber> subs;

  for (int i = 0; i < n_subs; i++) {
    auto callback = [idx = i, &vec_answers](shm::memory::Memblock *memblock) {
      int answer = *(reinterpret_cast<int *>(memblock->ptr));
      vec_answers[idx].push_back(answer);
    };
    shm::pubsub::Subscriber sub(topic, callback);
    subs.push_back(std::move(sub));
  }

  for (int message : messages) {
    std::cout << "Publishing: " << message << std::endl;
    pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
  }

  SECTION("messages by subscribers") {
    for (int i = 0; i < messages.size(); ++i) {
      std::cout << "Subscribe message: " << i << std::endl;
      for (int s = 0; s < n_subs; ++s) {
        std::cout << "Subscriber: " << s << std::endl;
        subs[s].spin_once();
      }
    }
  }

  SECTION("subscribers by messages") {
    for (int s = 0; s < n_subs; ++s) {
      std::cout << "Subscriber: " << s << std::endl;
      for (int i = 0; i < messages.size(); ++i) {
        std::cout << "Subscribe message: " << i << std::endl;
        subs[s].spin_once();
      }
    }
  }

  for (int i = 0; i < n_subs; ++i) {
    REQUIRE(vec_answers[i] == messages);
  }
}

TEST_CASE("multiple_pub_single_sub") {
  std::string topic = "multiple_pub_single_sub";

  int n_pubs = 3;
  int n_messages = 5;

  std::vector<shm::pubsub::Publisher> pubs;
  for (int i = 0; i < n_pubs; ++i) {
    shm::pubsub::Publisher pub(topic);
    pubs.push_back(std::move(pub));
  }

  std::vector<int> answers;
  auto callback = [&answers](shm::memory::Memblock *memblock) {
    int answer = *(reinterpret_cast<int *>(memblock->ptr));
    answers.push_back(answer);
  };

  SECTION("messages by publishers") {
    int msg = 1;
    for (int m = 0; m < n_messages; ++m) {
      for (int p = 0; p < n_pubs; ++p) {
        pubs[p].publish(reinterpret_cast<void *>(&msg), sizeof(int));
        msg++;
      }
    }
  }

  SECTION("publishers by messages") {
    int msg = 1;
    for (int p = 0; p < n_pubs; ++p) {
      for (int m = 0; m < n_messages; ++m) {
        pubs[p].publish(reinterpret_cast<void *>(&msg), sizeof(int));
        msg++;
      }
    }
  }

  shm::pubsub::Subscriber sub(topic, callback);
  std::vector<int> expected;
  for (int i = 0; i < n_messages * n_pubs; ++i) {
    sub.spin_once();
    expected.push_back(i + 1);
  }

  REQUIRE(expected == answers);
}

TEST_CASE("multiple_pub_multiple_sub") {
  std::string topic = "multiple_pub_multiple_sub";

  int n_pubs = 3, n_subs = 3, n_messages = 5;
  REQUIRE(n_pubs == n_subs);

  std::vector<shm::pubsub::Publisher> pubs;
  for (int i = 0; i < n_pubs; ++i) {
    shm::pubsub::Publisher pub(topic);
    pubs.push_back(std::move(pub));
  }

  std::vector<int> messages;
  int msg = 1;
  for (int p = 0; p < n_pubs; ++p) {
    for (int m = 0; m < n_messages; ++m) {
      messages.push_back(msg);
      pubs[p].publish(reinterpret_cast<void *>(&msg), sizeof(int));
      msg++;
    }
  }

  std::vector<std::vector<int>> vec_answers;
  for (int i = 0; i < n_subs; ++i) {
    std::vector<int> answers;
    vec_answers.push_back(answers);
  }

  std::vector<shm::pubsub::Subscriber> subs;

  for (int i = 0; i < n_subs; i++) {
    auto callback = [idx = i, &vec_answers](shm::memory::Memblock *memblock) {
      int answer = *(reinterpret_cast<int *>(memblock->ptr));
      vec_answers[idx].push_back(answer);
    };
    shm::pubsub::Subscriber sub(topic, callback);
    subs.push_back(std::move(sub));
  }

  for (int s = 0; s < n_subs; ++s) {
    for (int i = 0; i < messages.size(); ++i) {
      subs[s].spin_once();
    }
  }

  for (int i = 0; i < n_subs; ++i) {
    REQUIRE(vec_answers[i] == messages);
  }
}

TEST_CASE("spin_without_new_msg") {
  std::string topic = "spin_without_new_msg";

  int message = 3;
  shm::pubsub::Publisher pub(topic);

  int count = 0, answer;
  auto callback = [&count, &answer](shm::memory::Memblock *memblock) {
    answer = *(reinterpret_cast<int *>(memblock->ptr));
    ++count;
  };
  shm::pubsub::Subscriber sub(topic, callback);

  pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
  sub.spin_once();
  REQUIRE(answer == 3);
  REQUIRE(count == 1);
  ++message;
  sub.spin_once();
  REQUIRE(count == 1);
  ++message;
  pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
  sub.spin_once();
  REQUIRE(answer == 5);
  REQUIRE(count == 2);
}

TEST_CASE("sub_counter_jump") {
  std::string topic = "sub_counter_jump";
  shm::pubsub::Publisher pub(topic);

  int answer;
  auto callback = [&answer](shm::memory::Memblock *memblock) {
    answer = *(reinterpret_cast<int *>(memblock->ptr));
  };
  shm::pubsub::Subscriber sub(topic, callback);

  for (int i = 0; i < shm::memory::QUEUE_SIZE; ++i) {
    pub.publish(reinterpret_cast<void *>(&i), sizeof(int));
  }

  sub.spin_once();
  int lookback =
      shm::pubsub::jumpahead(shm::memory::QUEUE_SIZE, shm::memory::QUEUE_SIZE);
  REQUIRE(answer == lookback);

  int moveahead = shm::memory::QUEUE_SIZE - lookback + 1;
  for (int i = 0; i < moveahead; ++i) {
    int message = lookback + i;
    pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
  }

  sub.spin_once();
  REQUIRE(answer == moveahead);
}

TEST_CASE("sub_after_pub_dtor") {
  std::string topic = "sub_after_pub_dtor";

  std::vector<int> messages = {1, 2, 3, 4, 5};
  std::vector<int> answers;

  {
    shm::pubsub::Publisher pub(topic);
    for (int message : messages) {
      std::cout << "Publishing: " << message << std::endl;
      pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
    }
  }

  {
    auto callback = [&answers](shm::memory::Memblock *memblock) {
      int answer = *(reinterpret_cast<int *>(memblock->ptr));
      answers.push_back(answer);
    };
    shm::pubsub::Subscriber sub(topic, callback);

    for (int i = 0; i < messages.size(); ++i) {
      sub.spin_once();
    }
  }

  REQUIRE(answers == messages);
}

TEST_CASE("spin_on_thread") {
  std::string topic = "spin_on_thread";

  std::vector<int> messages = {1, 2, 3, 4, 5};
  std::vector<int> answers;
  std::mutex mu;

  auto callback = [&answers, &mu](shm::memory::Memblock *memblock) {
    std::unique_lock<std::mutex> lock(mu);
    int answer = *(reinterpret_cast<int *>(memblock->ptr));
    answers.push_back(answer);
  };

  shm::pubsub::Publisher pub(topic);
  for (int message : messages) {
    std::cout << "Publishing: " << message << std::endl;
    pub.publish(reinterpret_cast<void *>(&message), sizeof(int));
  }

  shm::pubsub::Subscriber sub(topic, callback);
  std::thread th([&]() { sub.spin(); });
  while (true) {
    std::unique_lock<std::mutex> lock(mu);
    if (answers.size() == messages.size()) {
      break;
    }
  }

  sub.stop();
  th.join();

  REQUIRE(answers == messages);
}

// TODO(squadricK): Add tests
// - All the multiple pub/sub tests with parallelism
