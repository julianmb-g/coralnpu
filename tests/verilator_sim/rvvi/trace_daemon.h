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

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 4096
#endif

template<size_t VLEN = 2048, size_t MAX_UPDATES = 64>
class TraceDaemon {
 public:
  TraceDaemon(SpscRingBuffer<TracePacket, BUFFER_SIZE>* buffer, std::ostream* output_stream);
  ~TraceDaemon();

  void Start();
  void Stop();

  bool is_running() const { return running_; }

  void SetSymbolResolver(std::function<std::string(uint64_t)> resolver);
  void SetTraceFormatter(TraceFormatterInterface* formatter);

 private:
  void DaemonLoop();
  void ProcessPacket(const TracePacket& packet);

  struct RegisterUpdate {
    uint8_t reg_type;
    uint16_t index;
    uint16_t total_size;
    uint8_t data[VLEN / 8]; // Parameterized to VLEN to restore configuration fidelity.
    uint64_t received_chunks_mask; // Bitmask of received 32-byte chunks

    static_assert(VLEN >= 128, "VLEN must be at least 128 bits.");
  };

  SpscRingBuffer<TracePacket, BUFFER_SIZE>* buffer_;
  std::ostream* output_stream_;
  std::thread daemon_thread_;
  std::atomic<bool> running_;
  
  std::function<std::string(uint64_t)> symbol_resolver_;
  
  static constexpr int kMaxAccumulatedUpdates = MAX_UPDATES;
  RegisterUpdate accumulated_updates_[kMaxAccumulatedUpdates];
  size_t num_accumulated_updates_ = 0;

  TraceFormatterInterface* trace_formatter_ = nullptr;

  TracePacket pending_inst_packet_;
  uint32_t pending_v_id_ = 0;
  bool has_pending_inst_ = false;
  bool has_any_pending_ = false;

  int sim_delay_ms_ = 0;

  void FlushPendingInstruction();
};

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_TRACE_DAEMON_H_
