#include "tests/verilator_sim/rvvi/mpact_trace_formatter.h"
#include "gtest/gtest.h"

using namespace coralnpu::sim::rvvi;

TEST(MpactTraceFormatterTest, DelegatesToFallback) {
  MpactTraceFormatter formatter;
  std::string result = formatter.Disassemble(0x00000013);
  EXPECT_EQ(result, "<unimplemented: mpact-riscv missing>");
}
