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

#ifndef TESTS_VERILATOR_SIM_RVVI_TRACE_DAEMON_H_
#define TESTS_VERILATOR_SIM_RVVI_TRACE_DAEMON_H_

#include <ostream>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_packet.h"
#include "tests/verilator_sim/rvvi/trace_formatter_interface.h"

namespace mpact::sim::riscv::rvvi {

class TraceDaemon {
 public:
  TraceDaemon(SpscRingBuffer<TracePacket, 4096>* buffer, std::ostream* output_stream);
  ~TraceDaemon();

  void Start();
  void Stop();

  void SetSymbolResolver(std::function<std::string(uint64_t)> resolver);
  void SetTraceFormatter(TraceFormatterInterface* formatter);

 private:
  void DaemonLoop();
  void ProcessPacket(const TracePacket& packet);

  struct RegisterUpdate {
    uint8_t reg_type;
    uint16_t index;
    uint16_t total_size;
    std::vector<uint8_t> data;
  };

  SpscRingBuffer<TracePacket, 4096>* buffer_;
  std::ostream* output_stream_;
  std::thread daemon_thread_;
  std::atomic<bool> running_;
  
  std::function<std::string(uint64_t)> symbol_resolver_;
  std::vector<RegisterUpdate> accumulated_updates_;
  TraceFormatterInterface* trace_formatter_ = nullptr;
};

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_TRACE_DAEMON_H_
