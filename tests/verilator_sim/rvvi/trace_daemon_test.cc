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
  EXPECT_FALSE(daemon.is_running());
  daemon.Start();
  EXPECT_TRUE(daemon.is_running());
  daemon.Stop();
  EXPECT_FALSE(daemon.is_running());
}

TEST_F(TraceDaemonTest, ProcessInstructionPacket) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.SetTraceFormatter(&formatter_);
  daemon.Start();

  TracePacket packet = {};
  packet.type = 'I';
  packet.v_id = 1;
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

  TracePacket r_packet = {};
  r_packet.type = 'R';
  r_packet.v_id = 1;
  r_packet.reg.reg_type = 'X';
  r_packet.reg.index = 10;
  r_packet.reg.offset = 0;
  r_packet.reg.total_size = 8;
  r_packet.reg.size = 8;
  r_packet.reg.value[0] = 0x123456789abcdef0;
  
  EXPECT_TRUE(buffer_.Push(r_packet));

  TracePacket i_packet = {};
  i_packet.type = 'I';
  i_packet.v_id = 1;
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

TEST_F(TraceDaemonTest, InterleavedPacketStreams) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.SetTraceFormatter(&formatter_);
  daemon.Start();

  // Instruction 1: R comes before I
  TracePacket r1 = {};
  r1.type = 'R'; r1.v_id = 1; r1.reg.reg_type = 'X'; r1.reg.index = 1;
  r1.reg.total_size = 4; r1.reg.size = 4; r1.reg.value[0] = 0xAAAA;
  buffer_.Push(r1);

  TracePacket i1 = {};
  i1.type = 'I'; i1.v_id = 1; i1.inst.pc = 0x1000; i1.inst.instruction = 0x1234;
  buffer_.Push(i1);

  // Instruction 2: I comes before R
  TracePacket i2 = {};
  i2.type = 'I'; i2.v_id = 2; i2.inst.pc = 0x2000; i2.inst.instruction = 0x5678;
  buffer_.Push(i2);

  TracePacket r2 = {};
  r2.type = 'R'; r2.v_id = 2; r2.reg.reg_type = 'X'; r2.reg.index = 2;
  r2.reg.total_size = 4; r2.reg.size = 4; r2.reg.value[0] = 0xBBBB;
  buffer_.Push(r2);

  daemon.Stop();
  
  std::string output = output_stream_.str();
  // Check I1 with R1 (Disassembly fallback adds 'inst_0x...')
  EXPECT_NE(output.find("rvvi,0,0000000000001000,00001234,inst_0x00001234,x1:0000aaaa"), std::string::npos);
  // Check I2 with R2
  EXPECT_NE(output.find("rvvi,0,0000000000002000,00005678,inst_0x00005678,x2:0000bbbb"), std::string::npos);
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

TEST_F(TraceDaemonTest, ProcessLargeRegisterPacket) {
  TraceDaemon daemon(&buffer_, &output_stream_);
  daemon.SetTraceFormatter(&formatter_);
  daemon.Start();

  TracePacket r_packet = {};
  r_packet.type = 'R';
  r_packet.v_id = 1;
  r_packet.reg.reg_type = 'V'; // Vector
  r_packet.reg.index = 1;
  r_packet.reg.offset = 0;
  r_packet.reg.total_size = 128; // Fits in 256-byte buffer
  r_packet.reg.size = 32;
  for (int i = 0; i < 4; ++i) r_packet.reg.value[i] = 0x1111111111111111;
  
  for (int i = 0; i < 4; ++i) {
    r_packet.reg.offset = i * 32;
    EXPECT_TRUE(buffer_.Push(r_packet));
  }

  TracePacket i_packet = {};
  i_packet.type = 'I';
  i_packet.v_id = 1;
  i_packet.inst.pc = 0x80000000;
  i_packet.inst.instruction = 0x00000013;
  
  EXPECT_TRUE(buffer_.Push(i_packet));
  
  daemon.Stop();
  
  std::string output = output_stream_.str();
  // It should NOT be capped at 64 bytes anymore. It should output 128 bytes (256 hex digits).
  size_t pos = output.find("v1:");
  EXPECT_NE(pos, std::string::npos);
  size_t end_pos = output.find_first_of(",\n", pos);
  std::string hex = output.substr(pos + 3, end_pos - (pos + 3));
  EXPECT_EQ(hex.length(), 256); // 128 bytes * 2 hex chars/byte
}

} // namespace mpact::sim::riscv::rvvi
