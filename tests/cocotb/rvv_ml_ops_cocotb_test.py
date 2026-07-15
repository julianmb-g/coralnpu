"""Test suite for RVV ML operations using Cocotb.

This file contains testbenches to verify matrix multiplication operations
accelerated by RISC-V Vector (RVV) instructions on the Coral NPU.
It tests both integer (int8) and floating-point (float32) variants,
using both C intrinsics and raw assembly implementations.

The tests generate random input data, compute the expected result using NumPy,
load the corresponding ELF file onto the simulated core, and verify that the
hardware execution matches the software reference.
"""
import cocotb
import numpy as np
import argparse
def golden_flash_attention(q, k, v):
    """NumPy Golden Reference for FlashAttention."""
    # S = Q * K^T
    d = q.shape[-1]
    scores = np.matmul(q, k.T) / np.sqrt(d)
    # Safe Softmax
    m = np.max(scores, axis=-1, keepdims=True)
    p = np.exp(scores - m)
    p /= np.sum(p, axis=-1, keepdims=True)
    # O = P * V
    return np.matmul(p, v)


@cocotb.test()
async def core_mini_rvv_flashattention_test(dut):
    """
    Injects the FlashAttention RVV kernel into the Coral NPU, 
    feeds it test matrices, and verifies the output.
    """
    r = runfiles.Create()
    fixture = await Fixture.Create(dut)
    rng = np.random.default_rng(seed=42)

    # 1. THE INJECTION: Locate and load the compiled C++ ELF binary
    elf_name = "rvv_flashattention_test.elf"
    elf_path = r.Rlocation(f"coralnpu_hw/tests/cocotb/rvv/ml_ops/{elf_name}")

    await fixture.load_elf_and_lookup_symbols(
        elf_path, ["q_buf", "k_buf", "v_buf", "o_buf", "csr_cycle_count"]
    )
    # 2. DATA GENERATION: Define dimensions
    seq_len_val = 32
    d_val = 32

    # Generate test data (scaled between -1 and 1 to prevent massive exponentials)
    q_data = rng.uniform(-1, 1, (seq_len_val, d_val)).astype(np.float32)
    k_data = rng.uniform(-1, 1, (seq_len_val, d_val)).astype(np.float32)
    v_data = rng.uniform(-1, 1, (seq_len_val, d_val)).astype(np.float32)

    # 1. HOLD IN RESET
    await fixture.core_mini_axi.reset()

    # 3. WRITE MATRICES
    await fixture.write("q_buf", q_data.flatten())
    await fixture.write("k_buf", k_data.flatten())
    await fixture.write("v_buf", v_data.flatten())
    await fixture.write("o_buf", np.zeros_like(q_data).flatten())

    # 4. UNPAUSE AND EXECUTE
    await fixture.run_to_halt(timeout_cycles=2000000)

    csr_cycle_count = (await
                       fixture.read_word('csr_cycle_count')).view(np.uint32)[0]

    log_matmul_metrics(
        dut,
        f"core_mini_rvv_flashattention_{seq_len_val}x{d_val}",
        csr_cycle_count,
        lhs_rows=2 * seq_len_val,
        rhs_cols=d_val,
        inner=seq_len_val
    )

    # 5. READBACK & VERIFICATION
    num_bytes = seq_len_val * d_val * 4  # 4 bytes per FP32
    actual_packed = await fixture.read("o_buf", num_bytes)
    actual_output = actual_packed.view(np.float32).reshape(seq_len_val, d_val)

    expected_output = golden_flash_attention(q_data, k_data, v_data)

    debug_msg = (
        f"Flash Attention mismatch!\n"
        f"Expected (first row): {expected_output[0][:4]}...\n"
        f"Actual (first row):   {actual_output[0][:4]}..."
    )

    # Assert with a slight tolerance due to the software exponential approximation
    np.testing.assert_allclose(
        actual_output,
        expected_output,
        rtol=1e-3,
        atol=1e-3,
        err_msg=debug_msg
    )
