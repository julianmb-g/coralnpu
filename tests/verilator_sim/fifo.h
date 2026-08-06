// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TESTS_VERILATOR_SIM_FIFO_H_
#define TESTS_VERILATOR_SIM_FIFO_H_

#include <cstdlib>
#include <vector>

// A SystemC CRT transaction queue.

template <typename T> class Fifo {
public:
  bool Empty() { return entries_.empty(); }

  void Write(T v) { entries_.emplace_back(v); }

  bool Read(T &v) {
    if (entries_.empty())
      return false;
    v = entries_.at(0);
    entries_.erase(entries_.begin());
    return true;
  }

  bool Next(T &v, int index = 0) {
    if (index >= Count())
      return false;
    v = entries_.at(index);
    return true;
  }

  bool Rand(T &v) {
    if (entries_.empty())
      return false;
    int index = ::rand() % Count();
    v = entries_.at(index);
    return true;
  }

  void Clear() { entries_.clear(); }

  bool Remove(int index = 0) {
    if (index >= Count())
      return false;
    entries_.erase(entries_.begin() + index);
    return true;
  }

  void Shuffle() {
    const int count = entries_.size();
    if (count < 2)
      return;
    for (int i = 0; i < count; ++i) {
      const int index = ::rand() % count;
      T v = entries_.at(index);
      entries_.erase(entries_.begin() + index);
      entries_.emplace_back(v);
    }
  }

  int Count() { return entries_.size(); }

private:
  std::vector<T> entries_;
};

#endif // TESTS_VERILATOR_SIM_FIFO_H_
