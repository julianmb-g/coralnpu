// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package coralnpu

import chisel3._
import chisel3.simulator.scalatest.ChiselSim
import org.scalatest.freespec.AnyFreeSpec

class DecodeInstructionWrapper(p: Parameters) extends Module {
  val io = IO(new Bundle {
    val inst = Input(UInt(32.W))
    val out = Output(new DecodedInstruction(p))
  })
  io.out := DecodeInstruction(p, 0, 0.U, io.inst, 0.U)
}

class DecodeSpec extends AnyFreeSpec with ChiselSim {
  val p = new Parameters

  "Branch Instruction Bit Patterns" in {
    simulate(new DecodeInstructionWrapper(p)) { dut =>
      // BEQ
      // 0000000_00000_00000_000_00000_1100011 -> 0x00000063
      dut.io.inst.poke("h00000063".U)
      dut.clock.step()
      dut.io.out.beq.expect(true.B)
      dut.io.out.bne.expect(false.B)

      // BNE
      // 0000000_00000_00000_001_00000_1100011 -> 0x00001063
      dut.io.inst.poke("h00001063".U)
      dut.clock.step()
      dut.io.out.bne.expect(true.B)
      dut.io.out.beq.expect(false.B)

      // BLT
      // 0000000_00000_00000_100_00000_1100011 -> 0x00004063
      dut.io.inst.poke("h00004063".U)
      dut.clock.step()
      dut.io.out.blt.expect(true.B)

      // BGE
      // 0000000_00000_00000_101_00000_1100011 -> 0x00005063
      dut.io.inst.poke("h00005063".U)
      dut.clock.step()
      dut.io.out.bge.expect(true.B)

      // BLTU
      // 0000000_00000_00000_110_00000_1100011 -> 0x00006063
      dut.io.inst.poke("h00006063".U)
      dut.clock.step()
      dut.io.out.bltu.expect(true.B)

      // BGEU
      // 0000000_00000_00000_111_00000_1100011 -> 0x00007063
      dut.io.inst.poke("h00007063".U)
      dut.clock.step()
      dut.io.out.bgeu.expect(true.B)
    }
  }
}
