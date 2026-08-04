# DMA engine

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

The single-channel, linked-list descriptor DMA engine offloads bulk data movement from the CPU. It connects to the TileLink-UL crossbar as a host (master, read/write transactions) and a device (slave, CPU programming via CSRs). It prevents CPU stalls during memory transfers (e.g., model weights, peripheral streaming).

## Architecture

TileLink-UL ports:

- **Host port** (128-bit): Issues Get/PutFullData crossbar transactions for memory accesses.
- **Device port** (32-bit): Accepts CSR read/write transactions from the CPU.

The engine processes an in-memory linked list of descriptors defining transfers (source, destination, length, beat size, flow control). Descriptors chain via `next_desc`; 0 signals end-of-chain.

### Transfer modes

| Mode | Source Addr | Dest Addr | Use Case |
|------|-----------|----------|----------|
| Mem→Mem | Incrementing | Incrementing | SRAM↔DDR, SRAM→ITCM/DTCM |
| Mem→Periph | Incrementing | Fixed | SRAM→SPI TX FIFO |
| Periph→Mem | Fixed | Incrementing | I2C RX→SRAM |

### Key parameters

- **Host port width**: 128-bit
- **Device port width**: 32-bit
- **Max transfer per descriptor**: 16 MB (24-bit length)
- **Outstanding transactions**: 1
- **Interrupt**: None (CPU polls STATUS)

## Register map

Base address: `0x40050000` (4 KB region)

| Offset | Name | Access | Bits | Description |
|--------|------|--------|------|-------------|
| `0x00` | CTRL | RW | [0] enable, [1] start (W1S, self-clearing), [2] abort | Control |
| `0x04` | STATUS | RO | [0] busy, [1] done, [2] error, [7:4] error_code | Status |
| `0x08` | DESC_ADDR | RW | [31:0] | First descriptor address |
| `0x0C` | CUR_DESC | RO | [31:0] | Current descriptor address |
| `0x10` | XFER_REMAIN | RO | [23:0] | Bytes remaining |

### Programming sequence

```text
1. Build memory descriptor chain
2. Write DESC_ADDR
3. Write CTRL (enable=1, start=1)
4. Poll STATUS.done
5. Check STATUS.error
```

### Descriptor format

32-byte aligned 32-byte descriptors (two 128-bit TL-UL beats) fetched via host port.

```text
Offset  Field         Bits     Description
0x00    src_addr      [31:0]   Source address
0x04    dst_addr      [31:0]   Destination address
0x08    xfer_len      [23:0]   Transfer length in bytes
        xfer_width    [26:24]  Beat size: log2(bytes). 0=1B, 1=2B, 2=4B, 3=8B, 4=16B
        flags         [31:27]  [27] src_fixed, [28] dst_fixed, [29] poll_en, [30:31] reserved
0x0C    next_desc     [31:0]   Next descriptor address (0 = end)
0x10    poll_addr     [31:0]   Status register poll address (0 = none)
0x14    poll_mask     [31:0]   Poll bitmask
0x18    poll_value    [31:0]   Expected masked value
0x1C    reserved      [31:0]
```

### Peripheral flow control

DMA uses descriptor-level status polling to pace transfers, avoiding peripheral modifications.
If configured (`poll_en`, `poll_addr != 0`), DMA reads `poll_addr` before each beat, waiting until `(read_data & poll_mask) == poll_value`.

### Example: DMA → SPI TX

```text
Descriptor:
  src_addr   = 0x20000000  (SRAM buffer)
  dst_addr   = 0x40020008  (SPI TXDATA)
  dst_fixed  = 1
  poll_addr  = 0x40020000  (SPI STATUS)
  poll_mask  = 0x00000004  (bit 2 = TX Full)
  poll_value = 0x00000000  (wait until TX not full)
```

### Example: I2C RX → DMA

```text
Descriptor:
  src_addr   = 0x40040008  (I2C RXDATA)
  dst_addr   = 0x20001000  (SRAM buffer)
  src_fixed  = 1
  poll_addr  = 0x40040000  (I2C STATUS)
  poll_mask  = 0x00000002  (bit 1 = RX available)
  poll_value = 0x00000002  (wait for RX data)
```

## State machine

