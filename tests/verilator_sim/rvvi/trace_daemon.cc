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
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace mpact::sim::riscv::rvvi {

/*
 * Rationale for Threading and Queueing Mechanism in TraceDaemon:
 * 
 * To meet the zero-loss tracing objective without stalling the main simulation thread,
 * we employ a single-producer single-consumer (SPSC) lock-free ring buffer (SpscRingBuffer).
 * The main thread is the sole producer pushing TracePacket structs to the buffer, while
 * the TraceDaemon background thread acts as the sole consumer pulling and formatting these packets.
 *
 * Threading Architecture:
 * - A dedicated std::thread is spawned on Start(), running DaemonLoop() in the background.
 * - SpscRingBuffer uses memory-ordered atomic pointers (head and tail) to synchronize state
 *   without traditional mutex locks or conditional variables, minimizing hot-path overhead.
 * - Yielding: When the buffer is empty, the daemon thread calls std::this_thread::yield() to
 *   avoid 100% CPU core pinning while keeping latency low.
 * - Backpressure: If the queue fills up, the producer (main simulation thread) blocks and
 *   yields until space becomes available, ensuring absolute zero packet loss during heavy I/O.
 * - Graceful Shutdown: On Stop(), the running flag is cleared, the daemon thread is joined,
 *   and any remaining buffered packets are synchronously processed (flushed) to guarantee 
 *   that the last instructions and termination states (e.g. mpause) are written to the trace.
 */

TraceDaemon::TraceDaemon(SpscRingBuffer<TracePacket, 4096>* buffer, std::ostream* output_stream)
    : buffer_(buffer), output_stream_(output_stream), running_(false) {}

TraceDaemon::~TraceDaemon() {
  Stop();
}

void TraceDaemon::Start() {
  if (running_) return;
  running_ = true;
  daemon_thread_ = std::thread(&TraceDaemon::DaemonLoop, this);
}

void TraceDaemon::Stop() {
  if (!running_) return;
  running_ = false;
  if (daemon_thread_.joinable()) {
    daemon_thread_.join();
  }
  
  // Flush any remaining packets in the SpscRingBuffer
  TracePacket packet;
  while (buffer_->Pop(packet)) {
    ProcessPacket(packet);
  }
  if (output_stream_) {
    output_stream_->flush();
  }
}

void TraceDaemon::SetSymbolResolver(std::function<std::string(uint64_t)> resolver) {
  symbol_resolver_ = resolver;
}

void TraceDaemon::SetTraceFormatter(TraceFormatterInterface* formatter) {
  trace_formatter_ = formatter;
}

void TraceDaemon::DaemonLoop() {
  while (running_ || !buffer_->IsEmpty()) {
    TracePacket packet;
    if (buffer_->Pop(packet)) {
      ProcessPacket(packet);
    } else {
      std::this_thread::yield();
    }
  }
}

void TraceDaemon::ProcessPacket(const TracePacket& packet) {
  if (packet.type == 'E') {
    running_ = false;
    return;
  }

  if (packet.type == 'R') {
    auto it = std::find_if(accumulated_updates_.begin(), accumulated_updates_.end(),
                           [&](const RegisterUpdate& u) {
                             return u.reg_type == packet.reg.reg_type && u.index == packet.reg.index;
                           });
    if (it == accumulated_updates_.end()) {
      RegisterUpdate ru;
      ru.reg_type = packet.reg.reg_type;
      ru.index = packet.reg.index;
      ru.total_size = packet.reg.total_size;
      ru.data.resize(packet.reg.total_size, 0);
      size_t size = packet.reg.size;
      size_t offset = packet.reg.offset;
      if (offset + size <= ru.total_size) {
        std::memcpy(ru.data.data() + offset, packet.reg.value, size);
      }
      accumulated_updates_.push_back(ru);
    } else {
      size_t size = packet.reg.size;
      size_t offset = packet.reg.offset;
      if (offset + size <= it->total_size) {
        std::memcpy(it->data.data() + offset, packet.reg.value, size);
      }
    }
    return;
  }

  if (packet.type == 'I' || packet.type == 'T') {
    std::string disasm;
    if (trace_formatter_) {
      disasm = trace_formatter_->Disassemble(packet.inst.instruction);
    } else {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "inst_0x%08x", packet.inst.instruction);
      disasm = buf;
    }
    
    std::string symbol_str = "";
    if (symbol_resolver_) {
      symbol_str = symbol_resolver_(packet.inst.pc);
    }
    std::string disasm_field = disasm;
    if (!symbol_str.empty()) {
      disasm_field = "'" + symbol_str + "' " + disasm;
    }

    char buf[128];
    std::snprintf(buf, sizeof(buf), "rvvi,0,%016lx,%08x", packet.inst.pc, packet.inst.instruction);
    std::string line = buf;
    if (!disasm_field.empty()) {
      line += "," + disasm_field;
    } else {
      line += ",";
    }

    for (const auto& update : accumulated_updates_) {
      std::string reg_prefix;
      if (update.reg_type == 'X') reg_prefix = "x";
      else if (update.reg_type == 'F') reg_prefix = "f";
      else if (update.reg_type == 'V') reg_prefix = "v";
      else if (update.reg_type == 'C') reg_prefix = "c";
      else reg_prefix = "?";

      std::string idx_str;
      if (update.reg_type == 'C') {
        char cbuf[16];
        std::snprintf(cbuf, sizeof(cbuf), "%x", update.index);
        idx_str = cbuf;
      } else {
        idx_str = std::to_string(update.index);
      }

      std::string val_hex = "";
      val_hex.reserve(update.data.size() * 2);
      for (int i = static_cast<int>(update.data.size()) - 1; i >= 0; --i) {
        char vbuf[4];
        std::snprintf(vbuf, sizeof(vbuf), "%02x", update.data[i]);
        val_hex += vbuf;
      }

      line += "," + reg_prefix + idx_str + ":" + val_hex;
    }

    if (output_stream_) {
      *output_stream_ << line << "\n";
    }
    accumulated_updates_.clear();
  }
}

} // namespace mpact::sim::riscv::rvvi
