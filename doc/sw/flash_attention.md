# Multi-Head FlashAttention RVV Kernel

> **Intended Audience:** SW/Compiler Developers, HW Co-design Engineers

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

This document describes the design, mathematical formulation, and vectorization strategy of the optimized Multi-Head FlashAttention software kernel implemented on the CoralNPU's [Vector Core (RVV)](../microarch/vector/rvv.md).

---

## 1. Algorithmic Overview

FlashAttention is an memory-efficient attention mechanism that avoids materializing the massive $N \times N$ attention matrix in external DRAM. Instead, it computes attention incrementally in on-chip SRAM ([TCM](../microarch/memory/tcm.md)) using tiled or row-by-row reductions and online Softmax calculation.

The FlashAttention kernel implemented on the CoralNPU processes Query ($Q$), Key ($K$), Value ($V$), and Output ($O$) matrices. The kernel is structured around multi-head execution, unrolled dot-product scaling, vectorized stable Softmax, and vector-register accumulation of the weighted values.

### Mathematical Formulation

For each head $h$, row $q$ (representing a single query vector of size $d = \text{dim}$), and sequence length $S = \text{s\_len}$:

1. **Dot Product**: $S_i = \frac{1}{\sqrt{d}} \sum_{j=1}^{d} Q_{q, j} \cdot K_{i, j} \quad \text{for } i \in [0, S-1]$
2. **Stable Softmax**:
   - Max extraction: $m = \max_{i} S_i$
   - Exponentiation with numerical offset: $P_i = e^{S_i - m}$
   - Tally sum: $d_{\text{tally}} = \sum_{i} P_i$
   - Probability normalization: $P_i = \frac{P_i}{d_{\text{tally}}}$
3. **Value Accumulation**: $O_q = \sum_{i=1}^{S} P_i \cdot V_i$

---

## 2. API Signature & Interface

The execution entry point is defined as a standard C-runtime ABI function:

```c
extern "C" void FlashAttentionRVV(
    const float* q_matrix,       // Pointer to Query matrix [num_heads * s_len * dim]
    const float* k_matrix,       // Pointer to Key matrix [num_heads * s_len * dim]
    const float* v_matrix,       // Pointer to Value matrix [num_heads * s_len * dim]
    float* o_matrix,             // Pointer to Output matrix [num_heads * s_len * dim]
    size_t num_heads,     // Number of distinct attention heads
    size_t s_len,         // Sequence length (max 256 for local stack buffering)
    size_t dim            // Head dimension (vector length size)
);
```

### Parameter Context

- **Matrix Layout**: Layout is row-major. Each head's data of size `s_len * dim` is stored sequentially. The stride to transition to the next head is `s_len * dim`.
- **Stack Buffering Limits**: The intermediate Softmax scores $S_i$ are accumulated and kept in a local stack buffer (`s_buf`) of size 256. The sequence length `s_len` must be $\le 256$ to prevent stack buffer overflow.

---

## 3. Microarchitectural Vectorization Strategy

The CoralNPU Vector Core achieves high throughput by pairing aggressive register grouping (LMUL) with hardware-assisted vector reductions.

### 3.1. Register Grouping (`LMUL = 8`)

To maximize vector width and execution efficiency on wide physical vector lanes, the kernel employs **`m8` vector grouping** (e.g., `vfloat32m8_t` representing 8 combined vector registers):

- `q_vec` ($Q$-row) is loaded as a single `vfloat32m8_t` and remains resident in the vector register file across all dot-product steps for a given query row.
- $K$ and $V$ rows are iteratively loaded as `vfloat32m8_t` register groups. This maximizes execution efficiency on configurations with large dimensions.
- Reduces loop overhead and memory instruction dispatch frequency.

### 3.2. Scalar-to-Vector Reduction (using `LMUL = 1` scalars)

- Vector sum reductions (`vfredusum`) and maximum reductions (`vfredmax`) reduce active elements from `m8` source registers down to a single element in an `m1` register group.
- This cross-lane reduction utilizes the hardware's internal logarithmic reduction tree in the [Vector Permutation/Reduction Unit (PMT/RDT)](../microarch/vector/pmtrdt.md), minimizing pipeline stalls.

---

## 4. Operational Breakdown

### 4.1. Fast Scaled Dot-Product Engine (Unrolled loop)

To hide SRAM/TCM loading latency and fully saturate the arithmetic pipeline, the dot product of $Q$ and $K^T$ is unrolled by a factor of 2.

```c
for (; kv_idx <= s_len - 2; kv_idx += 2) {
  auto k_vec0 = __riscv_vle32_v_f32m8(k_head + (kv_idx * dim), vl);
  auto k_vec1 = __riscv_vle32_v_f32m8(k_head + ((kv_idx + 1) * dim), vl);

  auto vacc0 = __riscv_vfmul_vv_f32m8(q_vec, k_vec0, vl);
  auto vacc1 = __riscv_vfmul_vv_f32m8(q_vec, k_vec1, vl);

  float s0 = __riscv_vfmv_f_s_f32m1_f32(__riscv_vfredusum_vs_f32m8_f32m1(vacc0, v_zero, vl));
  float s1 = __riscv_vfmv_f_s_f32m1_f32(__riscv_vfredusum_vs_f32m8_f32m1(vacc1, v_zero, vl));

  s_buf[kv_idx] = s0 * scale;
  s_buf[kv_idx + 1] = s1 * scale;
}
```

