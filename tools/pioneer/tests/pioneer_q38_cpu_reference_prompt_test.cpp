/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_cpu_reference_prompt.h"
#include <cassert>
#include <cstdio>
int main(){
 assert(PioneerQ38CpuReferenceInput(0,0,false)==248319);
 assert(PioneerQ38CpuReferenceInput(1,70253,false)==17);
 assert(PioneerQ38CpuReferenceInput(2,17,false)==17);
 assert(PioneerQ38CpuReferenceInput(0,0,true)==248319);
 assert(PioneerQ38CpuReferenceInput(1,70253,true)==70253);
 assert(PioneerQ38CpuReferenceInput(2,42,true)==42);
 std::puts("PASS distinct forced-prompt and autoregressive CPU-only input policies; NO MODEL OR DEVICE");
}
