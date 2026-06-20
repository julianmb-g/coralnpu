#ifndef TESTS_VERILATOR_SIM_CORALNPU_VCOREVERIFICATION_H_
#define TESTS_VERILATOR_SIM_CORALNPU_VCOREVERIFICATION_H_
// Mock VCoreVerification.h
#include <cstdint>
struct VCoreVerification {
  struct {
    uint32_t io_debug_port;
  } io;
};
#endif  // TESTS_VERILATOR_SIM_CORALNPU_VCOREVERIFICATION_H_
