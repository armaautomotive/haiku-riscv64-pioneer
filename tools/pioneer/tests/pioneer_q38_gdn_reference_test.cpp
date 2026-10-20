/* SPDX-License-Identifier: MIT */
// The OP and scalar vector bodies below are extracted WITHOUT modification
// from the pristine pinned GGML commit by the test runner. Scalar dot uses
// its REAL FP64 accumulator;do NOT label the SFPU's FP32 reductions bit-exact
// to this CPU implementation. All arrays in this file are TEST memory only.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function" // pinned upstream header's unrelated static helpers
#include "ggml-cpu/ggml-cpu-impl.h"
#include "ggml-cpu/ops.h"
#include "../PioneerQ38GgmlGdnContract.h"
#undef GGML_SIMD
#undef GGML_USE_ACCELERATE
typedef double ggml_float;
#include "pinned_gdn_vectors.inc"
#include "pinned_gdn_chunk.inc"
#pragma GCC diagnostic pop
#define PIONEER_Q38_DELTA_VECTOR_PROBE_IMAGE
#define PIONEER_Q38_DELTA_VECTOR_POST_SCALE
#define PIONEER_Q38_DELTA_VECTOR_IMAGE_MACHINE_ONLY
#include "pioneer_q38_delta_vector_image_test.cpp"
static void Normalize(std::vector<float>& a) {
 for (unsigned head=0;head<16;head++) {
  double squared=0; for (unsigned i=0;i<128;i++) squared+=double(a[head*128+i])*a[head*128+i];
  float inverse=1.f/float(sqrt(squared)); for (unsigned i=0;i<128;i++) a[head*128+i]*=inverse;
 }
}
int main(int argc,char** argv)
{
 assert(argc==2);
 constexpr unsigned StateWords=PioneerQ38GgmlGdnContract::StateWords,OutputWords=PioneerQ38GgmlGdnContract::OutputWords;
 ggml_init_params init={2*1024*1024,nullptr,true}; auto* ctx=ggml_init(init); assert(ctx);
 auto* q=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
 auto* k=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
 auto* v=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,48,1,1);
 auto* g=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
 auto* beta=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
 auto* s=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,128,48,1);
 auto* op=ggml_gated_delta_net(ctx,q,k,v,g,beta,s,1);
 PioneerQ38GgmlGdnContract shape; assert(PioneerQ38GgmlGdnContract::Describe(op,shape));
 std::vector<float> query(2048),key(2048),value(6144),gate(48),b(48),cpuState(StateWords),cpuResult(OutputWords+StateWords),
  current(StateWords),next(StateWords),output(OutputWords),scratch(128+CACHE_LINE_SIZE_F32);
 q->data=query.data(); k->data=key.data(); v->data=value.data(); g->data=gate.data(); beta->data=b.data();
 s->data=cpuState.data(); op->data=cpuResult.data();
 ggml_compute_params params={}; params.ith=0; params.nth=1; params.wsize=scratch.size()*4; params.wdata=scratch.data();
 double stateMax=0,outputMax=0; uint64_t changedStateBits=0,changedOutputBits=0; unsigned actualHalves=0;
 auto compare=[](float actual,float expected,double& maximum,uint64_t& changed) {
  assert(std::isfinite(actual) && std::isfinite(expected)); double difference=fabs(double(actual)-expected);
  assert(difference<=2e-6+2e-5*fabs(double(expected))); maximum=std::max(maximum,difference); changed+=Bits(actual)!=Bits(expected);
 };
 for (unsigned token=0;token<8;token++) {
  for (unsigned h=0;h<16;h++) for (unsigned i=0;i<128;i++) {
   query[h*128+i]=float(sin(i*.073+h*.13+token*.17)); key[h*128+i]=float(cos(i*.091+h*.17+token*.11));
  }
  Normalize(query); Normalize(key);
  for (unsigned h=0;h<48;h++) {
   gate[h]=-.03f-.01f*float((h+token)%31); b[h]=.07f+.013f*float((h*3+token)%61);
   for (unsigned j=0;j<128;j++) value[h*128+j]=float(sin(j*.037+h*.113+token*.19)*.7);
  }
  // ACTUAL upstream chunk body,not a rewritten reference GDN formula.
  ggml_compute_forward_gated_delta_net_one_chunk(&params,op,0,48);
  for (unsigned h=0;h<48;h++) for (unsigned half=0;half<2;half++) {
   DeltaVectorMachine m; for (auto& row:m.defined) row=true;
   for (unsigned i=0;i<128;i++) {
    m.q[i]=Bits(query[(h%16)*128+i]); m.k[i]=Bits(key[(h%16)*128+i]);
    for (unsigned j=0;j<64;j++) m.state[i*64+j]=Bits(current[h*16384+(half*64+j)*128+i]);
   }
   for (unsigned j=0;j<64;j++) m.lr[6+(j&1)][(j/16)*8+(j%16)/2]=Bits(value[h*128+half*64+j]);
   uint32_t decay=Bits(expf(gate[h])),bv=Bits(b[h]);
   // Four distinct key-head groups through ACTUAL compiled RV32/SFPU too.
   bool actual=h==0 || h==17 || h==32 || h==47;
   if (actual) {
    DeltaVectorImage machine(argv[1],1);
    for (unsigned i=0;i<8192;i++) machine.memory[0x90000+i*4]=m.state[i];
    for (unsigned i=0;i<128;i++) { machine.memory[0x8e000+i*4]=m.q[i]; machine.memory[0x8e200+i*4]=m.k[i]; }
    for (unsigned j=0;j<64;j++) machine.memory[0x8e400+j*4]=Bits(value[h*128+half*64+j]);
    machine.memory[0x8e500]=decay; machine.memory[0x8e504]=bv;
    assert(machine.RunVector()); machine.Verify();
    assert(PioneerQ38DeltaVectorHalf<true>(m,decay,bv));
    for (unsigned i=0;i<8192;i++) assert(machine.memory.at(0xa0000+i*4)==m.state[i]);
    for (unsigned j=0;j<64;j++) assert(machine.memory.at(0x8e800+j*4)==m.lr[4+(j&1)][(j/16)*8+(j%16)/2]);
    actualHalves++;
   } else assert(PioneerQ38DeltaVectorHalf<true>(m,decay,bv));
   for (unsigned i=0;i<128;i++) for (unsigned j=0;j<64;j++) next[h*16384+(half*64+j)*128+i]=Float(m.state[i*64+j]);
   for (unsigned j=0;j<64;j++) output[h*128+half*64+j]=Float(m.lr[4+(j&1)][(j/16)*8+(j%16)/2]);
  }
  for (unsigned i=0;i<StateWords;i++) compare(next[i],cpuResult[OutputWords+i],stateMax,changedStateBits);
  for (unsigned i=0;i<OutputWords;i++) compare(output[i],cpuResult[i],outputMax,changedOutputBits);
  std::copy(cpuResult.begin()+OutputWords,cpuResult.end(),cpuState.begin()); current.swap(next);
 }
 assert(actualHalves==64 && changedStateBits && changedOutputBits); // REAL scalar CPU accumulation differs
 ggml_free(ctx);
 printf("PASS PINNED ACTUAL GGML GDN scalar-FP64-dot vs vector-FP32:48 heads x8 successive tokens,64 actual RV32 half-heads,max_state_abs=%.9g max_output_abs=%.9g,different_state_bits=%llu output_bits=%llu;fixed tolerance2e-6+2e-5*abs(ref),NOT bit-exact/full-model/native residency or quality qualification\n",
  stateMax,outputMax,(unsigned long long)changedStateBits,(unsigned long long)changedOutputBits);
}
