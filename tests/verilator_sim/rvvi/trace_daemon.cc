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
    std::string disasm = Disassemble(packet.inst.instruction);
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

std::string TraceDaemon::Disassemble(uint32_t inst) {
  if (inst == 0x00000013) return "nop";
  if (inst == 0x08000073) return "mpause";
  if (inst == 0x00100073) return "ebreak";

  uint32_t opcode = inst & 0x7f;
  uint32_t funct3 = (inst >> 12) & 0x7;
  uint32_t funct7 = (inst >> 25) & 0x7f;
  uint32_t rd = (inst >> 7) & 0x1f;
  uint32_t rs1 = (inst >> 15) & 0x1f;
  uint32_t rs2 = (inst >> 20) & 0x1f;

  auto reg_name = [](uint32_t r) {
    static const char* kRegNames[] = {
      "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
      "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
      "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
      "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    return kRegNames[r & 31];
  };

  if (opcode == 0x13) { // OP-IMM
    std::int32_t imm = static_cast<std::int32_t>(inst) >> 20;
    if (funct3 == 0) { // ADDI
      if (rs1 == 0) return "li " + std::string(reg_name(rd)) + "," + std::to_string(imm);
      return "addi " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(imm);
    }
    if (funct3 == 1 && funct7 == 0) return "slli " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(rs2);
    if (funct3 == 2) return "slti " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(imm);
    if (funct3 == 3) return "sltiu " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(imm);
    if (funct3 == 4) return "xori " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(imm);
    if (funct3 == 5) {
      if (funct7 == 0) return "srli " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(rs2);
      if (funct7 == 0x20) return "srai " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(rs2);
    }
    if (funct3 == 6) return "ori " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(imm);
    if (funct3 == 7) return "andi " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(imm);
  }
  if (opcode == 0x33) { // OP
    std::string op = "unknown";
    if (funct7 == 0) {
      if (funct3 == 0) op = "add";
      else if (funct3 == 1) op = "sll";
      else if (funct3 == 2) op = "slt";
      else if (funct3 == 3) op = "sltu";
      else if (funct3 == 4) op = "xor";
      else if (funct3 == 5) op = "srl";
      else if (funct3 == 6) op = "or";
      else if (funct3 == 7) op = "and";
    } else if (funct7 == 0x20) {
      if (funct3 == 0) op = "sub";
      else if (funct3 == 5) op = "sra";
    } else if (funct7 == 1) {
      if (funct3 == 0) op = "mul";
      else if (funct3 == 1) op = "mulh";
      else if (funct3 == 2) op = "mulhsu";
      else if (funct3 == 3) op = "mulhu";
      else if (funct3 == 4) op = "div";
      else if (funct3 == 5) op = "divu";
      else if (funct3 == 6) op = "rem";
      else if (funct3 == 7) op = "remu";
    }
    return op + " " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + reg_name(rs2);
  }
  if (opcode == 0x37) { // LUI
    std::uint32_t imm = inst & 0xfffff000;
    char sbuf[32];
    std::snprintf(sbuf, sizeof(sbuf), "0x%x", imm >> 12);
    return "lui " + std::string(reg_name(rd)) + "," + sbuf;
  }
  if (opcode == 0x17) { // AUIPC
    std::uint32_t imm = inst & 0xfffff000;
    char sbuf[32];
    std::snprintf(sbuf, sizeof(sbuf), "0x%x", imm >> 12);
    return "auipc " + std::string(reg_name(rd)) + "," + sbuf;
  }
  if (opcode == 0x6f) { // JAL
    std::int32_t imm = 0;
    imm |= (inst & 0x80000000) >> 11; // 20
    imm |= (inst & 0x7fe00000) >> 20; // 10:1
    imm |= (inst & 0x00100000) >> 9;  // 11
    imm |= (inst & 0x000ff000);       // 19:12
    if (imm & 0x100000) imm |= 0xffe00000; // sign extend
    return "jal " + std::string(reg_name(rd)) + "," + std::to_string(imm);
  }
  if (opcode == 0x67) { // JALR
    std::int32_t imm = static_cast<std::int32_t>(inst) >> 20;
    return "jalr " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(imm);
  }
  if (opcode == 0x63) { // BRANCH
    std::int32_t imm = 0;
    imm |= (inst & 0x80000000) >> 19; // 12
    imm |= (inst & 0x7e000000) >> 20; // 10:5
    imm |= (inst & 0x00000f00) >> 7;  // 4:1
    imm |= (inst & 0x00000080) << 4;  // 11
    if (imm & 0x1000) imm |= 0xffffe000; // sign extend
    std::string op = "unknown_b";
    if (funct3 == 0) op = "beq";
    else if (funct3 == 1) op = "bne";
    else if (funct3 == 4) op = "blt";
    else if (funct3 == 5) op = "bge";
    else if (funct3 == 6) op = "bltu";
    else if (funct3 == 7) op = "bgeu";
    return op + " " + std::string(reg_name(rs1)) + "," + reg_name(rs2) + "," + std::to_string(imm);
  }
  if (opcode == 0x03) { // LOAD
    std::int32_t imm = static_cast<std::int32_t>(inst) >> 20;
    std::string op = "unknown_l";
    if (funct3 == 0) op = "lb";
    else if (funct3 == 1) op = "lh";
    else if (funct3 == 2) op = "lw";
    else if (funct3 == 4) op = "lbu";
    else if (funct3 == 5) op = "lhu";
    return op + " " + std::string(reg_name(rd)) + "," + std::to_string(imm) + "(" + reg_name(rs1) + ")";
  }
  if (opcode == 0x23) { // STORE
    std::int32_t imm = 0;
    imm |= (inst & 0xfe000000) >> 20;
    imm |= (inst & 0x00000f80) >> 7;
    if (imm & 0x800) imm |= 0xfffff000; // sign extend
    std::string op = "unknown_s";
    if (funct3 == 0) op = "sb";
    else if (funct3 == 1) op = "sh";
    else if (funct3 == 2) op = "sw";
    return op + " " + std::string(reg_name(rs2)) + "," + std::to_string(imm) + "(" + reg_name(rs1) + ")";
  }
  if (opcode == 0x73) { // SYSTEM
    std::string op = "system";
    if (funct3 == 1) op = "csrrw";
    else if (funct3 == 2) op = "csrrs";
    else if (funct3 == 3) op = "csrrc";
    else if (funct3 == 5) op = "csrrwi";
    else if (funct3 == 6) op = "csrrsi";
    else if (funct3 == 7) op = "csrrci";
    std::uint32_t csr = inst >> 20;
    char csr_hex[16];
    std::snprintf(csr_hex, sizeof(csr_hex), "0x%x", csr);
    if (funct3 == 0) {
      if (funct7 == 0 && rs2 == 0) return "ecall";
      if (funct7 == 1 && rs2 == 0) return "ebreak";
    }
    if (funct3 >= 1 && funct3 <= 3) {
      return op + " " + std::string(reg_name(rd)) + "," + csr_hex + "," + reg_name(rs1);
    }
    if (funct3 >= 5 && funct3 <= 7) {
      return op + " " + std::string(reg_name(rd)) + "," + csr_hex + "," + std::to_string(rs1);
    }
  }

  char sbuf[32];
  std::snprintf(sbuf, sizeof(sbuf), "inst_0x%08x", inst);
  return sbuf;
}

} // namespace mpact::sim::riscv::rvvi
