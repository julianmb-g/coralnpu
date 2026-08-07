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

#ifndef TESTS_VERILATOR_SIM_CORALNPU_MEMORY_IF_H_
#define TESTS_VERILATOR_SIM_CORALNPU_MEMORY_IF_H_

#include <systemc.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"

// Abstract base class for CoralNPU memory interfaces with bounds checking.
class MemoryIf : public sc_module {
 public:
  sc_in<bool> clock;
  sc_in<bool> reset;

  MemoryIf(sc_module_name n, const char* bin, int limit, absl::string_view profile)
      : sc_module(n), profile_(profile), pending_exit_code_(0) {
    if (profile == "highmem") {
      itcm_base_ = 0x00100000;
      itcm_size_ = 1024 * 1024; // 1MB Unified
      dtcm_base_ = 0x00100000;
      dtcm_size_ = 0;
    } else {
      // default
      itcm_base_ = 0x00000000;
      itcm_size_ = 8 * 1024;   // 8KB
      dtcm_base_ = 0x00010000;
      dtcm_size_ = 32 * 1024;  // 32KB
    }
    itcm_data_.resize(itcm_size_, 0);
    dtcm_data_.resize(dtcm_size_, 0);
  }

  virtual ~MemoryIf() = default;

  int PendingExitCode() const { return pending_exit_code_; }
  void SetPendingExitCode(int code) { pending_exit_code_ = code; }

  // Reads 'bytes' from 'addr' into 'data'. Returns true on success.
  virtual bool Read(uint32_t addr, int bytes, uint8_t* data) {
    if (addr >= itcm_base_ && addr + bytes <= itcm_base_ + itcm_size_) {
      std::memcpy(data, &itcm_data_[addr - itcm_base_], bytes);
      return true;
    }
    if (addr >= dtcm_base_ && addr + bytes <= dtcm_base_ + dtcm_size_) {
      std::memcpy(data, &dtcm_data_[addr - dtcm_base_], bytes);
      return true;
    }
    return false;
  }

  bool Read(uint64_t addr, void* dest, size_t count) {
    return Read(static_cast<uint32_t>(addr), static_cast<int>(count), reinterpret_cast<uint8_t*>(dest));
  }

  // Writes 'bytes' from 'data' to 'addr'. Returns true on success.
  virtual bool Write(uint32_t addr, int bytes, const uint8_t* data) {
    if (addr >= itcm_base_ && addr + bytes <= itcm_base_ + itcm_size_) {
      std::memcpy(&itcm_data_[addr - itcm_base_], data, bytes);
      return true;
    }
    if (addr >= dtcm_base_ && addr + bytes <= dtcm_base_ + dtcm_size_) {
      std::memcpy(&dtcm_data_[addr - dtcm_base_], data, bytes);
      return true;
    }
    return false;
  }

  bool Write(uint64_t addr, const void* src, size_t count) {
    return Write(static_cast<uint32_t>(addr), static_cast<int>(count), reinterpret_cast<const uint8_t*>(src));
  }

  virtual void Eval() = 0;

  // Returns a string representation of the current memory profile boundaries.
  std::string GetProfileBounds() const {
    if (profile_ == "highmem") {
      return absl::StrFormat("[0x%08x - 0x%08x] (Unified ITCM)", itcm_base_, itcm_base_ + itcm_size_);
    } else {
      return absl::StrFormat("[0x%08x - 0x%08x] (ITCM) and [0x%08x - 0x%08x] (DTCM)", 
                             itcm_base_, itcm_base_ + itcm_size_,
                             dtcm_base_, dtcm_base_ + dtcm_size_);
    }
  }

  // Helper to calculate the length of the intersection of two ranges [a1, a2) and [b1, b2).
  static uint32_t GetRangeOverlap(uint32_t a1, uint32_t a2, uint32_t b1, uint32_t b2) {
    uint32_t start = std::max(a1, b1);
    uint32_t end = std::min(a2, b2);
    if (start < end) return end - start;
    return 0;
  }

  // Calculates the number of bytes by which an access at 'addr' with 'bytes' exceeds bounds.
  uint32_t GetOverflowDelta(uint32_t addr, int bytes) const {
    uint32_t end_addr = addr + bytes;
    uint32_t itcm_overlap = GetRangeOverlap(addr, end_addr, itcm_base_, itcm_base_ + itcm_size_);
    uint32_t dtcm_overlap = GetRangeOverlap(addr, end_addr, dtcm_base_, dtcm_base_ + dtcm_size_);
    return bytes - (itcm_overlap + dtcm_overlap);
  }

 protected:
  std::string profile_;
  uint32_t itcm_base_;
  uint32_t itcm_size_;
  uint32_t dtcm_base_;
  uint32_t dtcm_size_;
  std::vector<uint8_t> itcm_data_;
  std::vector<uint8_t> dtcm_data_;
  int pending_exit_code_;
};

#endif  // TESTS_VERILATOR_SIM_CORALNPU_MEMORY_IF_H_
