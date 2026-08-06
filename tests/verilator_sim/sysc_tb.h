// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the \"License\");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an \"AS IS\" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TESTS_VERILATOR_SIM_SYSC_TB_H_
#define TESTS_VERILATOR_SIM_SYSC_TB_H_

// A SystemC baseclass for constrained random testing of Verilated RTL.
#include <systemc.h>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

#include "absl/strings/string_view.h"
#include "fifo.h"
// sc_core needs to be included before verilator header
using namespace sc_core;      // NOLINT(build/namespaces)
#include "verilated_fst_c.h"  // NOLINT(build/include_subdir): From verilator.

using sc_dt::sc_bv;

#define BIND(a, b) a.b(b)
#define BIND2(a, b, c) \
  BIND(a, c);          \
  BIND(b, c)

template <typename T>
class ScSignalVrb {
 public:
  sc_signal<bool> valid;
  sc_signal<bool> ready;
  sc_signal<T> bits;
};

template <typename T>
class ScInVrb {
 public:
  sc_in<bool> valid;
  sc_out<bool> ready;
  sc_in<T> bits;

  void bind(sc_signal<bool> &v, sc_signal<bool> &r, sc_signal<T> &b) {
    valid.bind(v);
    ready.bind(r);
    bits.bind(b);
  }

  void bind(ScSignalVrb<T> &vrb) {
    valid.bind(vrb.valid);
    ready.bind(vrb.ready);
    bits.bind(vrb.bits);
  }

  void operator()(sc_signal<bool> &v, sc_signal<bool> &r, sc_signal<T> &b) {
    bind(v, r, b);
  }

  void operator()(ScSignalVrb<T> &vrb) { bind(vrb); }
};

template <typename T>
class ScOutVrb {
 public:
  sc_out<bool> valid;
  sc_in<bool> ready;
  sc_out<T> bits;

  void bind(sc_signal<bool> &v, sc_signal<bool> &r, sc_signal<T> &b) {
    valid.bind(v);
    ready.bind(r);
    bits.bind(b);
  }

  void bind(ScSignalVrb<T> &vrb) {
    valid.bind(vrb.valid);
    ready.bind(vrb.ready);
    bits.bind(vrb.bits);
  }

  void operator()(sc_signal<bool> &v, sc_signal<bool> &r, sc_signal<T> &b) {
    bind(v, r, b);
  }

  void operator()(ScSignalVrb<T> &vrb) { bind(vrb); }
};

// eg. struct message : Base {...};
struct Base {
  inline bool operator==(const Base &rhs) const { return false; }

  inline friend std::ostream &operator<<(std::ostream &os, Base const &v) {
    return os;
  }
};

// Base class for testbench {posedge & negedge}.
class SyscTb : public sc_module {
 public:
  sc_clock clock;
  sc_signal<bool> reset;
  sc_signal<bool> resetn;

  SC_HAS_PROCESS(SyscTb);

  SyscTb(sc_module_name n, int loops, bool random = true)
      : sc_module(n),
        clock("clock", 1, SC_NS),
        reset("reset"),
        resetn("resetn"),
        random_(random),
        loops_(loops),
        started_(false) {
    loop_ = 0;
    error_ = false;

    clock_(clock);

    SC_METHOD(tb_posedge);
    sensitive << clock_.pos();

    SC_METHOD(tb_negedge);
    sensitive << clock_.neg();

    SC_METHOD(tb_stop);
    sensitive << clock_.neg();

    // Verilated::commandArgs(argc, argv);
    tf_ = new VerilatedFstC;
  }

  bool Started() const { return started_; }

  ~SyscTb() {
    if (tf_) {
      tf_->dump(sim_time_);  // last falling edge
      tf_->close();
      delete tf_;
      tf_ = nullptr;
    }
    if (error_) {
      exit(23);
    }
  }

