# Core Pipeline Wrapper

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

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

> **Intended Audience:** Hardware Developers

The `Core` module (`hdl/chisel/src/coralnpu/Core.scala`) is the primary top-level pipeline integration wrapper. It physically instantiates the base scalar core (`SCore`) and, optionally, the vector execution backend (`RvvCore`) based on the `enableRvv` configuration parameter.

This module acts as the final hardware boundary before any bus protocol adaptation (such as AXI4) occurs. It exposes the raw internal data buses and control lines directly to the external wrappers.

## Subsystem Instantiation

- **SCore**: The baseline scalar integer execution pipeline and fetch unit. Always instantiated.
- **RvvCore**: The vector processor backend. Conditionally instantiated and directly wired to `SCore.io.rvvcore`.

## Physical Interfaces

The `Core` module exposes native, un-adapted interfaces.

### Memory Buses

| Port   | Direction | Description                                                                                       |
| :----- | :-------- | :------------------------------------------------------------------------------------------------ |
| `ibus` | In/Out    | Instruction Bus (Fetch requests and responses). Directly passes through from `SCore.io.ibus`.     |
| `dbus` | In/Out    | Data Bus (Local memory/LSU requests and responses). Directly passes through from `SCore.io.dbus`. |
| `ebus` | In/Out    | External Bus (System memory and peripheral access). Directly passes through from `SCore.io.ebus`. |

### Interrupts and Control

| Port           | Direction | Description                                                         |
| :------------- | :-------- | :------------------------------------------------------------------ |
| `irq`          | Input     | External interrupt request line, passed to `SCore.io.irq`.          |
| `timer_irq`    | Input     | Timer interrupt request line, passed to `SCore.io.timer_irq`.       |
| `software_irq` | Input     | Software interrupt request line, passed to `SCore.io.software_irq`. |
| `debug_req`    | Input     | Debug request line.                                                 |
| `halted`       | Output    | Indicates the core is in a halted state (`SCore.io.halted`).        |
| `fault`        | Output    | Indicates a hardware fault condition (`SCore.io.fault`).            |
| `wfi`          | Output    | Wait For Interrupt state signal (`SCore.io.wfi`).                   |

### Additional Interfaces

- **`csr`**: Direct input/output mapping to the internal Control and Status Registers (`SCore.io.csr`).
- **`dm`**: Core Debug Module interface (`SCore.io.dm`).
- **`iflush` / `dflush`**: Instruction and Data cache flush signaling interfaces.

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

**Provenance & Traceability** - **Verified As Of:** 2026-07-06 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/coralnpu/Core.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
