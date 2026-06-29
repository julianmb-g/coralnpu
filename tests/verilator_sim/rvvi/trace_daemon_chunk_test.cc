#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include "tests/verilator_sim/rvvi/trace_daemon.h"
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"

using namespace mpact::sim::riscv::rvvi;

class TraceDaemonChunkTest : public ::testing::Test {
protected:
    SpscRingBuffer<TracePacket, 4096> buffer;
    std::stringstream output;
    TraceDaemon daemon{&buffer, &output};
};

TEST_F(TraceDaemonChunkTest, IncompleteChunkSequenceDiscarded) {
    // 1. Setup a scenario with incomplete chunk sequence
    TracePacket p1 = {};
    p1.type = 'R';
    p1.reg.reg_type = 'V';
    p1.reg.total_size = 64; // Requires 2 chunks of 32
    p1.reg.offset = 32;     // Chunk 1 (offset 32)
    buffer.Push(p1);

    TracePacket p2 = {};
    p2.type = 'I'; // New instruction arrives before chunk 0
    buffer.Push(p2);

    // 2. Run the daemon
    daemon.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    daemon.Stop();

    // 3. The incomplete chunk (p1) should have been discarded and an error logged to stderr.
    // Since we are not capturing stderr, we rely on the fact that no register
    // update for the vector register should appear in the output.
    EXPECT_TRUE(output.str().find("v0:") == std::string::npos);
}
