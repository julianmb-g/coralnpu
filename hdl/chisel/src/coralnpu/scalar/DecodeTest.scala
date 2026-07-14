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
import org.scalatest.matchers.should.Matchers

class DecodeSpec extends AnyFreeSpec with ChiselSim with Matchers {
  "DecodeInstruction" - {
    "should correctly decode branch instructions and maintain one-hot property" in {
      simulate(new Module {
        val io = IO(new Bundle {
          val inst = Input(UInt(32.W))
          val decoded = Output(new DecodedInstruction(Parameters()))
          val is_cond_br = Output(Bool())
        })
        val decoded = DecodeInstruction(Parameters(), 0, 0.U, io.inst, 0.U)
        io.decoded := decoded
        io.is_cond_br := decoded.isCondBr()
      }) { dut =>
        val rng = new scala.util.Random(42)

        def makeBranch(funct3: Int, rs1: Int, rs2: Int, imm1: Int, imm2: Int): UInt = {
          val inst = (BigInt(imm1) << 25) | (BigInt(rs2) << 20) | (BigInt(rs1) << 15) | (BigInt(funct3) << 12) | (BigInt(imm2) << 7) | 0x63
          inst.U(32.W)
        }

        for (i <- 0 until 10) {
          val rs1 = rng.nextInt(32)
          val rs2 = rng.nextInt(32)
          val imm1 = rng.nextInt(128)
          val imm2 = rng.nextInt(32)

          val branchTests = Seq(
            ("beq",  makeBranch(0, rs1, rs2, imm1, imm2)),
            ("bne",  makeBranch(1, rs1, rs2, imm1, imm2)),
            ("blt",  makeBranch(4, rs1, rs2, imm1, imm2)),
            ("bge",  makeBranch(5, rs1, rs2, imm1, imm2)),
            ("bltu", makeBranch(6, rs1, rs2, imm1, imm2)),
            ("bgeu", makeBranch(7, rs1, rs2, imm1, imm2))
          )

          for ((expected, inst) <- branchTests) {
            dut.io.inst.poke(inst)
            
            val decoded = dut.io.decoded
            
            // Verify the expected bit is high
            expected match {
              case "beq"  => decoded.beq.expect(true.B)
              case "bne"  => decoded.bne.expect(true.B)
              case "blt"  => decoded.blt.expect(true.B)
              case "bge"  => decoded.bge.expect(true.B)
              case "bltu" => decoded.bltu.expect(true.B)
              case "bgeu" => decoded.bgeu.expect(true.B)
            }

            // One-hot verification: ensure NO OTHER branch bit is high
            if (expected != "beq") decoded.beq.expect(false.B)
            if (expected != "bne") decoded.bne.expect(false.B)
            if (expected != "blt") decoded.blt.expect(false.B)
            if (expected != "bge") decoded.bge.expect(false.B)
            if (expected != "bltu") decoded.bltu.expect(false.B)
            if (expected != "bgeu") decoded.bgeu.expect(false.B)
            
            // Also ensure it's identified as a branch instruction
            dut.io.is_cond_br.expect(true.B)
          }
        }
        
        // Negative test cases (non-branch instructions)
        val negativeTests = Seq(
          "b0000000_00001_00010_000_00000_0110011".U(32.W), // add
          "b0000000_00001_00010_010_00000_0000011".U(32.W), // lw
          "b0000000_00000_00000_000_00000_0110111".U(32.W)  // lui
        )
        for (inst <- negativeTests) {
          dut.io.inst.poke(inst)
          val decoded = dut.io.decoded
          decoded.beq.expect(false.B)
          decoded.bne.expect(false.B)
          decoded.blt.expect(false.B)
          decoded.bge.expect(false.B)
          decoded.bltu.expect(false.B)
          decoded.bgeu.expect(false.B)
          dut.io.is_cond_br.expect(false.B)
        }
      }
    }
  }
}
