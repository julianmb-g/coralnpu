<!--
 Copyright 2026 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
-->

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** HW Devs

# CoralNPU Microarchitecture

![image](../images/microarch.png)

## Pipeline

The CoralNPU base processor is an in-order three-stage pipeline capable of
dispatching up to 4 instructions per cycle. The instruction stages are:

* **Instruction fetch:** Instructions are fetched from memory into an
  instruction buffer.
* **Decode/Dispatch:** The first 4 instructions in the instruction buffer are
  decoded. Interlock and scoreboard logic determine which of the instructions can
  be dispatched this cycle. Instructions are forwarded to their respective
  execution units.
* **Execute/Writeback:** Execution units with dispatched instructions read
  operands from the register file and perform computation. Results can also be
  written back to the register file in the same cycle.

Some execution units can take multiple cycles to execute. Instruction latencies
can be found in the table below:

| Instruction Type | Latency (cycles) | Description            |
| ---------------- | ---------------- | ---------------------- |
| Alu              | 1                | Add, sub, xor, ...     |
| Csr              | 1                | CSR instructions       |
| Bru              | 1                | bge, jal, ebreak, ...  |
| Mlu              | 2                | mul, mulh, ...         |
| Dvu              | Variable         | div, rem, ...          |
| Lsu              | 2+               | lw, sw, ...            |

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** N/A
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
