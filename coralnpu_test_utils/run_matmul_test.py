#!/usr/bin/env python3
# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import sys
import numpy as np
from elftools.elf.elffile import ELFFile
from coralnpu_hw.coralnpu_test_utils.ftdi_spi_master import FtdiSpiMaster

        if not all([self.addr_lhs, self.addr_rhs, self.addr_result,
                    self.addr_lhs_rows, self.addr_rhs_cols, self.addr_inner]
                   ) or self.entry_point is None:
            raise ValueError(
                "Could not find all required symbols in ELF file."
            )

    def _generate_data(self):
        """Generates input matrices and a golden output matrix."""
        # Dimensions from tests/cocotb/rvv/ml_ops/rvv_matmul.cc
        k_lhs_rows = 16
        k_rhs_cols = 16
        k_inner = 48

        print("Generating test data...")
        # Using int8 for the input matrices
        self.lhs_input = np.random.randint(
            -128, 127, size=(self.lhs_rows, self.inner), dtype=np.int8
        )
        self.rhs_input = np.random.randint(
            -128, 127, size=(self.inner, self.rhs_cols), dtype=np.int8
        )

        # The C++ code performs the matmul as int8*int8 -> int32
        # We need to cast the inputs to a wider type before multiplication to avoid overflow
        golden_lhs = self.lhs_input.astype(np.int32)
        golden_rhs = self.rhs_input.astype(np.int32)
        self.golden_output = np.matmul(golden_lhs, golden_rhs)
        print("Test data generated.")

    def run_test(self):
        """Executes the full matrix multiplication test flow."""
        is_responsive = False
        if not self.reset:
            try:
                print("Checking if FPGA bus is responsive...")
                self.spi_master.read_word(0x0)
                is_responsive = True
                print("FPGA bus is responsive.")
            except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
                print(
                    "FPGA bus is unresponsive. Attempting automatic recovery reset..."
                )

        if self.reset or not is_responsive:
            print("Performing hardware reset (toggle PROG_B)...")
            self.spi_master.device_reset()
            time.sleep(0.1)

        self.spi_master.idle_clocking(20)
        self._generate_data()

        # 1. Load ELF (without starting the core)
        self.spi_master.load_elf(
            self.elf_path, start_core=False, verify=self.verify
        )

        # 1b. Load dimensions
        print(
            f"Loading dimensions: {self.lhs_rows}x{self.inner}x{self.rhs_cols}"
        )
        self.spi_master.write_word(self.addr_lhs_rows, self.lhs_rows)
        self.spi_master.write_word(self.addr_rhs_cols, self.rhs_cols)
        self.spi_master.write_word(self.addr_inner, self.inner)

        # 2. Load input matrices into memory
        print(
            f"Loading LHS matrix ({self.lhs_input.nbytes} bytes) to 0x{self.addr_lhs:x}"
        )
        self.spi_master.load_data(self.lhs_input.tobytes(), self.addr_lhs)

        print(
            f"Loading RHS matrix ({self.rhs_input.nbytes} bytes) to 0x{self.addr_rhs:x}"
        )
        self.spi_master.load_data(
            self.rhs_input.flatten(order='F').tobytes(), self.addr_rhs
        )

        # 3. Start the core
        self.spi_master.set_entry_point(self.entry_point)
        self.spi_master.start_core()

        # 4. Wait for the core to halt
        if not self.spi_master.poll_for_halt(timeout=20.0):
            raise RuntimeError("TEST FAILED: Core did not halt.")

        # 5. Retrieve the output matrix
        result_size_bytes = self.golden_output.nbytes
        print(
            f"Reading result matrix ({result_size_bytes} bytes) from 0x{self.addr_result:x}"
        )
        result_data = self.spi_master.read_data(
            self.addr_result, result_size_bytes
        )

        # 6. Compare with the golden result
        result_array = np.frombuffer(
            result_data, dtype=self.golden_output.dtype
        )
        result_array = result_array.reshape(self.golden_output.shape)

        print("\nVerifying result...")
        if np.array_equal(self.golden_output, result_array):
            print("TEST PASSED!")
        else:
            print("Golden:\n", self.golden_output)
            print("Received:\n", result_array)
            raise RuntimeError(
                "TEST FAILED: Output does not match golden reference."
            )


