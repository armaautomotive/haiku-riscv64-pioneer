/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_bfp4_full_cpu_backend.h"
#include <cassert>
#include <cstdio>
int main(){
 assert(!Q38Bfp4CpuOpen(nullptr,1));assert(!Q38Bfp4CpuOpen("relative.gguf",1));
 assert(!Q38Bfp4CpuOpen("/definitely-not-a-pioneer-model.gguf",1));
 assert(!Q38Bfp4CpuOpen("/definitely-not-a-pioneer-model.gguf",0));assert(!Q38Bfp4CpuComplete(nullptr));
 std::vector<float> input(5120),output(1,42);
 assert(!Q38Bfp4CpuProject(nullptr,0,2,input,output) && output==std::vector<float>{42});
 assert(!Q38Bfp4CpuProject(nullptr,64,2,input,output) && output==std::vector<float>{42});
 assert(!Q38Bfp4CpuProject(nullptr,0,1,input,output) && output==std::vector<float>{42});
 Q38Bfp4CpuClose(nullptr);
 puts("PASS BFP4 full CPU backend invalid-source and atomic null-context refusals; NO device");
}
