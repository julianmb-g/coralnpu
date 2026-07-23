# Vector Backend Secondary Decode Stage (DE2)

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

> **Intended Audience:** Hardware Developers


The Vector Backend Secondary Decode Stage (DE2) is responsible for expanding decoded instructions (`LCMD_t`) from the command queue into executable micro-operations (`UOP_QUEUE_t`). It manages the structural boundaries between architectural instructions and the parallel micro-operation (uop) queue.

## Structural Layout

The DE2 stage is implemented across two primary components:

1. **`rvv_backend_decode_de2`**: The top-level wrapper that instantiates the decode units and the controller.
2. **`rvv_backend_decode_ctrl`**: The controller responsible for arbitrating uop dispatch and managing queue backpressure.

### NUM_DE_INST Scaling and Decoding

The architecture scales the number of parallel decode units based on the `NUM_DE_INST` parameter. For each instruction slot in the command queue, a dedicated `rvv_backend_decode_unit_de2` is instantiated.

- The unit dynamically expands an `lcmd` into a sequence of micro-operations.
- The first decode unit (`u_decode_unit0_de2`) tracks the remaining uop indices (`uop_index_remain`), while subsequent units initialize their index tracking to zero.

### NUM_DE_UOP Generation and Control

The `rvv_backend_decode_ctrl` module handles the complex logic of popping instructions from the command queue and pushing generated uops into the micro-operation queue.

- **Micro-operation Scaling**: The system generates up to `NUM_DE_UOP` parallel uops per cycle. A hard constraint ensures `NUM_DE_INST <= NUM_DE_UOP`.
- **Dynamic Routing**: A large combinatorial matrix evaluates the `de_uop_valid` signals across all instantiated instruction decoders to pack active uops contiguously into the `push` and `uop` buses.
- **Queue Backpressure**: The controller evaluates the `uq_ready` bitmask to ensure sufficient free slots in the uop queue. It pushes data only when space permits, preventing uop queue overflow.
- **Command Queue Management**: The `pop` signal for the command queue is asserted for an instruction slot only when its last micro-operation (`last_uop_valid`) has been successfully pushed to the uop queue.

## Trap and Flush Mechanisms

The DE2 stage receives a global `trap_flush_rvv` signal. When asserted, this clears the `uop_index_remain` state register (`uop_index_cdffr`), aborting any ongoing multi-uop expansion sequences and resynchronizing the pipeline state for exception handling.

## Interfaces

| Signal           | Direction | Width                              | Description                                                                    |
| :--------------- | :-------- | :--------------------------------- | :----------------------------------------------------------------------------- |
| `clk`            | Input     | 1-bit                              | Global clock signal.                                                           |
| `rst_n`          | Input     | 1-bit                              | Global active-low asynchronous reset signal.                                   |
| `lcmd_valid`     | Input     | `NUM_DE_INST` bits                 | Valid bitmask for incoming commands from the command queue.                    |
| `lcmd`           | Input     | `NUM_DE_INST` `LCMD_t` packets     | Command payloads containing decoded vector instructions.                       |
| `pop`            | Output    | `NUM_DE_INST` bits                 | Pop signals back to the command queue to clear instruction slots.              |
| `push`           | Output    | `NUM_DE_UOP` bits                  | Push validation bits for generated uops entering the micro-operation queue.    |
| `uop`            | Output    | `NUM_DE_UOP` `UOP_QUEUE_t` packets | Micro-operation payloads dispatching to the uop queue.                         |
| `uq_ready`       | Input     | `NUM_DE_UOP` bits                  | Ready status signals from the uop queue indicating available space.            |
| `trap_flush_rvv` | Input     | 1-bit                              | Global flush signal to reset secondary decode tracking registers during traps. |

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_decode_de2.sv`, `hdl/verilog/rvv/design/rvv_backend_decode_ctrl.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit f05a63aa421b1c7880e6fb2309e5e2c0e35607c3.
