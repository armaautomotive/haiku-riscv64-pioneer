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
 printf("PASS full-offload pointer-free grammar %u valid/%u refusals;old fixed diagnostic unchanged;NO inference or hardware\n",valid,refused);
}
