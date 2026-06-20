#include "tests/verilator_sim/rvvi/mpact_trace_formatter.h"
#include "gtest/gtest.h"

using namespace mpact::sim::riscv::rvvi;

TEST(MpactTraceFormatterTest, DelegatesToFallback) {
  MpactTraceFormatter formatter;
  // Test with a known NOP instruction (0x00000013)
  // CustomFallbackFormatter returns "nop" for this.
  std::string result = formatter.Disassemble(0x00000013);
  EXPECT_EQ(result, "nop");
}
