// Copyright 2025 Google LLC
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

#include "hw_sim/coralnpu_simulator.h"

// Note: This is a placeholder for the Barebones Simulator implementation.
// It will be fully implemented in a subsequent phase.

class CoreBarebonesSimulator : public CoralNPUSimulator {
 public:
  CoreBarebonesSimulator() {}
  ~CoreBarebonesSimulator() final = default;

  void ReadTCM(uint32_t addr, size_t size, char* data) final {
    // Barebones: Idealized memory read.
  }
  const CoralNPUMailbox& ReadMailbox(void) final {
    static CoralNPUMailbox mailbox;
    return mailbox;
  }
  void WriteTCM(uint32_t addr, size_t size, const char* data) final {
    // Barebones: Idealized memory write.
  }
  void WriteMailbox(const CoralNPUMailbox& mailbox) final {
    // Barebones: Mailbox write.
  }
  void Run(uint32_t start_addr) final {
    // Barebones: Run core directly.
  }
  bool WaitForTermination(int timeout) final {
    return true; // Barebones: Termination.
  }
};

// static
CoralNPUSimulator* CoralNPUSimulator::Create() {
  return new CoreBarebonesSimulator();
}
