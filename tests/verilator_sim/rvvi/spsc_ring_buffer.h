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

#ifndef TESTS_VERILATOR_SIM_RVVI_SPSC_RING_BUFFER_H_
#define TESTS_VERILATOR_SIM_RVVI_SPSC_RING_BUFFER_H_

#include "trace_packet.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <thread>

namespace mpact::sim::riscv::rvvi {

template <typename T = TracePacket, size_t Size = 4096> class SpscRingBuffer {
  static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

public:
  SpscRingBuffer() : head_(0), tail_(0) {}

  bool Push(const T &item) {
    size_t head = head_.load(std::memory_order_relaxed);
    size_t tail = tail_.load(std::memory_order_acquire);
    if (head - tail >= Size) {
      return false; // Full
    }
    buffer_[head & (Size - 1)] = item;
    head_.store(head + 1, std::memory_order_release);
    return true;
  }

  bool Pop(T &item) {
    size_t tail = tail_.load(std::memory_order_relaxed);
    size_t head = head_.load(std::memory_order_acquire);
    if (head == tail) {
      return false; // Empty
    }
    item = buffer_[tail & (Size - 1)];
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

  bool IsEmpty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }

  void WaitUntilEmpty() const {
    while (!IsEmpty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void Flush() { WaitUntilEmpty(); }

private:
  std::array<T, Size> buffer_;
  alignas(64) std::atomic<size_t> head_;
  alignas(64) std::atomic<size_t> tail_;
  char padding_[64 - sizeof(std::atomic<size_t>)]; // Ensure tail_ is padded at
                                                   // the end.
};

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_SPSC_RING_BUFFER_H_
