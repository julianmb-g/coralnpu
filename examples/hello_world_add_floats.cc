// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>

float input1[8] __attribute__((section(".data")));
float input2[8] __attribute__((section(".data")));
float output[8] __attribute__((section(".data")));

int main() {
  input1[0] = 1.0f;
  input2[0] = 2.0f;

  // Explicitly write to f0 (boundary FPR, wb_idx 32) to catch Mutant 3.
  asm volatile("flw f0, %0" : : "m"(input1[0]));
  asm volatile("flw f1, %0" : : "m"(input2[0]));

  for (int i = 0; i < 8; i++) {
    output[i] = input1[i] + input2[i];
  }
  return 0;
}
