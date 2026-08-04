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

# DMA engine software guide

> **Intended Audience:** SW/Compiler Devs

## Overview

The DMA engine offloads bulk data transfers. It supports memory-to-memory, memory-to-peripheral, and peripheral-to-memory transfers via descriptor chains. CPU builds descriptors, programs DMA, and polls for completion.

## Register map

[Source: `hdl/chisel/src/bus/DmaEngine.scala` (RegMap definition)]

Base address: `0x40050000`

| Offset | Name        | Access | Reset | Description |
|--------|-------------|--------|-------|-------------|
| 0x00   | CTRL        | RW     | 0x00  | `[0]` enable, `[1]` start (write-1-to-set, self-clearing), `[2]` abort |
| 0x04   | STATUS      | RO     | 0x00  | `[0]` busy, `[1]` done, `[2]` error, `[7:4]` error_code |
| 0x08   | DESC_ADDR   | RW     | 0x00  | Address of first descriptor |
| 0x0C   | CUR_DESC    | RO     | 0x00  | Address of current descriptor |
| 0x10   | XFER_REMAIN | RO     | 0x00  | Bytes remaining in current transfer |

### Status error codes

| Code | Meaning |
|------|---------|
| 0    | No error |
| 1    | Descriptor fetch error |
| 2    | Poll read error |
| 3    | Data read error |
| 4    | Data write error |
| 5    | Abort |

## Descriptor format

Descriptors must be **32-byte aligned** in DMA-accessible memory (SRAM or DDR). Size: 32 bytes:

```text

Offset  Field        Bits      Description
0x00    src_addr     [31:0]    Source address
0x04    dst_addr     [31:0]    Destination address
0x08    len_flags    [23:0]    Transfer length in bytes
                     [26:24]   Beat width: log2(bytes) — 0=1B, 1=2B, 2=4B
                     [27]      src_fixed: source address does not increment
                     [28]      dst_fixed: destination address does not increment
                     [29]      poll_en: enable flow-control polling
                     [31:30]   Reserved
0x0C    next_desc    [31:0]    Next descriptor address (0 = end of chain)
0x10    poll_addr    [31:0]    Address to poll before each beat
0x14    poll_mask    [31:0]    Bitmask for poll comparison
0x18    poll_value   [31:0]    Expected value after masking
0x1C    reserved     [31:0]    Must be 0

```

### C descriptor structure

```c

struct __attribute__((packed, aligned(32))) dma_descriptor {
  uint32_t src_addr;
  uint32_t dst_addr;
  uint32_t len_flags;
  uint32_t next_desc;
  uint32_t poll_addr;
  uint32_t poll_mask;
  uint32_t poll_value;
  uint32_t reserved;
};

```

### Building len_flags

```c

#define DMA_DESC_LEN_MASK        0xFFFFFF
#define DMA_DESC_WIDTH_MASK      0x7
#define DMA_DESC_WIDTH_SHIFT     24
#define DMA_DESC_SRC_FIXED_SHIFT 27
#define DMA_DESC_DST_FIXED_SHIFT 28
#define DMA_DESC_POLL_EN_SHIFT   29

static inline uint32_t make_len_flags(uint32_t len, uint32_t width_log2,
                                       int src_fixed, int dst_fixed,
                                       int poll_en) {
  return (len & DMA_DESC_LEN_MASK) |
         ((width_log2 & DMA_DESC_WIDTH_MASK) << DMA_DESC_WIDTH_SHIFT) |
         ((src_fixed ? 1u : 0u) << DMA_DESC_SRC_FIXED_SHIFT) |
         ((dst_fixed ? 1u : 0u) << DMA_DESC_DST_FIXED_SHIFT) |
         ((poll_en ? 1u : 0u) << DMA_DESC_POLL_EN_SHIFT);
}

```

## Programming sequence

