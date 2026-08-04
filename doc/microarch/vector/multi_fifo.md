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

# MultiFifo hardware primitive

> **Intended Audience:** HW Devs, SW/Compiler Devs

## Overview

The `MultiFifo` hardware primitive (defined in `hdl/verilog/rvv/design/MultiFifo.sv`) provides a high-bandwidth decoupling queue capable of concurrently enqueuing and dequeuing multiple elements per cycle. This structure is critical for the Vector Backend, acting as the foundation for multi-issue instruction queues and wide data path buffers. It implements a circular buffer with parameterized capacity and data width.

## Parameters

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `T` | Type | `logic [7:0]` | Data type of the elements in the FIFO. |
| `N` | Int | `4` | Number of elements to enqueue/dequeue concurrently. |
| `MAX_CAPACITY` | Int | `16` | Total depth of the buffer. |

## Interfaces

| Signal | Direction | Description |
| :--- | :--- | :--- |
| `clk` | Input | System Clock. |
| `rstn` | Input | Active-low Asynchronous Reset. |
| `valid_in` | Input | Number of elements being enqueued in this cycle (0 to `N`). |
| `data_in` | Input | Data vector to be enqueued. |
| `fill_level` | Output | Current number of elements in the FIFO. |
| `data_out` | Output | Data vector available for dequeuing. |
| `ready_out` | Input | Number of elements being dequeued in this cycle (0 to `N`). |

## Architectural function

The `MultiFifo` tracks occupancy using a `fill_level` register, which is updated atomically based on the `valid_in` (enqueue) and `ready_out` (dequeue) signals.

- **Enqueue:** Data is written to the buffer based on the `head` pointer and the `valid_in` count.

- **Dequeue:** Data is read from the buffer based on the `tail` pointer and the `ready_out` count.

- **Backpressure:** The FIFO enforces capacity constraints via assertions in simulation:

  - Enqueue must not exceed `MAX_CAPACITY - fill_level`.
  - Dequeue must not exceed `fill_level`.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** [hdl/verilog/rvv/design/MultiFifo.sv](../../../hdl/verilog/rvv/design/MultiFifo.sv) - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
