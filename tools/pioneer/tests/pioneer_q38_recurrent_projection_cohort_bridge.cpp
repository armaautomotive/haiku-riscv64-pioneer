/* SPDX-License-Identifier: MIT */
// Actual emitted cohort instructions with previously computed projection
// outputs. This is a new seeded cohort machine, not a continuous worker run.
#define PIONEER_Q38_MIXED_RECURRENT_COHORT_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_cohort_test.cpp"

int main(int argc,char** argv){
 assert(argc==3);
 std::array<uint32_t,6144> gate;
 std::array<uint32_t,10240> qkv;
 assert(fread(gate.data(),4,gate.size(),stdin)==gate.size());
 assert(fread(qkv.data(),4,qkv.size(),stdin)==qkv.size());
 assert(fgetc(stdin)==EOF);
 for(uint32_t value:gate)assert(isfinite(Float(value)));
 for(uint32_t value:qkv)assert(isfinite(Float(value)));
 auto m=std::make_unique<NewCohortMachine>(argv[1],argv[2]);
 assert(m->Wait(0));
 const unsigned base=m->header[8],route=m->header[9];
 for(unsigned i=0;i<gate.size();i++){
  m->Put(route,base+PioneerQ38ProjectionResultOffset(5)+i*4,gate[i]);
 }
 for(unsigned i=0;i<qkv.size();i++){
  m->Put(route,base+PioneerQ38ProjectionResultOffset(6)+i*4,qkv[i]);
 }
 m->Stage();assert(m->Wait(1));m->Verify();
 // Keep the actual computed result available for the output-projection
 // bridge; only the projection data crossed this simulated boundary.
 for(unsigned i=0;i<6144;i++){
  uint32_t value=m->Get(route,base+TT_Q38_GDN_GATED_OUTPUT+i*4);
  assert(isfinite(Float(value)) && fwrite(&value,4,1,stdout)==1);
 }
 assert(!fflush(stdout));
}
