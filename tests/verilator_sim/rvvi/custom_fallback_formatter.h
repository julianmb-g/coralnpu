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

#ifndef TESTS_VERILATOR_SIM_RVVI_CUSTOM_FALLBACK_FORMATTER_H_
#define TESTS_VERILATOR_SIM_RVVI_CUSTOM_FALLBACK_FORMATTER_H_

#include "tests/verilator_sim/rvvi/trace_formatter_interface.h"

namespace mpact::sim::riscv::rvvi {

class CustomFallbackFormatter : public TraceFormatterInterface {
 public:
  CustomFallbackFormatter() = default;
  ~CustomFallbackFormatter() override = default;

  std::string Disassemble(uint32_t inst) override {
    if (inst == 0x00000013) return "nop";
    if (inst == 0x08000073) return "mpause";
    if (inst == 0x00100073) return "ebreak";

    uint32_t opcode = inst & 0x7f;
    uint32_t funct3 = (inst >> 12) & 0x7;
    uint32_t funct7 = (inst >> 25) & 0x7f;
    uint32_t rd = (inst >> 7) & 0x1f;
    uint32_t rs1 = (inst >> 15) & 0x1f;
    uint32_t rs2 = (inst >> 20) & 0x1f;

    auto reg_name = [](uint32_t reg) {
      static const char* kRegNames[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
      };
      return kRegNames[reg & 31];
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
    if (opcode == 0x07) { // Vector Load
      char sbuf[32];
      std::snprintf(sbuf, sizeof(sbuf), "vload_0x%08x", inst);
      return sbuf;
    }
    if (opcode == 0x27) { // Vector Store
      char sbuf[32];
      std::snprintf(sbuf, sizeof(sbuf), "vstore_0x%08x", inst);
      return sbuf;
    }
    if (opcode == 0x57) { // Vector Opcode
      uint32_t funct3 = (inst >> 12) & 0x7;
      uint32_t funct6 = (inst >> 26) & 0x3f;

      if (funct3 == 7) { // vsetvli, vsetivli
        return "vsetvli " + std::string(reg_name(rd)) + "," + reg_name(rs1) + "," + std::to_string(inst & 0x7ff);
      }

      if (funct6 == 0) { // vadd
        if (funct3 == 0) return "vadd.vv v" + std::to_string(rd) + ",v" + std::to_string(rs2) + ",v" + std::to_string(rs1);
        if (funct3 == 3) {
          int32_t simm5 = rs1;
          if (simm5 & 0x10) simm5 |= 0xffffffe0; // Sign extend 5-bit immediate
          return "vadd.vi v" + std::to_string(rd) + ",v" + std::to_string(rs2) + "," + std::to_string(simm5);
        }
        if (funct3 == 4) return "vadd.vx v" + std::to_string(rd) + ",v" + std::to_string(rs2) + "," + reg_name(rs1);
      }

      if (funct6 == 0x31) { // vwadd.vv
        if (funct3 == 0 || funct3 == 2) return "vwadd.vv v" + std::to_string(rd) + ",v" + std::to_string(rs2) + ",v" + std::to_string(rs1);
        if (funct3 == 4 || funct3 == 6) return "vwadd.vx v" + std::to_string(rd) + ",v" + std::to_string(rs2) + "," + reg_name(rs1);
      }

      char sbuf[32];
      std::snprintf(sbuf, sizeof(sbuf), "v_inst_0x%08x", inst);
      return sbuf;
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
};

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_CUSTOM_FALLBACK_FORMATTER_H_
