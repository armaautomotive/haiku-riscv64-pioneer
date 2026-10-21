/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PEER_PHASE_PROFILE_H
#define PIONEER_Q38_PEER_PHASE_PROFILE_H
#ifndef PIONEER_Q38_WORK_PEER_PHASE_PROFILE
#error Separate OFFLINE peer hot-phase DATA diagnostic ONLY
#endif
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT64_TYPE__ uint64_t;
#define Q38_PEER_METRIC(name) __attribute__((noinline,optimize("Os"),section(".text.pioneer_q38_peer_phase." #name)))
#else
#include <stdint.h>
#define Q38_PEER_METRIC(name)
#endif
// DATA policy ONLY. No clock instructions,IO,address/source/owner authority,
// completion/failure/cleanup decision or numerical reference exists here.
// Caller independently owns a complete immutable job and its actual END.
// Times partition ONLY block fetch / metadata+raw unpack / matrix+affine
// regions. They INCLUDE real waits and overlapping deferred work,NOT pure
// DRAM/SFPU/FPU durations or a production inference-speed measurement.
class PioneerQ38PeerPhaseProfile {
 enum Phase {Ready,Fetch,Decode,Execute,Invalid} phase;
 unsigned worker,nonce,type,inputs,groups,blocks,panels;
 unsigned completedBlocks,nextPanel;
 uint64_t previous,total[3];
 Q38_PEER_METRIC(Init) void Init(unsigned w,unsigned n,unsigned t,unsigned k,unsigned g){
  // Typed volatile stores prevent freestanding memset/byte-store lowering.
  static_cast<volatile Phase&>(phase)=Invalid;
  static_cast<volatile unsigned&>(worker)=w;static_cast<volatile unsigned&>(nonce)=n;
  static_cast<volatile unsigned&>(type)=t;static_cast<volatile unsigned&>(inputs)=k;static_cast<volatile unsigned&>(groups)=g;
  static_cast<volatile unsigned&>(blocks)=0;static_cast<volatile unsigned&>(panels)=0;
  static_cast<volatile unsigned&>(completedBlocks)=0;static_cast<volatile unsigned&>(nextPanel)=0;
  static_cast<volatile uint64_t&>(previous)=0;
  for(unsigned p=0;p<3;p++)static_cast<volatile uint64_t&>(total[p])=0;
  if(!w||w>=120||!n||n>=0x7fffffff||(t!=8&&t!=12&&t!=14)||(k!=5120&&k!=6144&&k!=17408)||!g||g>16)return;
  blocks=k>>(t==8?5:8);panels=t==8?2:16;phase=Ready;
 }
 Q38_PEER_METRIC(Add) bool Add(unsigned bucket,uint64_t now){
  if(phase==Invalid)return false;
  if(bucket>=3||now<previous||now==~uint64_t(0)||now-previous>~uint64_t(0)-total[bucket]){phase=Invalid;return false;}
  total[bucket]+=now-previous;previous=now;return true;
 }
public:
 PioneerQ38PeerPhaseProfile(unsigned w,unsigned n,unsigned t,unsigned k,unsigned g){Init(w,n,t,k,g);}
 Q38_PEER_METRIC(BeginBlock) bool BeginBlock(unsigned group,unsigned block,uint64_t now){
  if(phase==Invalid)return false;
  if(phase!=Ready||group>=groups||block>=blocks||group*blocks+block!=completedBlocks||now<previous||now==~uint64_t(0)){phase=Invalid;return false;}
  previous=now;nextPanel=0;phase=Fetch;return true;
 }
 Q38_PEER_METRIC(Fetched) bool Fetched(uint64_t now){
  if(phase!=Fetch){phase=Invalid;return false;}
  if(!Add(0,now))return false;
  phase=Decode;return true;
 }
 Q38_PEER_METRIC(Decoded) bool Decoded(unsigned panel,uint64_t now){
  if(phase!=Decode||panel!=nextPanel||panel>=panels){phase=Invalid;return false;}
  if(!Add(1,now))return false;
  phase=Execute;return true;
 }
 Q38_PEER_METRIC(Executed) bool Executed(unsigned panel,uint64_t now){
  if(phase!=Execute||panel!=nextPanel||panel>=panels){phase=Invalid;return false;}
  if(!Add(2,now))return false;
  if(++nextPanel==panels){completedBlocks++;phase=Ready;}else phase=Decode;
  return true;
 }
 void BadClock(){phase=Invalid;} // sticky metrics invalid ONLY;NEVER execute an IO/END/reset
 bool Complete()const{return phase==Ready&&completedBlocks==groups*blocks&&total[0]&&total[1]&&total[2];}
 // Always fully define the16-word DATA envelope,including invalid timing.
 // True means complete metrics ONLY,NOT a successful device transaction.
 Q38_PEER_METRIC(Describe) bool Describe(uint32_t (&out)[16],bool independentlyCompleted)const{
  for(unsigned i=0;i<16;i++)static_cast<volatile uint32_t&>(out[i])=0;
  out[0]=0x31545051;out[1]=1;out[2]=nonce;out[3]=worker;out[4]=type;out[5]=inputs;out[6]=groups;
  bool valid=independentlyCompleted&&Complete();out[7]=valid;
  if(valid)for(unsigned p=0;p<3;p++){out[8+p*2]=uint32_t(total[p]);out[9+p*2]=uint32_t(total[p]>>32);}
  return valid;
 }
};
#undef Q38_PEER_METRIC
#endif
