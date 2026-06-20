#pragma once
class VerilatedFstC {
 public:
  void open(const char*) {}
  void close() {}
  void dump(int) {}
  void flush() {}
};
namespace Verilated {
  inline void traceEverOn(bool) {}
}