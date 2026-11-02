/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_layer29_wide_reduction_bridge.h"
#include <array>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
static uint32_t Bits(float value){uint32_t bits;memcpy(&bits,&value,4);return bits;}
static float Float(uint32_t bits){float value;memcpy(&value,&bits,4);return value;}
static void Reference(const float* q,const float* k,const float* value,float decay,float beta,
 std::vector<float>& state,std::array<float,128>& output,bool narrow){
 for(unsigned v=0;v<128;v++){
  double prediction=0;float narrowPrediction=0;
  for(unsigned key=0;key<128;key++){
   const float decayed=state[key*128+v]*decay;
   prediction+=double(decayed)*k[key];narrowPrediction+=decayed*k[key];
  }
  const float delta=(value[v]-(narrow?narrowPrediction:float(prediction)))*beta;
  double sum=0;float narrowSum=0;
  for(unsigned key=0;key<128;key++){
   const unsigned at=key*128+v;state[at]=state[at]*decay+k[key]*delta;
   sum+=double(state[at])*q[key];narrowSum+=state[at]*q[key];
  }
  output[v]=(narrow?narrowSum:float(sum))*0.0883883461356163f;
 }
}
int main(){
 assert(std::fegetround()==FE_TONEAREST);unsigned histories=0,steps=0,narrowDifferences=0;
 uint64_t modelOperations=0;
 for(unsigned head=0;head<4;head++){
  std::vector<float> expected(16384),actual(16384),narrow(16384);
  for(unsigned i=0;i<16384;i++)expected[i]=actual[i]=narrow[i]=head?float(std::sin(i*.031+head*.17)*.003):0.f;
  for(unsigned token=0;token<16;token++){
   std::array<float,128> q,k,value,out,reference,narrowOut;
   for(unsigned i=0;i<128;i++){
    q[i]=float(std::sin(i*.123+head*.317+token*.11)*.01);
    k[i]=float(std::cos(i*.153+head*.217+token*.13)*.03);
    value[i]=float(std::sin(i*.013+head*.117+token*.17)*2.1);
   }
   const float decay=token==0?0.f:token==15?1.f:.83f,beta=token==0?1.f:token==15?0.f:.37f;
   Reference(q.data(),k.data(),value.data(),decay,beta,expected,reference,false);
   Reference(q.data(),k.data(),value.data(),decay,beta,narrow,narrowOut,true);
   uint64_t operations=0;
   assert(Q38Layer29WideReductionCandidate(q.data(),k.data(),value.data(),decay,beta,actual.data(),out.data(),&operations));
   for(unsigned i=0;i<16384;i++)assert(Bits(actual[i])==Bits(expected[i]));
   for(unsigned i=0;i<128;i++){assert(Bits(out[i])==Bits(reference[i]));narrowDifferences+=Bits(narrowOut[i])!=Bits(reference[i]);}
   assert(operations);modelOperations+=operations;steps++;
  }
  histories++;
 }
 assert(narrowDifferences);
 unsigned refused=0;
 for(unsigned missing=0;missing<7;missing++){
  std::array<float,128> q={},k={},value={},out;out.fill(Float(0xdeadbeef));
  std::vector<float> state(16384,0);const auto before=state;const auto outputBefore=out;uint64_t operations=123;
  if(missing==6)assert(!std::fesetround(FE_DOWNWARD));
  assert(!Q38Layer29WideReductionCandidate(missing==0?nullptr:q.data(),missing==1?nullptr:k.data(),
   missing==2?nullptr:value.data(),.83f,.37f,missing==3?nullptr:state.data(),missing==4?nullptr:out.data(),missing==5?nullptr:&operations));
  assert(!std::fesetround(FE_TONEAREST));
  assert(!memcmp(state.data(),before.data(),state.size()*4) && !memcmp(out.data(),outputBefore.data(),sizeof(out)) && operations==123);refused++;
 }
 for(unsigned group=0;group<6;group++)for(unsigned index:{0U,63U,127U}){
  for(uint32_t bad:{1U,0x80000001U,0x7f800000U,0x7fc00001U,0x7f7fffffU}){
   std::array<float,128> q={},k={},value={},out;out.fill(Float(0xdeadbeef));
   std::vector<float> state(16384,0);float decay=.83f,beta=.37f;
   if(group==0)q[index]=Float(bad);else if(group==1)k[index]=Float(bad);
   else if(group==2)value[index]=Float(bad);else if(group==3)state[index*128]=Float(bad);
   else if(group==4)decay=Float(bad);else beta=Float(bad);
   const auto before=state;const auto outputBefore=out;uint64_t operations=UINT64_C(0x123456789abcdef);
   assert(!Q38Layer29WideReductionCandidate(q.data(),k.data(),value.data(),decay,beta,state.data(),out.data(),&operations));
   assert(!memcmp(state.data(),before.data(),state.size()*4) && !memcmp(out.data(),outputBefore.data(),sizeof(out))
    && operations==UINT64_C(0x123456789abcdef));refused++;
  }
 }
 printf("PASS HOST partial-Blackhole-FMA compensated candidate:%u separate histories/%u successive nonresetting transitions/all16384 next-state+128 output words BITEXACT to independent double-order reference/narrow negative-control%u differing outputs/%u malformed-input refusals preserve ALL outputs/model evaluations%llu;NOT emitted/native/physical/Work/TPS or arbitrary-input exactness\n",
  histories,steps,narrowDifferences,refused,static_cast<unsigned long long>(modelOperations));
}
