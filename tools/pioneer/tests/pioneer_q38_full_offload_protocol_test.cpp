/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenHiddenValidationProtocol.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
namespace P=PioneerQ38TokenControl;
namespace F=PioneerQ38FullOffload;
static std::unique_ptr<P::Packet> Request(unsigned count,unsigned outputs){
 auto p=std::make_unique<P::Packet>();auto& r=p->request;
 r.magic=P::Magic;r.version=P::Version;r.operation=P::Start;r.prompts=count;r.limit=count+outputs-1;r.stopEos=1;
 r.eos0=248044;r.eos1=248046;
 for(unsigned i=0;i<count;i++)p->prompt[i]=17+i;
 return p;
}
int main(){
#ifdef PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4
 static_assert(F::BuildTag==UINT64_C(0x2026110500010103),"exact fixed-config Q4 USER/native identity");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4)
 static_assert(F::BuildTag==UINT64_C(0x2026110500010102),"exact queued Q4 USER/native identity");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH)
 static_assert(F::BuildTag==UINT64_C(0x2026110500010101),"exact instruction-prefetch USER/native identity");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY)
 static_assert(F::BuildTag==UINT64_C(0x2026110500010100),"exact replay13 USER/native identity");
#endif
 unsigned valid=0,refused=0;
 for(unsigned count=1;count<=96;count++)for(unsigned output=1;output<=32;output++){
  auto p=Request(count,output);assert(F::RequestShape(*p));valid++;
  assert(!PioneerQ38TokenHiddenValidation::ControlRequestShape(*p));
 }
 for(unsigned count:{0U,97U,128U}){auto p=Request(count,1);assert(!F::RequestShape(*p));refused++;}
 for(unsigned output:{0U,33U,128U}){auto p=Request(2,output);assert(!F::RequestShape(*p));refused++;}
 for(unsigned at=96;at<P::Context;at++){
  auto p=Request(96,32);p->prompt[at]=1;assert(!F::RequestShape(*p));refused++;
 }
 for(unsigned at=0;at<sizeof(P::Reply);at++){
  auto p=Request(3,4);reinterpret_cast<unsigned char*>(&p->reply)[at]=1;assert(!F::RequestShape(*p));refused++;
 }
 for(unsigned at=0;at<P::Batch*P::RecordWords;at++){
  auto p=Request(3,4);p->records[at]=1;assert(!F::RequestShape(*p));refused++;
 }
 for(unsigned action=P::Observe;action<=P::Finish;action++)for(unsigned count=0;count<=64;count++){
  auto p=std::make_unique<P::Packet>();p->request.magic=P::Magic;p->request.version=1;p->request.operation=action;
  p->request.records=count;
  assert(F::RequestShape(*p)==(action==P::Read || !count));
 }
 F::Support support={};assert(F::SupportRequest(support) && !F::Supported(support));
 support={55,1,96,32,128,248320,F::AllCompute,0};assert(F::Supported(support) && !F::SupportRequest(support));
 for(unsigned word=0;word<8;word++){
  auto changed=support;reinterpret_cast<uint32_t*>(&changed)[word]^=1;
  assert(!F::Supported(changed));refused++;
 }
 auto p=Request(3,4);p->request.stopEos=0;assert(!F::RequestShape(*p));refused++;
 p=Request(3,4);p->prompt[2]=F::Vocabulary;assert(!F::RequestShape(*p));refused++;
 p=Request(3,4);p->request.eos1=F::Vocabulary;assert(!F::RequestShape(*p));refused++;
 p=Request(3,4);p->request.eos1=p->request.eos0;assert(!F::RequestShape(*p));refused++;
 // Fixed diagnostic keeps its old grammar;not relabeled or widened.
 p=std::make_unique<P::Packet>();p->request.magic=P::Magic;p->request.version=1;p->request.operation=P::Start;
 p->request.prompts=2;p->request.limit=3;p->prompt[0]=248319;p->prompt[1]=17;
 assert(PioneerQ38TokenHiddenValidation::ControlRequestShape(*p) && !F::RequestShape(*p));
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
 static_assert(TT_PCI_PROBE_Q38_FULL_OFFLOAD_COUNTER!=TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE,
  "shared integer DATA never aliases the old diagnostic ioctl");
 unsigned observations=0;
 F::CounterPacket metric;assert(F::CounterRequest(metric));
 for(unsigned byte=0;byte<sizeof(metric);byte++){
  auto bad=metric;reinterpret_cast<uint8_t*>(&bad)[byte]^=1;assert(!F::CounterRequest(bad));refused++;
 }
 for(unsigned prompts=1;prompts<=96;prompts++)for(unsigned outputs=1;outputs<=32;outputs++){
  const unsigned limit=prompts+outputs-1;
  for(unsigned committed=0;committed<=limit;committed++){
   F::CounterPacket m;auto& r=m.reply;r.version=r.operation=r.started=r.ready=1;
   r.detail[0]=r.detail[1]=r.detail[6]=1;r.detail[2]=12;r.detail[4]=63;
   r.detail[7]=committed;r.detail[8]=limit;r.detail[9]=prompts;
   assert(F::CounterReply(m) && !F::CounterRequest(m));observations++;
   r.detail[7]=limit+1;assert(!F::CounterReply(m));refused++;
  }
 }
 for(unsigned mode=0;mode<4;mode++){
  F::CounterPacket m;auto& r=m.reply;r.version=r.operation=r.started=r.ready=1;
  r.detail[0]=r.detail[1]=r.detail[6]=1;r.detail[2]=12;r.detail[4]=63;
  r.detail[8]=mode==0?129:mode==1?97:mode==2?128:4;r.detail[9]=mode==1?97:mode==2?96:1;
  if(mode==3)r.detail[10]=1;assert(!F::CounterReply(m));refused++;
 }
 printf("PASS full-card metrics DATA %u bounded count observations/all request bytes and out-of-range counts refused;NO timing,lifecycle or hardware authority\n",observations);
#endif
 printf("PASS full-offload pointer-free grammar %u valid/%u refusals;old fixed diagnostic unchanged;NO inference or hardware\n",valid,refused);
}
