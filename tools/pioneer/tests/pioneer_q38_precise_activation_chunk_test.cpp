/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE 1
#define PIONEER_Q38_SIGMOID_PRECISION_IMAGE 1
#include "pioneer_q38_precise_activation_reference.h"
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned accepted=0,refused=0,ranges=0;
 for(unsigned type:{8U,12U,14U})for(unsigned trial=0;trial<74;trial++){
  float gate[256],up[256],wanted[256],actual[256];PreciseChunkInputs(trial,gate,up);
  float gateCopy[256],upCopy[256];memcpy(gateCopy,gate,sizeof(gate));memcpy(upCopy,up,sizeof(up));
  memset(actual,0xa5,sizeof(actual));float poison[256];memcpy(poison,actual,sizeof(actual));
  PioneerQ38ProjectionInput out,expected,independent;memset(&out,0xa5,sizeof(out));const auto before=out;
  bool okay=CurrentRootActivation(gate,up,wanted) && PioneerQ38ProjectionQuantize(type,wanted,256,&expected);
  assert(PioneerQ38PreciseActivationChunk(type,gate,up,256,actual,&out)==okay);
  if(okay){
   ProjectionInputReference(type,wanted,independent);
   assert(!memcmp(actual,wanted,sizeof(actual)) && !memcmp(&out,&expected,sizeof(out))
    && !memcmp(&out,&independent,sizeof(out)));accepted++;
  }else{assert(!memcmp(actual,poison,sizeof(actual)) && !memcmp(&out,&before,sizeof(out)));refused++;}
  assert(!memcmp(gate,gateCopy,sizeof(gate)) && !memcmp(up,upCopy,sizeof(up)));
 }
 float gate[256]={},up[256],activated[256];for(float& value:up)value=1;
 memset(activated,0xa5,sizeof(activated));float untouched[256];memcpy(untouched,activated,sizeof(activated));
 PioneerQ38ProjectionInput out;memset(&out,0xa5,sizeof(out));const auto before=out;
 auto noSink=[&](){assert(!memcmp(activated,untouched,sizeof(activated)) && !memcmp(&out,&before,sizeof(out)));};
 for(unsigned which:{0U,1U})for(unsigned at=0;at<256;at++)for(unsigned bad:{1U,0x80000001U,0x007fffffU,
  0x7f800000U,0xff800000U,0x7fc00001U,0x267fffffU,0x47800000U}){
  float* target=which?up:gate;const float saved=target[at];target[at]=PioneerAttentionFloat(bad);
  assert(!PioneerQ38PreciseActivationChunk(12,gate,up,256,activated,&out));noSink();target[at]=saved;refused++;
 }
 for(unsigned count:{0U,1U,255U,257U,17408U,UINT32_MAX}){
  assert(!PioneerQ38PreciseActivationChunk(12,gate,up,count,activated,&out));noSink();ranges++;
 }
 for(unsigned type:{0U,1U,9U,UINT32_MAX}){
  assert(!PioneerQ38PreciseActivationChunk(type,gate,up,256,activated,&out));noSink();ranges++;
 }
 for(unsigned which=0;which<4;which++)for(uintptr_t bad:{uintptr_t(0),uintptr_t(gate)+1,~uintptr_t(3)}){
  const uintptr_t address[]={uintptr_t(gate),uintptr_t(up),uintptr_t(activated),uintptr_t(&out)};
  uintptr_t args[4];memcpy(args,address,sizeof(args));args[which]=bad;
  assert(!PioneerQ38PreciseActivationChunk(12,reinterpret_cast<float*>(args[0]),reinterpret_cast<float*>(args[1]),
   256,reinterpret_cast<float*>(args[2]),reinterpret_cast<PioneerQ38ProjectionInput*>(args[3])));noSink();ranges++;
 }
 for(unsigned a=0;a<4;a++)for(unsigned b=a+1;b<4;b++)for(unsigned delta:{0U,4U,512U}){
  alignas(4) float arena[2048];for(float& value:arena)value=1;float copy[2048];memcpy(copy,arena,sizeof(arena));
  uintptr_t args[4];for(unsigned i=0;i<4;i++)args[i]=uintptr_t(arena+i*512);args[b]=args[a]+delta;
  assert(!PioneerQ38PreciseActivationChunk(12,reinterpret_cast<float*>(args[0]),reinterpret_cast<float*>(args[1]),
   256,reinterpret_cast<float*>(args[2]),reinterpret_cast<PioneerQ38ProjectionInput*>(args[3]))
   && !memcmp(arena,copy,sizeof(arena)));ranges++;
 }
 printf("PASS local precise activation chunks/accepted%u/refused%u/ranges%u/current ROOT-order F32 bit parity+independent packing/atomic outputs+all-buffer alias refusal;NO distributed worker,native,Work,full-model or speed grant\n",accepted,refused,ranges);
}
