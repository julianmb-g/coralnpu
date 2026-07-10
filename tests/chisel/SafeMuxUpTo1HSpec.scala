package common

import chisel3._
import chisel3.simulator.scalatest.ChiselSim
import chisel3.util._
import org.scalatest.freespec.AnyFreeSpec

object TestEnum extends ChiselEnum {
  val A, B, C = Value
}

class SafeMuxUpTo1HTester extends Module {
  val io = IO(new Bundle {
    val sel = Input(Bool())
    val invalid_bits = Input(UInt(2.W)) // Will be passed as 3 (out of bounds)
    val out_valid = Output(Bool())
    val out_bits = Output(UInt(2.W))
  })

  val defaultVal = Wire(Valid(TestEnum()))
  defaultVal.valid := false.B
  defaultVal.bits := TestEnum.A

  val in0 = Wire(Valid(TestEnum()))
  in0.valid := true.B
  // Force an out-of-bounds value into the enum using asTypeOf
  in0.bits := io.invalid_bits.asTypeOf(TestEnum())

  // SafeMuxUpTo1H will try to extract the enum from the Mux result
  val out = SafeMuxUpTo1H(defaultVal, Seq(io.sel), Seq(in0), TestEnum)
  
  io.out_valid := out.valid
  io.out_bits := out.bits.asUInt
}

class SafeMuxUpTo1HSpec extends AnyFreeSpec with ChiselSim {
  "SafeMuxUpTo1H should handle out-of-bounds extraction without throwing simulation assertion" in {
    simulate(new SafeMuxUpTo1HTester) { dut =>
      // Pass 3, which is out of bounds for TestEnum (0, 1, 2)
      dut.io.invalid_bits.poke(3.U)
      // Select the invalid value
      dut.io.sel.poke(true.B)

      dut.clock.step()
      
      // If the mutant is used (enumObj(bitsUInt)), Chisel will throw an exception 
      // or assert during simulation here because 3 is out of bounds for TestEnum.
      // If safe extraction is used (enumObj.safe(bitsUInt)._1), it will safely 
      // return the first element (0) without crashing and mask the out-of-bounds value.
      // If the mutant (unsafe cast) is used, it will pass through the invalid value (3).
      dut.io.out_valid.expect(true.B)
      dut.io.out_bits.expect(0.U)
    }
  }
}
