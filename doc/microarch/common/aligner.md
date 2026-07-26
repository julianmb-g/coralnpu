# Aligner

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


The `Aligner` is a generic hardware utility module used to compress an array of input data by packing all valid elements toward the lower indices (the "front") of the output array, eliminating any invalid gaps. It preserves the original relative ordering of the valid elements.

## Architectural Function

In vectorized or superscalar data paths, operations often result in sparse validity masks (e.g., after predicated execution or filtering). The `Aligner` reorganizes these sparse arrays into dense, contiguous blocks.

**Behavioral Example:**

- **Inputs:**
  - `valid_in` = `[0, 1, 0, 1]`
  - `data_in` = `[A, B, C, D]`
- **Outputs:**
  - `valid_out` = `[1, 1, 0, 0]`
  - `data_out` = `[B, D, X, X]` (where X is "don't care")

### Implementation Details

The underlying SystemVerilog (`Aligner.sv`) operates by computing a parallel prefix sum (population count) of the `valid_in` signals.

- For each input index `i`, `valid_count[i]` determines the target output index `o` where `data_in[i]` should be routed if it is valid.
- The output multiplexers use this computed index to select the appropriate input data for each output slot.

## Interfaces

The `Aligner` is instantiated as a parameterized component over a data type `T` (with a specific bit width) and an array size `N`. In Chisel, it is exposed as a `BlackBox` that generates the appropriate Verilog wrapper.

| Port Name   | Direction | Type            | Description                                           |
| :---------- | :-------- | :-------------- | :

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/common/Aligner.scala:L53`, `hdl/verilog/rvv/design/Aligner.sv:L19` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