- **Arithmetic Operations**: Hardware performs element-wise multiplication on the Vector FPU, followed by a scalar reduction.
- **Latency Hiding**: Loading `k_vec0` and `k_vec1` back-to-back leverages dual-port TCM streaming and dual-issue capability, overlapping memory load wait states with FPU execution cycles.

### 4.2. Vectorized Exponential Function (`rvv_exp_f32m8`)

The natural exponential function $e^x$ is evaluated natively on the Vector Core using a high-precision, optimized approximation:
$$e^x = 2^{x \cdot \log_2 e} = 2^{y} = 2^{i + f}$$
Where $i = \text{round}(y)$ is an integer and $f = y - i$ is the fractional part ($|f| \le 0.5$).

1. **Underflow Protection**: Input $x$ is clipped against underflow bounds via `vfmax.vv` with a lower threshold of $-88.0f$ (since single-precision floating point underflows below $e^{-88}$).
2. **Format Splitting**:
   - $y = x \cdot \log_2 e \quad (\log_2 e \approx 1.44269504f)$
   - $i = \lfloor y \rceil$ is obtained via float-to-integer conversion (`vfcvt.x.f.v`).
   - $f = x - (i \cdot \ln 2) \quad (\ln 2 \approx 0.69314718f)$.
3. **Polynomial Evaluation**: $e^f$ is approximated using a 3rd-order Taylor series:
   $$e^f \approx 1 + f + \frac{f^2}{2!} + \frac{f^3}{3!} = 1 + f \left(1 + f \left(\frac{1}{2} + \frac{1}{6} f\right)\right)$$
   This is executed using sequential Multiply-Accumulate (`vfmacc`) instructions.
4. **Exponent Assembly**: The scaling factor $2^i$ is assembled at the bit level to avoid floating-point multiplication loops.
   - The bias of $127$ is added to $i$.
   - The biased exponent is left-shifted by $23$ bits to occupy the exponent field of a single-precision IEEE 754 float.
   - The resulting integer vector is bitwise reinterpreted (`vreinterpret`) directly as a float vector, creating $2^i$.
5. **Final Recomposition**: The polynomial output $e^f$ is multiplied by the reconstructed $2^i$ to deliver the final value $e^x$.

### 4.3. Numerically Stable Softmax

Softmax computes the normalized probabilities over the sequence length `s_len`.

- **Stable Shift**: To prevent floating-point overflow during exponentiation, the maximum value $m$ of the entire row is located using a vector reduction `vfredmax`. $m$ is subtracted from every element in the sequence:
  $$S^{\prime}_i = S_i - m$$
- **Probability Normalization**: The exponent of the shifted scores is computed using `rvv_exp_f32m8`. These scores are summed to obtain $d_{\text{tally}}$, and every exponent is divided by $d_{\text{tally}}$ (`vfdiv.vf`) to yield the finalized Softmax probability vector $P$.

### 4.4. Accumulation & Weighted Sum Extraction

The final step computes the weighted sum of Value vectors $V$ based on the calculated Softmax probabilities.

- **Zero-Skipping Optimization**: Since Softmax scores can be highly sparse (containing many near-zero values), the loop checks each probability value `p_val`:

  ```c
  for (size_t kv_idx = 0; kv_idx < s_len; kv_idx++) {
    float p_val = s_buf[kv_idx];
    if (p_val == 0.0f) continue;

    auto v_v = __riscv_vle32_v_f32m8(v_head + (kv_idx * dim), vl);
    v_o = __riscv_vfmacc_vf_f32m8(v_o, p_val, v_v, vl);
  }
  ```

  If `p_val == 0.0f`, the corresponding $V$ vector load and its associated Multiply-Accumulate operations are skipped, bypassing DRAM/TCM load latency and FPU active cycles.

- **SRAM Single-Write Coalescing**: The accumulated vector register `v_o` is written to Output memory `o_head` exactly once per row, minimizing outbound bus transaction overhead.

---

## 5. Performance and Resource Mapping

| Operation / Stage  | Primary Vector Instructions                     | Hardware Units Engaged   | Throughput / Bottleneck                                      |
| :----------------- | :---------------------------------------------- | :----------------------- | :----------------------------------------------------------- |
| **Dot Product**    | `vle32.v`, `vfmul.vv`, `vfredusum.vs`           | LSU, Vector FPU, RDT/PMT | Limited by TCM load bandwidth and reduction tree latency.    |
| **Exp Evaluation** | `vfmax.vv`, `vfmul.vv`, `vfsub.vv`, `vfmacc.vv` | Vector FPU               | Purely ALU-bound; maps to Vector FALU with full pipelining.  |
| **Bit assembly**   | `vadd.vv`, `vsll.vx`, `vreinterpret`            | Vector ALU               | Single-cycle logical throughput. No floating-point overhead. |
| **Weighted Sum**   | `vle32.v`, `vfmacc.vf`                          | LSU, Vector FPU          | Bandwidth-limited. Completely optimized by zero-skipping.    |

<!-- mdformat off -->

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `tests/cocotb/rvv/ml_ops/rvv_flashattention_kernel.cc` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