  void Start(bool trace, int reset_cycles) {
    Init();

    if (trace) {
      // Placeholder for trace setup, will be integrated with trace()
    }

    // Reset sequence
    reset = 1;
    resetn = 0;
    for (int i = 0; i < reset_cycles; ++i) {
      sc_start(5, SC_NS);  // Posedge
      sc_start(5, SC_NS);  // Negedge
    }
    reset = 0;
    resetn = 1;
    sc_start(5, SC_NS);  // Falling edge to ensure deassertion is seen.

    started_ = true;
    sc_start();

    if (tf_) {
      tf_->dump(sim_time_++);  // last falling edge
    }
  }

  void Start() { Start(false, 5); } // Default to no trace, 5 reset cycles

  template <typename T>
  void Trace(T* design, const char *name = "") {
    if (!strlen(name)) {
      name = design->name();
    }
    std::string path = std::string("/tmp/") + name;

    reset = 1;
    resetn = 0;
    sc_start(SC_ZERO_TIME);
    reset = 0;
    resetn = 1;

    design->trace(tf_, 99);
    path += ".fst";
    Verilated::traceEverOn(true);
    tf_->open(path.c_str());
    printf("\nInfo: default timescale unit used for tracing: 1 ps (%s)\n",
           path.c_str());
  }

  static absl::string_view GetName(absl::string_view s) {
    size_t pos = s.find_last_of('/');
    return (pos == absl::string_view::npos) ? s : s.substr(pos + 1);
  }

 protected:
  virtual void Init() {}
  virtual void Posedge() {}
  virtual void Negedge() {}

  bool Check(bool v, const char *s = "") {
    const char *KRED = "\x1B[31m";
    const char *KRST = "\033[0m";
    if (!v) {
      sc_stop();
      printf("%s", KRED);
      if (strlen(s)) {
        printf("***ERROR[%s]::VERIFY \"%s\"\n", this->name(), s);
      } else {
        printf("***ERROR[%s]::VERIFY\n", this->name());
      }
      printf("%s", KRST);
      error_ = true;
    }
    return v;
  }

  bool SyscTbRandBool() {
    // Do not allow any 'io_in_valid' controls to be set during reset.
    return !reset &&
           (!random_ || (rand() & 1));  // NOLINT(runtime/threadsafe_fn)
  }

  // Generates a number on the range [min, max].
  int RandInt(int min = 0, int max = (1 << 31)) {
    return (rand() % (max - min + 1)) + min;  // NOLINT(runtime/threadsafe_fn)
  }

  uint32_t RandUint32(uint32_t min = 0, uint32_t max = 0xffffffffu) {
    uint32_t r = (rand() & 0xffff) |  // NOLINT(runtime/threadsafe_fn)
                 (rand() << 16);      // NOLINT(runtime/threadsafe_fn)
    if (min == 0 && max == 0xffffffff) return r;
    return (r % (max - min + 1)) + min;
  }

  uint64_t RandUint64(uint64_t min = 0, uint64_t max = 0xffffffffffffffffull) {
    uint64_t r = RandUint32() | (uint64_t(RandUint32()) << 32);
    if (min == 0 && max == 0xffffffffffffffffull) return r;
    return (r % (max - min + 1)) + min;
  }

  uint32_t Cycle() {
    return sim_time_ / 2;  // posedge + negedge
  }

 private:
  const bool random_;
  const int loops_;
  int loop_;
  bool error_;
  bool started_;

  sc_in<bool> clock_;

  uint32_t sim_time_ = 0;
  VerilatedFstC *tf_ = nullptr;

  void tb_posedge() {
    if (tf_ && started_) { tf_->dump(sim_time_); tf_->flush(); }
    sim_time_++;
    if (reset) return;
    Posedge();
  }

  void tb_negedge() {
    if (tf_ && started_) { tf_->dump(sim_time_); tf_->flush(); }
    sim_time_++;
    if (reset) return;
    Negedge();
  }

  void tb_stop() {
    // LessThanEqual for one more edge (end - start + 1).
    if (loop_ <= loops_) {
      loop_++;
    } else {
      printf("\nInfo: loop limit \"%d\" reached\n", loops_);
      sc_stop();
    }
  }
};

#endif  // TESTS_VERILATOR_SIM_SYSC_TB_H_