```text
IDLE ──[start]──► FETCH_DESC_0 ──[d.fire]──► FETCH_DESC_1 ──[d.fire]──► POLL_CHECK
  ▲                                                                          │
  │                                                          [no poll or match]
  │                                                                          ▼
  │                                                                   XFER_READ_REQ
  │                                                                          │
  │                  [poll_en &&                                         [a.fire]
  │                   mismatch]                                              ▼
  │                       │                                          XFER_READ_RESP
  │                  POLL_REQ ◄── POLL_RESP                                  │
  │                       │          ▲  │                                [d.fire]
  │                  [a.fire]        │  │                                     ▼
  │                       ▼          │  [match]                       XFER_WRITE_REQ
  │                  POLL_RESP ──────┘     │                                 │
  │                                        ▼                            [a.fire]
  │                                  XFER_READ_REQ                           ▼
  │                                                                   XFER_WRITE_RESP
  │                                                                          │
  │                                                               [d.fire, remaining>0]
  │                                                                     ──► POLL_CHECK
  │                                                               [d.fire, remaining==0,
  │                                                                next!=0]
  │                                                                     ──► FETCH_DESC_0
  │                                                               [d.fire, remaining==0,
  │                                                                next==0]
  │                                                                          │
  └────────────────────────── DONE ◄─────────────────────────────────────────┘
```

### State descriptions

- **IDLE**: Waits for `CTRL.start`. Latches `DESC_ADDR`.
- **FETCH_DESC_0**: TL-UL Get (128-bit) bytes 0–15.
- **FETCH_DESC_1**: TL-UL Get (128-bit) bytes 16–31.
- **POLL_CHECK**: If `poll_en` and `poll_addr != 0`, goto POLL_REQ. Else XFER_READ_REQ.
- **POLL_REQ**: TL-UL Get (32-bit) at `poll_addr`.
- **POLL_RESP**: Captures D channel. If `(data & poll_mask) == poll_value`, goto XFER_READ_REQ. Else POLL_REQ.
- **XFER_READ_REQ**: TL-UL Get at source address.
- **XFER_READ_RESP**: Captures D channel data.
- **XFER_WRITE_REQ**: TL-UL PutFullData to destination.
- **XFER_WRITE_RESP**: On D ack, updates addresses and remaining length. If remaining > 0, loop to POLL_CHECK. If remaining == 0 and `next_desc != 0`, goto FETCH_DESC_0. Else DONE.
- **DONE**: Sets `STATUS.done`, returns to IDLE.

Abort transitions to IDLE with error flag. TL-UL D channel error transitions to DONE with error code.

## TileLink host interface

Single 128-bit TL-UL master port generates:

- **Get**: opcode=4, size=`xfer_width`, address=`src_addr`, mask=all-ones
- **PutFullData**: opcode=0, size=`xfer_width`, address=`dst_addr`, data=buffer
- **Poll Get**: opcode=4, size=2, address=`poll_addr`
- **Descriptor Get**: opcode=4, size=4, address=`desc_addr` / `desc_addr+16`

Source ID is 0. Host A channel shared among descriptor fetch, poll reads, data accesses.

## TileLink device interface

Follows GPIO pattern (`hdl/chisel/src/bus/GPIO.scala`):

- `tl_a.ready := !tl_d_valid`
- `tl_a.fire`: decode `address[11:0]`, access CSRs. Start bit triggers FSM.

### Crossbar connection

### Address map

DMA occupies `0x40050000–0x40050FFF`, after I2C at `0x40040000`.

### Host connectivity

Host port connects to memory and peripherals:

```scala
"dma" -> Seq("sram", "coralnpu_device", "rom", "ddr_ctrl", "ddr_mem",
             "spi_master", "gpio", "i2c_master", "uart0", "uart1")
```

CPU programming path:

```scala
"coralnpu_core" -> Seq(...existing..., "dma")
```

## Implementation

File: `hdl/chisel/src/bus/DmaEngine.scala`

Configuration changes:

- `hdl/chisel/src/soc/CrossbarConfig.scala` (host/device ranges, connections)
- `hdl/chisel/src/soc/SoCChiselConfig.scala` (`DmaParameters`)
- `hdl/chisel/src/soc/CoralNPUChiselSubsystem.scala` (instantiation)

### Module IO

```scala
class DmaEngine(hostParams: Parameters, deviceParams: Parameters) extends Module {
  val hostTlulP = new TLULParameters(hostParams)
  val deviceTlulP = new TLULParameters(deviceParams)
  val io = IO(new Bundle {
    val tl_host   = new OpenTitanTileLink.Host2Device(hostTlulP)
    val tl_device = Flipped(new OpenTitanTileLink.Host2Device(deviceTlulP))
  })
}
```

### Internal structure

- **CSR register file**: CTRL, STATUS, DESC_ADDR.
- **Descriptor latch**: Registers loaded during FETCH.
- **Data buffer**: 128-bit register.
- **Address counters**: src/dst, remaining length.
- **FSM**: ChiselEnum states.
- **Integrity**: `RequestIntegrityGen` (host A), `ResponseIntegrityGen` (device D).

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-04 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/bus/DmaEngine.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.


> **Traceability:** Generated by Gemini. Derived from upstream commit d9622642c63f7eba6e0c9baa7fea2188d32e28e3.