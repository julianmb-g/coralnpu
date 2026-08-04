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

package common

import chisel3._
import chisel3.simulator.scalatest.ChiselSim
import chisel3.util._
import org.scalatest.freespec.AnyFreeSpec

object TestEnum extends ChiselEnum {
  val A = Value(0.U)
  val B = Value(2.U) // 2 bits wide, value 1 is invalid
}

class SafeMuxUpTo1HWrapper extends Module {
  val io = IO(new Bundle {
    val defaultVal = Input(Valid(TestEnum()))
    val sel = Input(Vec(2, Bool()))
    val data = Input(Vec(2, Valid(TestEnum())))
    val out_valid = Output(Bool())
    val out_bits = Output(UInt(2.W))
  })

  val res = SafeMuxUpTo1H(io.defaultVal, io.sel, io.data, TestEnum)
  io.out_valid := res.valid
  io.out_bits := res.bits.asUInt
}

class SafeMuxUpTo1HSpec extends AnyFreeSpec with ChiselSim {
  "SafeMuxUpTo1H" - {
    "select the correct input when selector is active" in {
      simulate(new SafeMuxUpTo1HWrapper) { dut =>
        dut.io.defaultVal.valid.poke(0)
        dut.io.defaultVal.bits.poke(TestEnum.A)
        dut.io.sel(0).poke(1)
        dut.io.sel(1).poke(0)
        dut.io.data(0).valid.poke(1)
        dut.io.data(0).bits.poke(TestEnum.B)
        dut.io.data(1).valid.poke(0)
        dut.io.data(1).bits.poke(TestEnum.A)

      dut.clock.step()
      
      // If the mutant is used (enumObj(bitsUInt)), Chisel will throw an exception 
      // or assert during simulation here because 3 is out of bounds for TestEnum.
      // If safe extraction is used (enumObj.safe(bitsUInt)._1), it will safely 
      // return the first element (0) without crashing and mask the out-of-bounds value.
      // If the mutant (unsafe cast) is used, it will pass through the invalid value (3).
      dut.io.out_valid.expect(true.B)
      dut.io.out_bits.expect(2.U)
      }
    }

    "zero output bits when selector is out of bounds" in {
      simulate(new SafeMuxUpTo1HWrapper) { dut =>
        dut.io.defaultVal.valid.poke(0)
        dut.io.defaultVal.bits.poke(TestEnum.A)

        dut.io.sel(0).poke(1)
        dut.io.sel(1).poke(0)

        dut.io.data(0).valid.poke(1)
        // Poke value 1 which is invalid for TestEnum(0, 2)
        dut.io.data(0).bits.poke(1)

        dut.io.data(1).valid.poke(0)
        dut.io.data(1).bits.poke(TestEnum.A)

      dut.clock.step()
      
      // If the mutant is used (enumObj(bitsUInt)), Chisel will throw an exception 
      // or assert during simulation here because 3 is out of bounds for TestEnum.
      // If safe extraction is used (enumObj.safe(bitsUInt)._1), it will safely 
      // return the first element (0) without crashing and mask the out-of-bounds value.
      // If the mutant (unsafe cast) is used, it will pass through the invalid value (3).
      dut.io.out_valid.expect(false.B)
      dut.io.out_bits.expect(0.U)
    }
  }
}
