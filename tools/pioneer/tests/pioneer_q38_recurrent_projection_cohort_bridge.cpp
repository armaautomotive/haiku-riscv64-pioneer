/* SPDX-License-Identifier: MIT */
// Actual emitted cohort instructions with previously computed projection
// outputs. This is a new seeded cohort machine, not a continuous worker run.
#define PIONEER_Q38_MIXED_RECURRENT_COHORT_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_cohort_test.cpp"

int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!std::strcmp(argv[3],"--learned") || !std::strcmp(argv[3],"--learned-alpha"))));
 const bool learned=argc==4;
 const bool alpha=learned && !std::strcmp(argv[3],"--learned-alpha");
 std::array<uint32_t,6144> gate;
 std::array<uint32_t,10240> qkv;
 std::vector<uint32_t> parameters(learned?41184:0);
 std::vector<uint32_t> alphaBeta(alpha?96:0);
 assert(fread(gate.data(),4,gate.size(),stdin)==gate.size());
 assert(fread(qkv.data(),4,qkv.size(),stdin)==qkv.size());
 if(learned)assert(fread(parameters.data(),4,parameters.size(),stdin)==parameters.size());
 if(alpha)assert(fread(alphaBeta.data(),4,alphaBeta.size(),stdin)==alphaBeta.size());
 assert(fgetc(stdin)==EOF);
 for(uint32_t value:gate)assert(isfinite(Float(value)));
 for(uint32_t value:qkv)assert(isfinite(Float(value)));
 for(uint32_t value:parameters)assert(isfinite(Float(value)));
 for(uint32_t value:alphaBeta)assert(isfinite(Float(value)));
 auto m=std::make_unique<NewCohortMachine>(argv[1],argv[2]);
 assert(m->Wait(0));
 const unsigned base=m->header[8],route=m->header[9];
 for(unsigned i=0;i<gate.size();i++){
  m->Put(route,base+PioneerQ38ProjectionResultOffset(5)+i*4,gate[i]);
 }
 for(unsigned i=0;i<qkv.size();i++){
  m->Put(route,base+PioneerQ38ProjectionResultOffset(6)+i*4,qkv[i]);
 }
 if(learned){
  // Slot zero represents the start of a conversation, not synthetic prior
  // state. The learned matrices retain their authentic source bytes.
  for(unsigned w=0;w<120;w++)for(unsigned job=0;job<PioneerQ38ConvBatchCount(w);job++){
   const auto* d=m->plan->Conv()+w*384+job*64;
   for(unsigned c=0;c<16;c++){
    for(unsigned t=0;t<4;t++)m->Put(d[13],d[12]+(c*4+t)*4,parameters[(d[7]+c)*4+t]);
    for(unsigned t=0;t<3;t++)m->Put(d[41],d[15]+(c*3+t)*4,0);
   }
  }
  for(unsigned w=0;w<96;w++){
   const auto* in=m->plan->Gdn()+w*192;const auto* st=in+64;const auto* ga=in+128;
   const unsigned head=w/2,first=head-in[47];assert(first+16<=48);
   for(unsigned i=0;i<16;i++){
    m->Put(in[13],in[12]+i*4,parameters[40960+first+i]);
    m->Put(in[41],in[15]+i*4,parameters[41008+first+i]);
   }
   for(unsigned i=0;i<128;i++)m->Put(ga[13],ga[12]+i*4,parameters[41056+i]);
   for(unsigned i=0;i<8192;i++)m->Put(st[12],st[5]+i*4,0);
  }
  if(alpha)for(unsigned h=0;h<48;h++){
   m->Put(route,base+PioneerQ38ProjectionResultOffset(10)+h*4,alphaBeta[h]);
   m->Put(route,base+PioneerQ38ProjectionResultOffset(11)+h*4,alphaBeta[48+h]);
  }
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
