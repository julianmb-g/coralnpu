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
import chisel3.util._
import chisel3.simulator.scalatest.ChiselSim
import org.scalatest.freespec.AnyFreeSpec
import coralnpu.rvv._

class DispatchV2Wrapper(p: Parameters) extends Module {
  val io = IO(new Bundle {
    val inst = Vec(p.instructionLanes, Flipped(Decoupled(new FetchInstruction(p))))
    val dvu_op = Output(UInt(4.W))
  })

  val dut = Module(new DispatchV2(p))
  
  dut.io.halted := false.B
  dut.io.mactive := false.B
  dut.io.lsuActive := false.B
  dut.io.scoreboard.regd := 0.U
  dut.io.scoreboard.comb := 0.U
  if (p.enableFloat) dut.io.fscoreboard.get := 0.U
  if (p.enableFloat && dut.io.csrFrm.nonEmpty) dut.io.csrFrm.get := 0.U
  dut.io.branchTaken := false.B
  dut.io.retirement_buffer_nSpace := 4.U
  dut.io.retirement_buffer_empty := true.B
  dut.io.retirement_buffer_trap_pending := false.B
  dut.io.single_step := false.B
  dut.io.debug_mode := false.B
  dut.io.interlock := false.B
  dut.io.lsuQueueCapacity := DontCare
  dut.io.jalrTarget.foreach { t =>
    t := 0.U.asTypeOf(t)
  }
  if (p.enableRvv) {
    dut.io.rvvIdle.get := true.B
    dut.io.rvvQueueCapacity.get := 4.U
    dut.io.rvvState.get.valid := false.B
    dut.io.rvvState.get.bits := 0.U.asTypeOf(dut.io.rvvState.get.bits)
  }

  for (i <- 0 until p.instructionLanes) {
    dut.io.inst(i) <> io.inst(i)
    dut.io.rs1Read(i) := DontCare
    dut.io.rs1Set(i) := DontCare
    dut.io.rs2Read(i) := DontCare
    dut.io.rs2Set(i) := DontCare
    dut.io.rdMark(i) := DontCare
    dut.io.busRead(i) := DontCare
    dut.io.lsu(i).ready := DontCare
    dut.io.mlu(i).ready := DontCare
    dut.io.dvu(i).ready := DontCare
  }
  
  if (p.enableFloat) {
    dut.io.rdMark_flt.get := DontCare
    dut.io.float.get.ready := DontCare
    for (i <- 0 until p.instructionLanes) {
      dut.io.frs1Read.get(i) := DontCare
    }
  }

  if (p.enableRvv) {
    for (i <- 0 until p.instructionLanes) {
      dut.io.rvvRdMark.get(i) := DontCare
      dut.io.rvv.get(i).ready := DontCare
    }
  }

  io.dvu_op := dut.io.dvu(0).bits.op.asUInt
}

class DispatchDvuFallbackSpec extends AnyFreeSpec with ChiselSim {
  val p = new Parameters

  "Invalid instruction falls back to DvuOp.DIV" in {
    simulate(new DispatchV2Wrapper(p)) { dut =>
      for (i <- 0 until p.instructionLanes) {
        dut.io.inst(i).valid.poke(false.B)
        dut.io.inst(i).bits.inst.poke(0.U)
        dut.io.inst(i).bits.addr.poke(0.U)
        dut.io.inst(i).bits.brchFwd.poke(false.B)
      }

      // Provide an invalid instruction on lane 0
      dut.io.inst(0).valid.poke(true.B)
      dut.io.inst(0).bits.inst.poke(0.U) // All zeros is an invalid opcode

      assert(dut.io.dvu_op.peek().litValue == 0, "DVU fallback opcode did not match DvuOp.DIV")
    }
  }
}
