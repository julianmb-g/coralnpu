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

#include <chrono>
#include <sstream>
#include <thread>

#include "gtest/gtest.h"

#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_daemon.h"
#include "tests/verilator_sim/rvvi/trace_packet.h"

namespace mpact::sim::riscv::rvvi {

class TraceDaemonTest : public ::testing::Test {
protected:
  SpscRingBuffer<> buffer_;
  std::stringstream output_stream_;
};

TEST_F(TraceDaemonTest, StartAndStopDaemon) {
  TraceDaemon<> daemon(&buffer_, &output_stream_, nullptr);
  EXPECT_FALSE(daemon.IsRunning());
  daemon.Start();
  EXPECT_TRUE(daemon.IsRunning());
  daemon.Stop();
  EXPECT_FALSE(daemon.IsRunning());
}

TEST_F(TraceDaemonTest, ProcessVectorChunkReassembly) {
  TraceDaemon<> daemon(&buffer_, &output_stream_, nullptr);
  daemon.Start();

  // Send instruction packet first to set current instruction context
  TracePacket inst_packet = {};
  inst_packet.type = 'I';
  inst_packet.pc = 0x1000;
  inst_packet.inst = 0x00000057; // Vector instruction
  std::strcpy(reinterpret_cast<char *>(inst_packet.raw_bytes), "vadd.vv");
  EXPECT_TRUE(buffer_.Push(inst_packet));

  // Send first chunk
  TracePacket p1 = {};
  p1.type = 'R';
  p1.reg_type = 'V';
  p1.reg_index = 0;
  p1.chunk_size = 32;
  p1.offset = 0;
  p1.total_size = 64;
  for (int i = 0; i < 4; ++i)
    p1.raw_words[i] = i; // Simplified data

  EXPECT_TRUE(buffer_.Push(p1));

  // Send second chunk
  TracePacket p2 = {};
  p2.type = 'R';
  p2.reg_type = 'V';
  p2.reg_index = 0;
  p2.chunk_size = 32;
  p2.offset = 32;
  p2.total_size = 64;
  for (int i = 4; i < 8; ++i)
    p2.raw_words[i - 4] = i;

  EXPECT_TRUE(buffer_.Push(p2));

  // Send end packet to flush and process the line
  TracePacket e_packet = {};
  e_packet.type = 'E';
  EXPECT_TRUE(buffer_.Push(e_packet));

  // Wait for processing
  while (!buffer_.IsEmpty()) {
    std::this_thread::yield();
  }

  daemon.Stop();

  std::string output = output_stream_.str();
  EXPECT_FALSE(output.empty());
  EXPECT_NE(output.find("rvvi,0,"), std::string::npos);
  EXPECT_NE(output.find("vadd.vv"), std::string::npos);
}

TEST_F(TraceDaemonTest, ProcessEndPacketTerminatesCleanly) {
  // This test verifies that pushing an 'E' packet causes the daemon to stop
  // and destruct cleanly without leaving threads unjoined (QA-004).
  TraceDaemon<> daemon(&buffer_, &output_stream_, nullptr);
  daemon.Start();

  TracePacket e_packet;
  e_packet.type = 'E';

  EXPECT_TRUE(buffer_.Push(e_packet));

  // Wait for processing
  while (!buffer_.IsEmpty()) {
    std::this_thread::yield();
  }

  // Give the daemon thread time to process the 'E' packet and set running_ =
  // false.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_FALSE(daemon.IsRunning());
}

TEST_F(TraceDaemonTest, ProcessMultiIssueInstructions) {
  // This test verifies that the daemon correctly handles multiple instructions
  // retired in sequence (simulating superscalar/multi-issue retirement).
  TraceDaemon<> daemon(&buffer_, &output_stream_, nullptr);
  daemon.Start();

  // Lane 0 instruction
  TracePacket p0 = {};
  p0.type = 'I';
  p0.pc = 0x80000000;
  p0.inst = 0x00000013; // nop
  std::strcpy(reinterpret_cast<char *>(p0.raw_bytes), "nop");
  EXPECT_TRUE(buffer_.Push(p0));

  // Lane 1 instruction
  TracePacket p1 = {};
  p1.type = 'I';
  p1.pc = 0x80000004;
  p1.inst = 0x00100513; // li a0, 1
  std::strcpy(reinterpret_cast<char *>(p1.raw_bytes), "li a0, 1");
  EXPECT_TRUE(buffer_.Push(p1));

  // End packet to flush everything
  TracePacket e_packet = {};
  e_packet.type = 'E';
  EXPECT_TRUE(buffer_.Push(e_packet));

  while (!buffer_.IsEmpty()) {
    std::this_thread::yield();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  daemon.Stop();

  std::string output = output_stream_.str();
  // Verify both instructions appeared in the trace output.
  EXPECT_NE(output.find("80000000"), std::string::npos);
  EXPECT_NE(output.find("80000004"), std::string::npos);
}

} // namespace mpact::sim::riscv::rvvi
