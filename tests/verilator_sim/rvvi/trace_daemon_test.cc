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

#include "tests/verilator_sim/rvvi/trace_daemon.h"
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_packet.h"
#include "gtest/gtest.h"
#include <sstream>
#include <thread>
#include <chrono>

namespace mpact::sim::riscv::rvvi {

class TraceDaemonTest : public ::testing::Test {
 protected:
  SpscRingBuffer<> buffer_;
  std::stringstream output_stream_;
};

TEST_F(TraceDaemonTest, StartAndStopDaemon) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.Start();
  daemon.Stop();
  EXPECT_TRUE(true);
}

TEST_F(TraceDaemonTest, ProcessInstructionPacket) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.Start();

  TracePacket packet;
  packet.type = 'I';
  packet.inst.pc = 0x80000000;
  packet.inst.instruction = 0x00000013; // nop
  
  EXPECT_TRUE(buffer_.Push(packet));
  
  // Wait for processing
  while (!buffer_.IsEmpty()) {
    std::this_thread::yield();
  }
  
  daemon.Stop();
  
  std::string output = output_stream_.str();
  EXPECT_NE(output.find("rvvi,0,0000000080000000,00000013"), std::string::npos);
}

} // namespace mpact::sim::riscv::rvvi
