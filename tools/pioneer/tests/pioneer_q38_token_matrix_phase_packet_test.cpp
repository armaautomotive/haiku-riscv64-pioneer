/* SPDX-License-Identifier: MIT */
// Pure packet grammar ONLY. All metric/STOP DATA below are FABRICATED.
// No native state,hardware,driver dispatch,model or timing is exercised.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <type_traits>
enum {B_DEVICE_OP_CODES_END=0x1000};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenMatrixPhasePacket.h"
namespace P=PioneerQ38TokenMatrixPhaseControl;
namespace Q=PioneerQ38TokenMatrixPhaseData;
static P::Packet Sample(unsigned worker,unsigned inputs,unsigned groups,unsigned state){
 P::Packet p;p.request.worker=worker;
 auto& r=p.reply;r.version=P::Version;r.operation=P::Read;r.worker=worker;r.stopNonce=19;
 r.started=r.ready=1;
 auto* d=r.detail;d[0]=1;d[1]=1;d[2]=4;d[3]=worker;d[4]=63;d[6]=1;
 if(state!=Q::Missing){
  auto* w=p.words;w[0]=Q::Magic;w[1]=Q::Version;w[2]=17;w[3]=worker;w[4]=12;w[5]=inputs;w[6]=groups;
  if(state==Q::Intervals){w[7]=1;w[8]=7;w[9]=3;w[10]=11;w[11]=5;w[12]=13;w[13]=7;}
  if(state==Q::Malformed)w[0]^=1;
 }
 d[7]=state==Q::Intervals;d[8]=state;d[9]=p.words[2];d[10]=p.words[5];d[11]=p.words[6];
 assert(Q::Classify(p.words,worker,r.stopNonce)==state);return p;
}
int main(){
 static_assert(std::is_standard_layout<P::Packet>::value && std::is_trivially_copyable<P::Packet>::value,
  "fixed copyable DATA only");
 static_assert(TT_PCI_PROBE_Q38_TOKEN_MATRIX_PHASE_READ==B_DEVICE_OP_CODES_END+0x660a,"distinct un-routed opcode");
 unsigned good=0,bad=0;
 for(unsigned worker=1;worker<120;worker++){
  P::Packet p;p.request.worker=worker;assert(P::RequestShape(p));good++;
  for(unsigned inputs:{5120U,6144U,17408U})for(unsigned groups=1;groups<=16;groups++)
   for(unsigned state:{unsigned(Q::Missing),unsigned(Q::InvalidClock),unsigned(Q::Intervals),unsigned(Q::Malformed)}){
    auto sample=Sample(worker,inputs,groups,state);assert(P::ReplyShape(sample) && !P::RequestShape(sample));good++;
   }
 }
 for(unsigned worker:{0U,120U,UINT32_MAX}){P::Packet p;p.request.worker=worker;assert(!P::RequestShape(p));bad++;}
 P::Packet pristine;pristine.request.worker=1;
 for(unsigned member=0;member<8;member++)if(member!=2)for(unsigned bit=0;bit<32;bit++){
  auto p=pristine;uint32_t fields[8];memcpy(fields,&p.request,sizeof(fields));
  fields[member]^=UINT32_C(1)<<bit;memcpy(&p.request,fields,sizeof(fields));
  assert(!P::RequestShape(p));bad++;
 }
 for(unsigned byte=0;byte<sizeof(P::Reply);byte++){
  auto p=pristine;reinterpret_cast<uint8_t*>(&p.reply)[byte]=1;assert(!P::RequestShape(p));bad++;
 }
 for(unsigned i=0;i<P::DataWords;i++)for(unsigned bit=0;bit<32;bit++){
  auto p=pristine;p.words[i]=UINT32_C(1)<<bit;assert(!P::RequestShape(p));bad++;
 }
 const auto valid=Sample(119,17408,16,Q::Intervals);
 for(unsigned member=0;member<24;member++)if(member!=7)for(unsigned bit=0;bit<32;bit++){
  auto p=valid;uint32_t fields[24];memcpy(fields,&p.reply,sizeof(fields));
  fields[member]^=UINT32_C(1)<<bit;memcpy(&p.reply,fields,sizeof(fields));
  assert(!P::ReplyShape(p));bad++;
 }
 for(unsigned nonce:{0U,1U,17U,0x7fffffffU,UINT32_MAX}){
  auto p=valid;p.reply.stopNonce=nonce;assert(!P::ReplyShape(p));bad++;
 }
 {auto p=valid;p.reply.stopNonce++;assert(P::ReplyShape(p));good++;}
 // A different syntactically possible bound is deliberately accepted:only
 // actual native provenance can authenticate STOP,not this DATA parser.
 for(unsigned word:{8U,10U,12U}){auto p=valid;p.words[word]++;assert(P::ReplyShape(p));good++;}
 for(unsigned worker=1;worker<120;worker++)for(unsigned phase=1;phase<=3;phase++)
  for(unsigned failedWorker=0;failedWorker<120;failedWorker++){
   P::Packet p;p.request.worker=worker;auto& r=p.reply;r.version=P::Version;r.operation=P::Read;
   r.worker=worker;r.status=-3;r.started=r.unsafe=1;
   r.detail[0]=1;r.detail[1]=r.detail[5]=1;r.detail[2]=phase;r.detail[3]=failedWorker;r.detail[4]=63;
   assert(P::ReplyShape(p));good++;
   auto dirty=p;dirty.words[15]=1;assert(!P::ReplyShape(dirty));bad++;
   dirty=p;dirty.reply.stopNonce=19;assert(!P::ReplyShape(dirty));bad++;
  }
 {P::Packet p;p.request.worker=1;auto& r=p.reply;r.version=P::Version;r.operation=P::Read;r.worker=1;
  r.status=-2;r.detail[0]=1;assert(P::ReplyShape(p));good++;
  r.unsafe=r.detail[5]=1;assert(!P::ReplyShape(p));bad++;}
 printf("PASS pure fixed192-byte matrix timing packet:%u fabricated DATA successes/%u malformed request,reply,leaked-output refusals/119 workers,all3 K shapes,1..16 groups,4 metric states/STOP bound checked separately from metric nonce/changed plausible DATA is NOT provenance;NO ioctl route,native owner,HOLD,actual STOP,cleanup,physical timing,model,Work or TPS proof\n",good,bad);
}
