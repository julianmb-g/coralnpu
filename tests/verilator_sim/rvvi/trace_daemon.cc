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
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

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

template<size_t VLEN, size_t MAX_UPDATES>
TraceDaemon<VLEN, MAX_UPDATES>::TraceDaemon(SpscRingBuffer<TracePacket, BUFFER_SIZE>* buffer, std::ostream* output_stream)
    : buffer_(buffer), output_stream_(output_stream), running_(false), paused_(false) {}

template<size_t VLEN, size_t MAX_UPDATES>
TraceDaemon<VLEN, MAX_UPDATES>::~TraceDaemon() {
  Stop();
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::Start() {
  if (running_) return;
  running_ = true;
  daemon_thread_ = std::thread(&TraceDaemon::DaemonLoop, this);
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::Stop() {
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

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::Pause() {
  paused_ = true;
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::Resume() {
  paused_ = false;
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::SetSymbolResolver(std::function<std::string(uint64_t)> resolver) {
  symbol_resolver_ = resolver;
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::SetTraceFormatter(TraceFormatterInterface* formatter) {
  trace_formatter_ = formatter;
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::DaemonLoop() {
  while (running_.load() || !buffer_->IsEmpty()) {
    if (paused_.load()) {
      std::this_thread::yield();
      continue;
    }
    TracePacket packet;
    if (buffer_->Pop(packet)) {
      ProcessPacket(packet);
    } else {
      std::this_thread::yield();
    }
  }
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::FlushPendingInstruction() {
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
    if (num_chunks >= sizeof(update.received_chunks_mask) * 8) {
      LOG(ERROR) << absl::StrFormat("[FATAL] Trace reassembly error: num_chunks (%u) exceeds mask capacity.", num_chunks);
      std::exit(1);
    }
    uint64_t expected_mask = (1ULL << num_chunks) - 1;
    if (update.received_chunks_mask != expected_mask) {
      LOG(ERROR) << absl::StrFormat("[FATAL] Trace reassembly error: Incomplete chunks for %c%d. Mask: 0x%lx, Expected: 0x%lx",
              update.reg_type, update.index, update.received_chunks_mask, expected_mask);
      std::exit(1);
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
    uint32_t num_chunks = (update.total_size + 31) / 32;
    if (num_chunks >= sizeof(update.received_chunks_mask) * 8) {
      LOG(ERROR) << absl::StrFormat("[FATAL] Trace reassembly error: num_chunks (%u) exceeds mask capacity.", num_chunks);
      std::exit(1);
    }
    uint64_t expected_mask = (1ULL << num_chunks) - 1;
    if (update.received_chunks_mask != expected_mask) {
        LOG(ERROR) << absl::StrFormat("[FATAL] Incomplete vector chunks detected for register %c%d. Mask: 0x%016lx. Expected: 0x%016lx", update.reg_type, update.index, update.received_chunks_mask, expected_mask);
        std::exit(1);
    }

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
    if (!*output_stream_) {
        fprintf(stderr, "[ERROR] Trace output stream error!\n");
        running_ = false;
        return;
    }
    output_stream_->flush();
    if (!output_stream_->good()) {
        fprintf(stderr, "[ERROR] Trace output stream flush error!\n");
        running_ = false;
        return;
    }
  }
  
  num_accumulated_updates_ = 0;
  has_pending_inst_ = false;
  has_any_pending_ = false;
}

template<size_t VLEN, size_t MAX_UPDATES>
void TraceDaemon<VLEN, MAX_UPDATES>::ProcessPacket(const TracePacket& packet) {
  if (packet.type == 'E') {
    FlushPendingInstruction();
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
    RegisterUpdate* register_update = nullptr;
    for (size_t i = 0; i < num_accumulated_updates_; ++i) {
      if (accumulated_updates_[i].reg_type == packet.reg.reg_type && 
          accumulated_updates_[i].index == packet.reg.index) {
        register_update = &accumulated_updates_[i];
        break;
      }
    }

    if (!register_update) {
      if (num_accumulated_updates_ >= kMaxAccumulatedUpdates) {
        fprintf(stderr, "[FATAL] Trace reassembly error: num_accumulated_updates_ (%zu) exceeds kMaxAccumulatedUpdates (%d). Buffer overflow.\n",
                num_accumulated_updates_, kMaxAccumulatedUpdates);
        std::exit(1);
      }
      if (packet.reg.total_size > sizeof(RegisterUpdate::data)) {
        fprintf(stderr, "[FATAL] Trace reassembly error: packet.reg.total_size (%d) exceeds register_update->data size (%zu).\n",
                packet.reg.total_size, sizeof(RegisterUpdate::data));
        std::exit(1);
      }
      register_update = &accumulated_updates_[num_accumulated_updates_++];
      register_update->reg_type = packet.reg.reg_type;
      register_update->index = packet.reg.index;
      register_update->total_size = packet.reg.total_size;
      register_update->received_chunks_mask = 0;
      std::memset(register_update->data, 0, sizeof(register_update->data));
    }

    if (register_update) {
      size_t size = packet.reg.size;
      size_t offset = packet.reg.offset;
      if (offset + size <= sizeof(register_update->data) && offset + size <= register_update->total_size) {
        uint32_t chunk_idx = offset / 32;
        if (register_update->received_chunks_mask & (1ULL << chunk_idx)) {
          fprintf(stderr, "[WARNING] Trace reassembly error: Duplicate chunk %d for %c%d at v_id %u\n",
                  chunk_idx, register_update->reg_type, register_update->index, packet.v_id);
        }
        std::memcpy(register_update->data + offset, packet.reg.value, size);
        register_update->received_chunks_mask |= (1ULL << chunk_idx);
      } else {
        fprintf(stderr, "[FATAL] Trace reassembly error: Invalid offset %zu + size %zu for %c%d. Max offset: %zu, Total Size: %d. Packet discarded.\n",
                offset, size, register_update->reg_type, register_update->index, sizeof(register_update->data), register_update->total_size);
        std::exit(1);
      }
    }
    return;
  }

  if (packet.type == 'I' || packet.type == 'T') {
    pending_inst_packet_ = packet;
    has_pending_inst_ = true;
  }
}

// Explicit Instantiations
template class TraceDaemon<128, 64>;
template class TraceDaemon<256, 64>;
template class TraceDaemon<512, 64>;
template class TraceDaemon<1024, 64>;
template class TraceDaemon<2048, 64>; // Default

} // namespace mpact::sim::riscv::rvvi
