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

package bus

import chisel3._
import chisel3.util._
import chisel3.simulator.scalatest.ChiselSim
import coralnpu.Parameters
import org.scalatest.freespec.AnyFreeSpec

class TlulSocket1NTest extends AnyFreeSpec with ChiselSim with TLULTestUtils {
  val p = new Parameters
  val tlul_p = new TLULParameters(p)

  "TlulSocket1N should preserve user integrity bits" in {
    simulate(new Module {
      val io = IO(new Bundle {
        val tl_h = Flipped(new OpenTitanTileLink.Host2Device(tlul_p))
        val tl_d = Vec(4, new OpenTitanTileLink.Host2Device(tlul_p))
        val dev_select_i = Input(UInt(log2Ceil(4).W))
      })
      val dut = Module(new TlulSocket1N(tlul_p, N=4))
      io.tl_h <> dut.io.tl_h
      dut.io.tl_d <> io.tl_d
      dut.io.dev_select_i := io.dev_select_i
    }) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)

      // Drive request to device 0 with integrity bits
      dut.io.dev_select_i.poke(0.U)
      dut.io.tl_h.a.valid.poke(true.B)
      dut.io.tl_h.a.bits.address.poke(0x1000.U)
      dut.io.tl_h.a.bits.user.data_intg.poke(1.U)
      dut.io.tl_h.a.bits.user.cmd_intg.poke(1.U)

      dut.clock.step()
      
      assert(dut.io.tl_d(0).a.bits.user.data_intg.peek().litValue == 1)
      assert(dut.io.tl_d(0).a.bits.user.cmd_intg.peek().litValue == 1)
    }
  }
}
