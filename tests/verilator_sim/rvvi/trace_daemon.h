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

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "mpact/sim/util/program_loader/elf_program_loader.h"
#include "tests/verilator_sim/rvvi/register_value.h"
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_packet.h"

#include "absl/synchronization/blocking_counter.h"
#include "absl/synchronization/mutex.h"
#include "absl/synchronization/notification.h"
#include <semaphore.h>

namespace mpact::sim::riscv::rvvi {

inline constexpr size_t kBufferSize = 4096;

template <size_t VLEN = 128, size_t BufferSize = kBufferSize>
class TraceDaemon {
public:
  TraceDaemon(SpscRingBuffer<TracePacket, BufferSize> *buffer,
              std::ostream *output_stream,
              mpact::sim::util::ElfProgramLoader *elf_loader)
      : buffer_(buffer), output_stream_(output_stream), elf_loader_(elf_loader),
        running_(false), sim_delay_ms_(0), first_line_(true) {
    const char *sim_delay_env = std::getenv("SIM_DELAY_MS");
    if (sim_delay_env) {
      if (!absl::SimpleAtoi(sim_delay_env, &sim_delay_ms_)) {
        LOG(WARNING)
            << "Invalid SIM_DELAY_MS environment variable, defaulting to 0";
      }
    }
    sem_init(&step_sem_, 0, 0);
  }

  ~TraceDaemon() {
    Stop();
    sem_destroy(&step_sem_);
  }

  void Start() {
    running_ = true;
    daemon_thread_ = std::thread(&TraceDaemon::DaemonLoop, this);
  }

  void Stop() {
    running_ = false;
    if (sim_delay_ms_ < 0) {
      // Release any thread waiting on the semaphore
      sem_post(&step_sem_);
    }
    if (daemon_thread_.joinable()) {
      daemon_thread_.join();
    }
  }

  bool IsRunning() const { return running_; }

  // Manually step the daemon if sim_delay_ms_ < 0.
  void Step(int count = 1) {
    for (int i = 0; i < count; ++i) {
      sem_post(&step_sem_);
    }
  }

private:
  void DaemonLoop() {
    while (running_ || !buffer_->IsEmpty()) {
      if (sim_delay_ms_ < 0 && running_) {
        sem_wait(&step_sem_);
      }

      TracePacket packet;
      if (buffer_->Pop(packet)) {
        ProcessPacket(packet);
        if (sim_delay_ms_ > 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(sim_delay_ms_));
        }
      } else {
        if (!running_)
          break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
  }

  void ProcessPacket(const TracePacket &packet) {
    switch (packet.type) {
    case 'I':
      FlushLine();
      current_pc_ = packet.pc;
      current_inst_ = packet.inst;
      std::memcpy(current_disasm_, packet.raw_bytes, 32);
      current_disasm_[31] = '\0'; // Ensure null-termination
      has_pending_inst_ = true;
      break;
    case 'R': {
      RegisterValue *reg = nullptr;
      for (size_t i = 0; i < num_pending_registers_; ++i) {
        if (pending_registers_[i].RegType() == packet.reg_type &&
            pending_registers_[i].Index() == packet.reg_index) {
          reg = &pending_registers_[i];
          break;
        }
      }

      if (reg == nullptr) {
        if (num_pending_registers_ < 32) {
          reg = &pending_registers_[num_pending_registers_++];
          reg->SetRegType(packet.reg_type);
          reg->SetIndex(packet.reg_index);
          reg->SetCurrentSize(packet.total_size);
          std::memset(reg->DataPtr(), 0, 256);
        } else {
          LOG(ERROR) << "Too many pending registers";
          return;
        }
      }

      if (packet.offset + packet.chunk_size <= 256) {
        std::memcpy(reg->DataPtr() + packet.offset, packet.raw_bytes,
                    packet.chunk_size);
      } else {
        LOG(ERROR) << "Chunk out of bounds";
      }
      break;
    }
    case 'E':
      FlushLine();
      running_ = false;
      break;
    default:
      LOG(WARNING) << "Unknown packet type: " << packet.type;
      break;
    }
  }

  void FlushLine() {
    if (!has_pending_inst_)
      return;

    // Format: rvvi,0,PC,INST,DISASM[,REG:VAL]*
    std::string line = absl::StrFormat("rvvi,0,%016lx,%08x,%s", current_pc_,
                                       current_inst_, current_disasm_);

    for (size_t i = 0; i < num_pending_registers_; ++i) {
      const auto &reg = pending_registers_[i];
      line +=
          absl::StrFormat(",%c%d:", std::tolower(reg.RegType()), reg.Index());
      auto data = reg.Data();
      for (int j = data.size(); j > 0; --j) {
        line += absl::StrFormat("%02x", data[j - 1]);
      }
    }
    *output_stream_ << line << "\n";

    num_pending_registers_ = 0;
    has_pending_inst_ = false;
  }

  SpscRingBuffer<TracePacket, BufferSize> *buffer_;
  std::ostream *output_stream_;
  mpact::sim::util::ElfProgramLoader *elf_loader_;
  std::thread daemon_thread_;
  std::atomic<bool> running_;
  int sim_delay_ms_ = 0;
  sem_t step_sem_;

  // Internal accumulation state
  uint64_t current_pc_ = 0;
  uint32_t current_inst_ = 0;
  char current_disasm_[32];
  bool has_pending_inst_ = false;
  bool first_line_ = true;

  RegisterValue pending_registers_[32];
  size_t num_pending_registers_ = 0;
};

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_TRACE_DAEMON_H_
