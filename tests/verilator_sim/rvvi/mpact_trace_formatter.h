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

#ifndef TESTS_VERILATOR_SIM_RVVI_MPACT_TRACE_FORMATTER_H_
#define TESTS_VERILATOR_SIM_RVVI_MPACT_TRACE_FORMATTER_H_

#include "tests/verilator_sim/rvvi/trace_formatter_interface.h"
#include "tests/verilator_sim/rvvi/custom_fallback_formatter.h"
#include <iostream>

namespace mpact::sim::riscv::rvvi {

// Adapter for the mpact-riscv TraceFormatter.
// In environments where mpact-riscv is unavailable, this adapter falls back
// to the CustomFallbackFormatter.
class MpactTraceFormatter : public TraceFormatterInterface {
 public:
  MpactTraceFormatter() {
    // Note: Full mpact_riscv integration is currently blocked due to the missing
    // Bazel build environment in this local Git workspace, and the complex
    // external dependencies required by mpact_riscv. This adapter serves as a
    // documented placeholder/fallback that maintains API compatibility while
    // delegating to CustomFallbackFormatter.
    std::cerr << "Warning: MpactTraceFormatter: mpact-riscv library unavailable, "
              << "falling back to CustomFallbackFormatter." << std::endl;
  }
  ~MpactTraceFormatter() override = default;

  std::string Disassemble(uint32_t inst) override {
    return fallback_formatter_.Disassemble(inst);
  }

 private:
  CustomFallbackFormatter fallback_formatter_;
};

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_MPACT_TRACE_FORMATTER_H_
