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
#include <cstdlib>

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

TraceDaemon::TraceDaemon(SpscRingBuffer<TracePacket, BUFFER_SIZE>* buffer, std::ostream* output_stream)
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
  running_ = false;
  if (daemon_thread_.joinable()) {
    daemon_thread_.join();
  }
  
  // Flush any remaining packets in the SpscRingBuffer
  TracePacket packet;
  while (buffer_->Pop(packet)) {
    ProcessPacket(packet);
  }
  FlushPendingInstruction();
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

void TraceDaemon::FlushPendingInstruction() {
  if (!has_any_pending_) return;

  if (!has_pending_inst_) {
    num_accumulated_updates_ = 0;
    has_any_pending_ = false;
    return;
  }

  // Validate that all chunks were received for each register update
  for (size_t i = 0; i < num_accumulated_updates_; ++i) {
    const auto& update = accumulated_updates_[i];
    uint32_t num_chunks = (update.total_size + 31) / 32;
    uint64_t expected_mask = (1ULL << num_chunks) - 1;
    if (update.received_chunks_mask != expected_mask) {
      fprintf(stderr, "[WARNING] Trace reassembly error: Incomplete chunks for %c%d. Mask: 0x%lx, Expected: 0x%lx\n",
              update.reg_type, update.index, update.received_chunks_mask, expected_mask);
    }
  }

  std::string disasm;
  if (trace_formatter_) {
    disasm = trace_formatter_->Disassemble(pending_inst_packet_.inst.instruction);
  } else {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "inst_0x%08x", pending_inst_packet_.inst.instruction);
    disasm = buf;
  }
  
  std::string symbol_str = "";
  if (symbol_resolver_) {
    symbol_str = symbol_resolver_(pending_inst_packet_.inst.pc);
  }
  std::string disasm_field = disasm;
  if (!symbol_str.empty()) {
    disasm_field = "'" + symbol_str + "' " + disasm;
  }

  char buf[128];
  std::snprintf(buf, sizeof(buf), "rvvi,0,%016lx,%08x", pending_inst_packet_.inst.pc, pending_inst_packet_.inst.instruction);
  std::string line = buf;
  if (!disasm_field.empty()) {
    line += "," + disasm_field;
  } else {
    line += ",";
  }

  for (size_t i = 0; i < num_accumulated_updates_; ++i) {
    const auto& update = accumulated_updates_[i];
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
    val_hex.reserve(update.total_size * 2);
    for (int j = static_cast<int>(update.total_size) - 1; j >= 0; --j) {
      char vbuf[4];
      std::snprintf(vbuf, sizeof(vbuf), "%02x", update.data[j]);
      val_hex += vbuf;
    }

    line += "," + reg_prefix + idx_str + ":" + val_hex;
  }

  if (output_stream_) {
    *output_stream_ << line << "\n";
  }
  
  num_accumulated_updates_ = 0;
  has_pending_inst_ = false;
  has_any_pending_ = false;
}

void TraceDaemon::ProcessPacket(const TracePacket& packet) {
  if (packet.type == 'E') {
    FlushPendingInstruction();
    running_ = false;
    return;
  }

  if (!has_any_pending_) {
    pending_v_id_ = packet.v_id;
    has_any_pending_ = true;
  } else if (packet.v_id != pending_v_id_) {
    FlushPendingInstruction();
    pending_v_id_ = packet.v_id;
    has_any_pending_ = true;
  }

  if (packet.type == 'R') {
    RegisterUpdate* ru = nullptr;
    for (size_t i = 0; i < num_accumulated_updates_; ++i) {
      if (accumulated_updates_[i].reg_type == packet.reg.reg_type && 
          accumulated_updates_[i].index == packet.reg.index) {
        ru = &accumulated_updates_[i];
        break;
      }
    }

    if (!ru) {
      if (num_accumulated_updates_ >= kMaxAccumulatedUpdates) {
        fprintf(stderr, "[FATAL] Trace reassembly error: num_accumulated_updates_ (%zu) exceeds kMaxAccumulatedUpdates (%d). Buffer overflow.\n",
                num_accumulated_updates_, kMaxAccumulatedUpdates);
        std::exit(1);
      }
      ru = &accumulated_updates_[num_accumulated_updates_++];
      ru->reg_type = packet.reg.reg_type;
      ru->index = packet.reg.index;
      // Cap total_size to prevent buffer over-read in FlushPendingInstruction.
      ru->total_size = std::min(packet.reg.total_size, static_cast<uint16_t>(sizeof(ru->data)));
      ru->received_chunks_mask = 0;
      std::memset(ru->data, 0, sizeof(ru->data));
    }

    if (ru) {
      size_t size = packet.reg.size;
      size_t offset = packet.reg.offset;
      if (offset + size <= sizeof(ru->data) && offset + size <= ru->total_size) {
        uint32_t chunk_idx = offset / 32;
        if (ru->received_chunks_mask & (1ULL << chunk_idx)) {
          fprintf(stderr, "[WARNING] Trace reassembly error: Duplicate chunk %d for %c%d at v_id %u\n",
                  chunk_idx, ru->reg_type, ru->index, packet.v_id);
        }
        std::memcpy(ru->data + offset, packet.reg.value, size);
        ru->received_chunks_mask |= (1ULL << chunk_idx);
      }
    }
    return;
  }

  if (packet.type == 'I' || packet.type == 'T') {
    pending_inst_packet_ = packet;
    has_pending_inst_ = true;
  }
}

} // namespace mpact::sim::riscv::rvvi
