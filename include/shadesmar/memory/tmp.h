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

#ifndef INCLUDE_SHADESMAR_MEMORY_TMP_H_
#define INCLUDE_SHADESMAR_MEMORY_TMP_H_

#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "shadesmar/memory/filesystem.h"

namespace shm::memory::tmp {
char const default_chars[] =
    "abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

char const tmp_prefix[] = "/tmp/shm/";

inline std::string random_string(size_t len = 15) {
  std::mt19937_64 gen{std::random_device()()};
  std::uniform_int_distribution<size_t> dist{0, sizeof(default_chars) - 1};
  std::string ret;
  std::generate_n(std::back_inserter(ret), len,
                  [&] { return default_chars[dist(gen)]; });
  return ret;
}

inline bool file_exists(const std::string &file_name) {
  // POSIX only
  struct stat buffer {};
  return (stat(file_name.c_str(), &buffer) == 0);
}

inline void write(const std::string &name) {
  if (!file_exists(tmp_prefix)) {
    std::filesystem::create_directories(tmp_prefix);
  }
  std::fstream file;
  std::string file_name = tmp_prefix + random_string();
  file.open(file_name, std::ios::out);

  file << name.c_str() << std::endl;
  file.close();
}

inline std::vector<std::string> get_tmp_names() {
  std::vector<std::string> names{};
  if (!file_exists(tmp_prefix)) {
    return names;
  }

  for (const auto &entry : std::filesystem::directory_iterator(tmp_prefix)) {
    std::fstream file;
    file.open(entry.path().generic_string(), std::ios::in);
    std::string name;
    file >> name;
    names.push_back(name);
  }

  return names;
}

inline bool exists(const std::string &name) {
  auto existing_names = get_tmp_names();
  for (auto &existing_name : existing_names) {
    if (existing_name == name) {
      return true;
    }
  }
  return false;
}

inline void delete_topics() { std::filesystem::remove_all(tmp_prefix); }
}  // namespace shm::memory::tmp
#endif  // INCLUDE_SHADESMAR_MEMORY_TMP_H_