def main():
    parser = argparse.ArgumentParser(
        description="Run Matrix Multiplication test on CoralNPU."
    )
    parser.add_argument(
        "elf_file",
        nargs="?",
        default=None,
        help=
        "Path to the rvv_matmul.elf file. If omitted, the target will be built via Bazel."
    )
    parser.add_argument(
        "--usb-serial",
        required=True,
        help="USB serial number of the FTDI device."
    )
    parser.add_argument(
        "--ftdi-port",
        type=int,
        default=1,
        help="Port number of the FTDI device."
    )
    parser.add_argument(
        "--csr-base-addr",
        type=lambda x: int(x, 0),
        default=None,
        help=
        "Base address for CSR registers (defaults to 0x200000 for highmem, 0x30000 for lowmem)."
    )
    parser.add_argument(
        "--highmem",
        action="store_true",
        help=
        "Use highmem target (rvv_matmul_highmem) and highmem CSR base (0x200000)."
    )
    parser.add_argument(
        "--reset",
        action="store_true",
        help="Force hardware reset (toggle PROG_B) before running."
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Verify ELF loading by reading back memory."
    )
    args = parser.parse_args()

    elf_file = args.elf_file
    if elf_file is None:
        # Resolve project root
        script_dir = os.path.dirname(os.path.abspath(__file__))
        under_bazel_run = "BUILD_WORKSPACE_DIRECTORY" in os.environ
        project_root = os.environ.get(
            "BUILD_WORKSPACE_DIRECTORY", os.path.dirname(script_dir)
        )

        target = "//tests/cocotb/rvv/ml_ops:rvv_matmul_highmem" if args.highmem else "//tests/cocotb/rvv/ml_ops:rvv_matmul"

        # Try to find it first (in case it was already built)
        pattern = "bazel-out/*/bin/tests/cocotb/rvv/ml_ops/rvv_matmul_highmem.elf" if args.highmem else "bazel-out/*/bin/tests/cocotb/rvv/ml_ops/rvv_matmul.elf"
        import glob
        matches = glob.glob(os.path.join(project_root, pattern))

        if matches:
            elf_file = matches[0]
            print(f"Using built ELF: {elf_file}")
        elif under_bazel_run:
            print(
                f"Error: ELF file not found in bazel-out. Please build it first:"
            )
            print(f"  bazel build {target}")
            sys.exit(1)
        else:
            print(f"No ELF file provided. Building {target}...")
            try:
                subprocess.run(["bazel", "build", target],
                               cwd=project_root,
                               check=True)
                # Search again after build
                matches = glob.glob(os.path.join(project_root, pattern))
                if matches:
                    elf_file = matches[0]
                else:
                    elf_rel_path = "bazel-bin/tests/cocotb/rvv/ml_ops/rvv_matmul_highmem.elf" if args.highmem else "bazel-bin/tests/cocotb/rvv/ml_ops/rvv_matmul.elf"
                    elf_file = os.path.join(project_root, elf_rel_path)
            except subprocess.CalledProcessError as e:
                print(f"Error: Bazel build failed: {e}")
                sys.exit(1)

    csr_base_addr = args.csr_base_addr
    if csr_base_addr is None:
        csr_base_addr = 0x200000 if args.highmem else 0x30000

    try:
        runner = MatmulRunner(args.elf_file, args.usb_serial, args.ftdi_port, args.csr_base_addr)
        runner.run_test()
    except (ValueError, RuntimeError, FileNotFoundError) as e:
        print(f"Error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
