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

class DispatchAluSpec extends AnyFreeSpec with ChiselSim {
  val p = new Parameters

  "Invalid instructions should not produce valid ALU ops" in {
    simulate(new DispatchV2(p)) { dut =>
      dut.io.halted.poke(false.B)
      dut.io.mactive.poke(false.B)
      dut.io.lsuActive.poke(false.B)
      dut.io.branchTaken.poke(false.B)
      dut.io.interlock.poke(false.B)
      dut.io.retirement_buffer_empty.poke(true.B)
      dut.io.retirement_buffer_nSpace.poke(4.U)
      dut.io.retirement_buffer_trap_pending.poke(false.B)
      dut.io.single_step.poke(false.B)
      dut.io.lsuQueueCapacity.poke(4.U)
      dut.io.scoreboard.regd.poke(0.U)
      dut.io.scoreboard.comb.poke(0.U)
      if (p.enableFloat) dut.io.fscoreboard.get.poke(0.U)
      if (p.enableFloat && dut.io.csrFrm.nonEmpty) dut.io.csrFrm.get.poke(0.U)

      for (i <- 0 until p.instructionLanes) {
        dut.io.inst(i).valid.poke(false.B)
        dut.io.inst(i).bits.inst.poke(0.U)
        dut.io.inst(i).bits.addr.poke(0.U)
        dut.io.inst(i).bits.brchFwd.poke(false.B)
      }

      // Test with a non-ALU instruction (JAL) in slot 0
      dut.io.inst(0).valid.poke(true.B)
      dut.io.inst(0).bits.inst.poke(0x0000006F.U)

      dut.clock.step()
      dut.io.alu(0).valid.expect(false.B)
    }
  }
}
