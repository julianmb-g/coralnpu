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

class CsrDecodeTester(p: Parameters) extends Module {
  val io = IO(new Bundle {
    val inst = Input(UInt(32.W))
    val isCsr = Output(Bool())
  })
  val decoded = DecodeInstruction(p, 0, 0.U, io.inst, 0.U)
  io.isCsr := decoded.isCsr()
}

class CsrReadBoundsTest extends AnyFreeSpec with ChiselSim {
  val p = new Parameters

  "is_varying_csr_read logic bounds for SYSTEM opcode" in {
    simulate(new CsrDecodeTester(p)) { dut =>
      // Test the varying CSR reads specifically identified by the SV checker:
      // mcycle (0xB00), mcycleh (0xB80)
      // cycle (0xC00), cycleh (0xC80)
      // time (0xC01), timeh (0xC81)
      
      val varyingCsrs = Seq(0xB00, 0xB80, 0xC00, 0xC80, 0xC01, 0xC81)
      
      for (csr <- varyingCsrs) {
        val baseInst: Long = (csr.toLong << 20) | (0L << 15) | (2L << 12) | (1L << 7)
        val validSystemInst: Long = baseInst | 0x73L
        
        dut.io.inst.poke(validSystemInst.U(32.W))
        dut.clock.step()
        assert(dut.io.isCsr.peek().litToBoolean == true, s"Valid SYSTEM CSR read failed for CSR 0x${csr.toHexString}")
        
        val invalidNonSystemInst: Long = baseInst | 0x33L
        dut.io.inst.poke(invalidNonSystemInst.U(32.W))
        dut.clock.step()
        assert(dut.io.isCsr.peek().litToBoolean == false, s"Mutated non-SYSTEM instruction incorrectly decoded as CSR read for CSR 0x${csr.toHexString}")
      }
    }
  }
}
