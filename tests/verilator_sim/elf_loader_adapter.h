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

#ifndef TESTS_VERILATOR_SIM_ELF_LOADER_ADAPTER_H_
#define TESTS_VERILATOR_SIM_ELF_LOADER_ADAPTER_H_

#include <iostream>
#include <string>
#include <cstdlib>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "mpact/sim/generic/core_debug_interface.h"
#include "mpact/sim/util/program_loader/elf_program_loader.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/coralnpu/memory_if.h"

namespace mpact::sim::util {

// Adapt the CoralNPU MemoryIf interface to the mpact-sim CoreDebugInterface
// so we can use the upstream ElfProgramLoader.
class MemoryIfDebugAdapter : public mpact::sim::generic::CoreDebugInterface {
 public:
  MemoryIfDebugAdapter(MemoryIf* mem_if) : mem_if_(mem_if) {}

  absl::Status Halt() override { return absl::UnimplementedError(""); }
  absl::Status Halt(HaltReason halt_reason) override { return absl::UnimplementedError(""); }
  absl::Status Halt(HaltReasonValueType halt_reason) override { return absl::UnimplementedError(""); }
  absl::StatusOr<int> Step(int num) override { return absl::UnimplementedError(""); }
  absl::Status Run() override { return absl::UnimplementedError(""); }
  absl::Status Wait() override { return absl::UnimplementedError(""); }
  absl::StatusOr<RunStatus> GetRunStatus() override { return RunStatus::kNone; }
  absl::StatusOr<HaltReasonValueType> GetLastHaltReason() override { return 0; }
  absl::StatusOr<uint64_t> ReadRegister(const std::string& name) override { return absl::UnimplementedError(""); }
  absl::Status WriteRegister(const std::string& name, uint64_t value) override { return absl::UnimplementedError(""); }
  absl::StatusOr<mpact::sim::generic::DataBuffer*> GetRegisterDataBuffer(const std::string& name) override { return absl::UnimplementedError(""); }

  absl::StatusOr<size_t> ReadMemory(uint64_t address, void* buf, size_t length) override {
    if (mem_if_->Read(address, buf, length)) {
      return length;
    }
    return absl::DataLossError("Read failed out of bounds");
  }

  absl::StatusOr<size_t> WriteMemory(uint64_t address, const void* buf, size_t length) override {
    if (!mem_if_->Write(address, buf, length)) {
      return absl::OutOfRangeError(absl::StrFormat(
          "[FATAL] ELF load violation. Requested: [0x%08x - 0x%08x]. "
          "Available: %s. Delta: Exceeds bounds by 0x%08x bytes.",
          static_cast<uint32_t>(address), static_cast<uint32_t>(address + length),
          mem_if_->GetProfileBounds(),
          mem_if_->GetOverflowDelta(static_cast<uint32_t>(address), length)));
    }
    return length;
  }

  bool HasBreakpoint(uint64_t address) override { return false; }
  absl::Status SetSwBreakpoint(uint64_t address) override { return absl::UnimplementedError(""); }
  absl::Status ClearSwBreakpoint(uint64_t address) override { return absl::UnimplementedError(""); }
  absl::Status ClearAllSwBreakpoints() override { return absl::UnimplementedError(""); }
  absl::StatusOr<mpact::sim::generic::Instruction*> GetInstruction(uint64_t address) override { return absl::UnimplementedError(""); }
  absl::StatusOr<std::string> GetDisassembly(uint64_t address) override { return absl::UnimplementedError(""); }

 private:
  MemoryIf* mem_if_;
};

} // namespace mpact::sim::util

#endif // TESTS_VERILATOR_SIM_ELF_LOADER_ADAPTER_H_
