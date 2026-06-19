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
#include "tests/verilator_sim/rvvi/custom_fallback_formatter.h"
#include "gtest/gtest.h"
#include <sstream>
#include <thread>
#include <chrono>

namespace mpact::sim::riscv::rvvi {

class TraceDaemonTest : public ::testing::Test {
 protected:
  SpscRingBuffer<> buffer_;
  std::stringstream output_stream_;
  CustomFallbackFormatter formatter_;
};

TEST_F(TraceDaemonTest, StartAndStopDaemon) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.SetTraceFormatter(&formatter_);
  daemon.Start();
  daemon.Stop();
  EXPECT_TRUE(true);
}

TEST_F(TraceDaemonTest, ProcessInstructionPacket) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.SetTraceFormatter(&formatter_);
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

TEST_F(TraceDaemonTest, ProcessRegisterPacket) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.SetTraceFormatter(&formatter_);
  daemon.Start();

  TracePacket r_packet;
  r_packet.type = 'R';
  r_packet.reg.reg_type = 'X';
  r_packet.reg.index = 10;
  r_packet.reg.offset = 0;
  r_packet.reg.total_size = 8;
  r_packet.reg.size = 8;
  r_packet.reg.value[0] = 0x123456789abcdef0;
  
  EXPECT_TRUE(buffer_.Push(r_packet));

  TracePacket i_packet;
  i_packet.type = 'I';
  i_packet.inst.pc = 0x80000004;
  i_packet.inst.instruction = 0x00100513; // li a0, 1
  
  EXPECT_TRUE(buffer_.Push(i_packet));
  
  // Wait for processing
  while (!buffer_.IsEmpty()) {
    std::this_thread::yield();
  }
  
  daemon.Stop();
  
  std::string output = output_stream_.str();
  EXPECT_NE(output.find("rvvi,0,0000000080000004,00100513"), std::string::npos);
  EXPECT_NE(output.find("x10:123456789abcdef0"), std::string::npos);
}

TEST_F(TraceDaemonTest, ProcessEndPacketTerminatesCleanly) {
  // This test verifies that pushing an 'E' packet causes the daemon to stop
  // and destruct cleanly without leaving threads unjoined (QA-004).
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.SetTraceFormatter(&formatter_);
  daemon.Start();

  TracePacket e_packet;
  e_packet.type = 'E';
  
  EXPECT_TRUE(buffer_.Push(e_packet));
  
  // Wait for processing
  while (!buffer_.IsEmpty()) {
    std::this_thread::yield();
  }
  
  // Give the daemon thread time to process the 'E' packet and set running_ = false.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // If the bug exists, daemon destructor might crash due to unjoined thread
  // because running_ was set to false by ProcessPacket, making Stop() return early.
}

} // namespace mpact::sim::riscv::rvvi
