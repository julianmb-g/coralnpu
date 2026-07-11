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

#include <gtest/gtest.h>
#include <sstream>
#include <thread>
#include <chrono>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/log.h"
#include "absl/log/check.h"

#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_daemon.h"

using namespace mpact::sim::riscv::rvvi;

// A mock daemon that doesn't consume anything, to test backpressure.
class MockStuckDaemon {
 public:
  MockStuckDaemon(SpscRingBuffer<TracePacket, 4096>* buffer)
      : buffer_(buffer) {}

  // Intentionally do NOT start any consumer thread.

 private:
  SpscRingBuffer<TracePacket, 4096>* buffer_;
};

TEST(CoreRvviWatchdogTest, BackpressureDetection) {
  SpscRingBuffer<TracePacket, 4096> buffer;
  // We don't even need a daemon, just a buffer that isn't being emptied.

  // Push packets until full.
  int pushed = 0;
  while (buffer.Push({})) {
    pushed++;
  }
  EXPECT_EQ(pushed, 4096);
  EXPECT_TRUE(buffer.IsFull());

  // Now test the watchdog-like behavior: a Push should fail.
  EXPECT_FALSE(buffer.Push({}));
}

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage("CoralNPU RVVI Watchdog Test");
  absl::ParseCommandLine(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
