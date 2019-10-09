//
// Created by squadrick on 24/8/19.
//

#ifndef shadesmar_TMP_H
#define shadesmar_TMP_H

#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <random>
#include <string>

#include <experimental/filesystem>

namespace shm::tmp {
std::string const default_chars =
    "abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

std::string const tmp_prefix = "/tmp/shm/";

std::string random_string(size_t len = 15,
                          std::string const &allowed_chars = default_chars) {
  std::mt19937_64 gen{std::random_device()()};
  std::uniform_int_distribution<size_t> dist{0, allowed_chars.length() - 1};
  std::string ret;
  std::generate_n(std::back_inserter(ret), len,
                  [&] { return allowed_chars[dist(gen)]; });
  return ret;
}

inline bool file_exists(const std::string &file_name) {
  // POSIX only
  struct stat buffer {};
  return (stat(file_name.c_str(), &buffer) == 0);
}

void write_topic(const std::string &topic) {
  if (!file_exists(tmp_prefix)) {
    std::experimental::filesystem::create_directories(tmp_prefix);
  }
  std::fstream file;
  std::string file_name = tmp_prefix + random_string();

  file.open(file_name, std::ios::out);

  file << topic.c_str() << std::endl;
  file.close();
}

std::vector<std::string> get_topics() {
  std::vector<std::string> topic_names;
  if (!file_exists(tmp_prefix)) {
    return topic_names;
  }

  for (const auto &entry :
       std::experimental::filesystem::directory_iterator(tmp_prefix)) {
    std::fstream file;
    file.open(entry.path().generic_string(), std::ios::in);
    std::string topic_name;
    file >> topic_name;
    topic_names.push_back(topic_name);
  }

  return topic_names;
}

bool exists(const std::string &topic) {
  auto existing_topics = get_topics();
  for (auto &existing_topic : existing_topics) {
    if (existing_topic == topic) {
      return true;
    }
  }
  return false;
}

void delete_topics() { std::experimental::filesystem::remove_all(tmp_prefix); }
}  // namespace shm::tmp
#endif  // shadesmar_TMP_H
