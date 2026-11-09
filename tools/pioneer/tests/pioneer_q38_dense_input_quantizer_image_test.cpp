/* SPDX-License-Identifier: MIT */
// Standalone emitted Q6 projection-input instructions on pinned CPU DATA.
#define main pioneer_unused_projection_input_image_main
#include "pioneer_q38_projection_input_image_test.cpp"
#undef main
int main(int argc,char** argv){
 assert(argc==3);FILE* f=fopen(argv[2],"rb");assert(f);
 std::vector<float> input(6144);assert(fread(input.data(),sizeof(float),input.size(),f)==input.size());
 assert(fgetc(f)==EOF && !ferror(f) && !fclose(f));
 unsigned maxSteps=0,minStack=0x70000;
 for(unsigned at=0;at<input.size();at+=256){
  ProjectionInputMachine machine(argv[1],14,input.data()+at);
  assert(machine.Run() && machine.writes==144);
  ProjectionInput actual,expected;machine.Collect(actual);
  ProjectionInputReference(14,input.data()+at,expected);
  assert(!memcmp(&actual,&expected,sizeof(actual)));
  maxSteps=std::max(maxSteps,machine.steps);minStack=std::min(minStack,machine.minStack);
 }
 printf("PASS standalone emitted RV32 Q6 input quantizer 24 chunks/6144 CPU-derived values max_steps%u min_stack%08x; SIMULATED, NOT CONNECTED ROOT OR CARD\n",maxSteps,minStack);
 return 0;
}