```c

#define REG32(addr) (*(volatile uint32_t*)(addr))
#define DMA_BASE       0x40050000
#define DMA_CTRL       (DMA_BASE + 0x00)
#define DMA_STATUS     (DMA_BASE + 0x04)
#define DMA_DESC_ADDR  (DMA_BASE + 0x08)

#define DMA_CTRL_ENABLE (1 << 0)
#define DMA_CTRL_START  (1 << 1)
#define DMA_CTRL_ABORT  (1 << 2)

#define DMA_STATUS_DONE  (1 << 1)
#define DMA_STATUS_ERROR (1 << 2)

// 1. Build descriptor(s) in memory

desc->src_addr  = src;
desc->dst_addr  = dst;
desc->len_flags = make_len_flags(nbytes, 2 /* 4-byte beats */, 0, 0, 0);

desc->next_desc = 0;  // single descriptor

// 2. Program and start DMA

REG32(DMA_DESC_ADDR) = (uint32_t)desc;
asm volatile("fence" ::: "memory");  // ensure descriptor is in memory before start
REG32(DMA_CTRL) = (DMA_CTRL_ENABLE | DMA_CTRL_START);

// 3. Wait for completion

while (!(REG32(DMA_STATUS) & DMA_STATUS_DONE)) {}  // poll done bit

// 4. Check for errors

if (REG32(DMA_STATUS) & DMA_STATUS_ERROR) {
  // error occurred, check error_code in bits [7:4]
}

```

## Transfer modes

### Memory-to-memory

Standard bulk copy. Source and destination addresses increment.

```c

desc->len_flags = make_len_flags(nbytes, 2, 0, 0, 0);

```

### Memory-to-peripheral (fixed destination)

Source increments, destination fixed. Used for writing to peripheral FIFOs or data registers.

```c

#define SPI_TXDATA_REG 0x40020008

desc->src_addr  = (uint32_t)sram_buffer;
desc->dst_addr  = SPI_TXDATA_REG;  // e.g., SPI TXDATA register
desc->len_flags = make_len_flags(nbytes, 2, 0, 1, 0);  // dst_fixed=1

```

### Peripheral-to-memory (fixed source)

Source fixed, destination increments. Used for draining peripheral receive registers.

```c

#define I2C_RXDATA_REG 0x40040008

desc->src_addr  = I2C_RXDATA_REG;  // e.g., I2C RXDATA register
desc->dst_addr  = (uint32_t)sram_buffer;
desc->len_flags = make_len_flags(nbytes, 2, 1, 0, 0);  // src_fixed=1

```

## Descriptor chaining

Descriptors link via `next_desc`. DMA sequentially fetches and executes each. Set `next_desc = 0` on the final descriptor.

```c

desc0->next_desc = (uint32_t)desc1;
desc1->next_desc = 0;  // end of chain

REG32(DMA_DESC_ADDR) = (uint32_t)desc0;
REG32(DMA_CTRL) = (DMA_CTRL_ENABLE | DMA_CTRL_START);

```

STATUS.done sets after entire chain completes.

## Flow control with polling

For peripheral transfers, DMA polls status registers before data beats to prevent FIFO overruns. Set `poll_en=1` and configure:

```c

#define SPI_STATUS_REG 0x40020000
#define SPI_STATUS_TX_FULL_MASK 0x00000004

// Wait until SPI TX FIFO is not full before each write
desc->len_flags = make_len_flags(nbytes, 0, 0, 1, 1);  // poll_en=1, dst_fixed=1
desc->poll_addr  = SPI_STATUS_REG;  // SPI STATUS register
desc->poll_mask  = SPI_STATUS_TX_FULL_MASK;  // bit 2 = TX Full
desc->poll_value = 0x00000000;  // proceed when TX not full

```

DMA reads `poll_addr` and retries until `(read_data & poll_mask) == poll_value`. This provides hardware flow control.

## Aborting a transfer

Write CTRL bit 2 to abort:

```c

REG32(DMA_CTRL) = DMA_CTRL_ABORT;

```

Post-abort STATUS: `done=1, error=1, error_code=5`.

## Constraints

- Descriptors must be 32-byte aligned

- Maximum transfer per descriptor: 16 MB (24-bit length field)

- Beat width must not exceed the bus width (128 bits / 16 bytes)

- The DMA issues one outstanding transaction at a time (no burst pipelining)

- No interrupt support; use polling on STATUS.done

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/bus/DmaEngine.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.


> **Traceability:** Generated by Gemini. Derived from upstream commit d9622642c63f7eba6e0c9baa7fea2188d32e28e3.