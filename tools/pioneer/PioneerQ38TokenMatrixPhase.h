/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenMatrixPhaseRole.h"
#ifndef PIONEER_Q38_TOKEN_MATRIX_PHASE_H
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_H
#if !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE) && !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_DATA_TEST)
#error Matrix phase policy has no normal model/Work selector
#endif
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT64_TYPE__ uint64_t;
#define Q38_MATRIX_PHASE_FN(name) __attribute__((noinline,optimize("Os"),section(".text.pioneer_q38_token_matrix_phase." #name)))
#else
#include <stdint.h>
#define Q38_MATRIX_PHASE_FN(name)
#endif
// Pure DATA. Three inclusive intervals in ONE complete Q4 matrix job:
// fetch/wait, metadata+UNPACK/pack, matrix+affine. Not pure engine times.
// Setup/input validation/final output publication are outside these buckets.
// No clock,IO,address,lifetime,completion or cleanup authority is provided.
// Clock failure invalidates metrics ONLY;caller must finish the real job.
class PioneerQ38TokenMatrixPhase {
 enum Phase {Ready,Fetch,Decode,Execute,Invalid} phase=Invalid;
 unsigned worker=0,nonce=0,inputs=0,groups=0,blocks=0,completed=0,next=0;
 uint64_t previous=0,total[3]={};
 Q38_MATRIX_PHASE_FN(Add) bool Add(unsigned bucket,uint64_t now){
  if(phase==Invalid)return false;
  if(bucket>=3 || now<previous || now==~uint64_t(0) || now-previous>~uint64_t(0)-total[bucket]){
   phase=Invalid;return false;
  }
  total[bucket]+=now-previous;previous=now;return true;
 }
public:
 enum {Magic=0x324d5051,Version=2,Words=16,Address=0x84500};
 Q38_MATRIX_PHASE_FN(Init) void Init(unsigned w,unsigned n,unsigned k,unsigned g){
  phase=Invalid;worker=w;nonce=n;inputs=k;groups=g;blocks=completed=next=0;previous=0;
  for(unsigned b=0;b<3;b++)static_cast<volatile uint64_t&>(total[b])=0;
  if(!w || w>=120 || !n || n>=0x7fffffff || (k!=5120 && k!=6144 && k!=17408) || !g || g>16)return;
  blocks=k>>8;phase=Ready;
 }
 Q38_MATRIX_PHASE_FN(Begin) bool Begin(unsigned group,unsigned block,uint64_t now){
  if(phase==Invalid)return false;
  if(phase!=Ready || group>=groups || block>=blocks || group*blocks+block!=completed || now<previous || now==~uint64_t(0)){
   phase=Invalid;return false;
  }
  previous=now;next=0;phase=Fetch;return true;
 }
 Q38_MATRIX_PHASE_FN(Fetched) bool Fetched(uint64_t now){
  if(phase!=Fetch){phase=Invalid;return false;}
  if(!Add(0,now))return false;
  phase=Decode;return true;
 }
 Q38_MATRIX_PHASE_FN(Decoded) bool Decoded(unsigned panel,uint64_t now){
  if(phase!=Decode || panel!=next || panel>=16){phase=Invalid;return false;}
  if(!Add(1,now))return false;
  phase=Execute;return true;
 }
 Q38_MATRIX_PHASE_FN(Executed) bool Executed(unsigned panel,uint64_t now){
  if(phase!=Execute || panel!=next || panel>=16){phase=Invalid;return false;}
  if(!Add(2,now))return false;
  if(++next==16){completed++;phase=Ready;}else phase=Decode;
  return true;
 }
 void BadClock(){phase=Invalid;}
 bool Complete()const{return phase==Ready && completed==groups*blocks && total[0] && total[1] && total[2];}
 Q38_MATRIX_PHASE_FN(Describe) bool Describe(uint32_t (&out)[Words],bool independentlyCompleted)const{
  for(unsigned i=0;i<Words;i++)static_cast<volatile uint32_t&>(out[i])=0;
  out[0]=Magic;out[1]=Version;out[2]=nonce;out[3]=worker;out[4]=12;out[5]=inputs;out[6]=groups;
  const bool valid=independentlyCompleted && Complete();out[7]=valid;
  if(valid)for(unsigned b=0;b<3;b++){out[8+b*2]=uint32_t(total[b]);out[9+b*2]=uint32_t(total[b]>>32);}
  return valid;
 }
};
#undef Q38_MATRIX_PHASE_FN
#endif
