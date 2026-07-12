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

#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_packet.h"
#include "gtest/gtest.h"
#include <thread>

namespace coralnpu::sim::rvvi {

class SpscRingBufferTest : public ::testing::Test {
 protected:
  SpscRingBuffer<> buffer_;
};

TEST_F(SpscRingBufferTest, PushAndPop) {
  TracePacket packet;
  packet.type = 'I';
  EXPECT_TRUE(buffer_.Push(packet));
  TracePacket popped;
  EXPECT_TRUE(buffer_.Pop(popped));
  EXPECT_EQ(popped.type, 'I');
}

TEST_F(SpscRingBufferTest, EmptyBuffer) {
  TracePacket packet;
  EXPECT_FALSE(buffer_.Pop(packet));
}

TEST(SpscRingBufferFullTest, FullBuffer) {
  SpscRingBuffer<TracePacket, 4> buffer;
  TracePacket packet;
  packet.type = 'I';

  EXPECT_TRUE(buffer.Push(packet));
  EXPECT_TRUE(buffer.Push(packet));
  EXPECT_TRUE(buffer.Push(packet));
  EXPECT_TRUE(buffer.Push(packet));

  // Should be full now
  EXPECT_FALSE(buffer.Push(packet));

  // Pop one
  TracePacket popped;
  EXPECT_TRUE(buffer.Pop(popped));

  // Should be able to push one more
  EXPECT_TRUE(buffer.Push(packet));

  // Should be full again
  EXPECT_FALSE(buffer.Push(packet));
}

TEST(SpscRingBufferConcurrentTest, ConcurrentStressTest) {
  SpscRingBuffer<TracePacket, 16> buffer;
  const int kNumItems = 100000;

  std::thread producer([&]() {
    for (int i = 0; i < kNumItems; ++i) {
      TracePacket packet;
      packet.type = 'I';
      packet.inst.pc = i;
      while (!buffer.Push(packet)) {
        std::this_thread::yield();
      }
    }
  });

  std::thread consumer([&]() {
    for (int i = 0; i < kNumItems; ++i) {
      TracePacket packet;
      while (!buffer.Pop(packet)) {
        std::this_thread::yield();
      }
      EXPECT_EQ(packet.type, 'I');
      EXPECT_EQ(packet.inst.pc, i);
    }
  });

  producer.join();
  consumer.join();

  EXPECT_TRUE(buffer.IsEmpty());
}

} // namespace coralnpu::sim::rvvi
