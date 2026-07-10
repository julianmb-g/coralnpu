# MultiFifo Hardware Primitive

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** Hardware Developers, Compiler Engineers

## Overview

The `MultiFifo` hardware primitive (defined in `hdl/verilog/rvv/design/MultiFifo.sv`) provides a high-bandwidth decoupling queue capable of concurrently enqueuing and dequeuing multiple elements per cycle. This structure is critical for the Vector Backend, acting as the foundation for multi-issue instruction queues and wide data path buffers.

## Parameters

| Parameter        | Type   | Description                                                                              |
| :--------------- | :----- | :--------------------------------------------------------------------------------------- |
| `T`              | `type` | The data type of the elements (default `logic [7:0]`).                                   |
| `N`              | `int`  | The maximum number of elements that can be enqueued or dequeued in a single clock cycle. |
| `MAX_CAPACITY`   | `int`  | The total element capacity of the internal buffer array.                                 |
| `INTERFACE_BITS` | `int`  | The bit-width for count signals (derived as `$clog2(N+1)`).                              |
| `CAPACITYBITS`   | `int`  | The bit-width for buffer pointers (derived as `$clog2(MAX_CAPACITY+1)`).                 |

## Interfaces

| Port         | Direction | Type                         | Description                                                                                      |
| :----------- | :-------- | :--------------------------- | :----------------------------------------------------------------------------------------------- |
| `clk`        | Input     | `logic`                      | Clock signal.                                                                                    |
| `rstn`       | Input     | `logic`                      | Active-low reset signal.                                                                         |
| `valid_in`   | Input     | `logic [INTERFACE_BITS-1:0]` | The number of elements the producer is actively enqueuing this cycle.                            |
| `data_in`    | Input     | `T [N-1:0]`                  | The array of elements to be enqueued. Only elements up to `valid_in` are written.                |
| `fill_level` | Output    | `logic [CAPACITYBITS-1:0]`   | The current number of elements in the FIFO. Used by external logic to calculate available space. |
| `data_out`   | Output    | `T [N-1:0]`                  | The array of elements available to be dequeued.                                                  |
| `ready_out`  | Input     | `logic [INTERFACE_BITS-1:0]` | The number of elements the consumer is actively dequeuing this cycle.                            |

## Wrap-Around Pointer Architecture

The `MultiFifo` maintains state via three primary registers:

- `head`: The index pointing to the next available write slot.
- `tail`: The index pointing to the oldest valid read slot.
- `m_fill_level`: The running count of valid elements.

Because `valid_in` and `ready_out` are variable counts rather than binary signals, the queue utilizes a combinatorial `WrapAroundSum` function. This function adds the variable element count to the current pointer and performs a modulo operation against `MAX_CAPACITY` to handle circular buffer wrapping within a single cycle.

## Backpressure Constraints

Unlike a standard decoupled queue that manages its own `ready` signals, `MultiFifo` relies on the external producer and consumer to respect its capacity boundaries using the exposed `fill_level`. The hardware enforces the following strict assertions:

- **Producer Constraint:** `valid_in <= MAX_CAPACITY - fill_level` (Cannot overflow the buffer).
- **Consumer Constraint:** `ready_out <= fill_level` (Cannot underflow the buffer).

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/MultiFifo.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
