/* SPDX-License-Identifier: MIT */
// Actual typed packet+extracted adapter. FD/source/OS/observer/quarantine
// below are explicitly MOCK; no genuine owner or device is constructed.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierPacket.h"
#include "actual_frontier_result.inc"
using status_t=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3};
struct Context {unsigned observes=0,retains=0;};
struct PioneerProbeCookie {uint32_t magic=0x50313030;int team=23;Context* context=nullptr;};
struct thread_info {int team=0;};
static Context context;static PioneerProbeCookie cookie;
static unsigned uid=0,copies=0,failCopy=0,scopeEntries=0,scopeExits=0,locks=0;
static bool failThread=false,admit=true,loseAdmission=false,mutateUser=false;
static unsigned mode=0,value=0,limit=34;
static unsigned TestUid(){return uid;}
static int find_thread(const char*){return 17;}
static status_t get_thread_info(int thread,thread_info* output){
 assert(thread==17);if(failThread)return -81;output->team=23;return B_OK;
}
static status_t TestCopy(void* to,const void* from,size_t bytes){
 namespace P=PioneerQ38TokenFrontierControl;
 assert(bytes==sizeof(P::Packet));copies++;
 assert(locks==unsigned(copies==2));
 if(copies==failCopy)return -99;
 memcpy(to,from,bytes);
 if(copies==1 && loseAdmission)admit=false;
 if(copies==1 && mutateUser){
  auto* user=const_cast<P::Packet*>(static_cast<const P::Packet*>(from));
  user->request.operation=999;user->reply.ready=1;user->reply.detail[7]=8192;
 } // USER mutation after kernel snapshot,never a trusted observation
 return B_OK;
}
struct PioneerQ38TokenCallScope {
 PioneerProbeCookie* cookie;bool owned=false;
 explicit PioneerQ38TokenCallScope(PioneerProbeCookie* p):cookie(p){
  scopeEntries++;assert(!locks);if(admit){locks=1;owned=true;}
 }
 ~PioneerQ38TokenCallScope(){scopeExits++;if(owned){assert(locks==1);locks=0;}}
 ::Context* Context(){return owned?cookie->context:nullptr;}
 status_t Status()const{return owned?B_OK:-82;}
};
static bool ObserveQ38TokenNativeFrontier(Context& c,PioneerQ38TokenFrontierResult& result){
 assert(locks==1);c.observes++;
 if(mode==2)return false; // metadata admission refusal,NO device result
 result.started=1;result.phase=12;result.word=63;
 if(mode==1){result.unsafe=1;return false;}
 result.ready=1;result.committed=value;result.limit=limit;result.prompts=2;return true;
}
static void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope& scope){
 assert(locks==1 && scope.Context()==&context);context.retains++;
} // MOCK metadata retention only;NO following observer/device operations
#define geteuid TestUid
#define user_memcpy TestCopy
#include "actual_frontier_control.inc"
#undef user_memcpy
#undef geteuid
static void Reset(){
 assert(!locks);context={};cookie={};cookie.context=&context;
 uid=copies=failCopy=scopeEntries=scopeExits=mode=value=0;limit=34;
 failThread=loseAdmission=mutateUser=false;admit=true;
}
int main(){
 namespace P=PioneerQ38TokenFrontierControl;
 static_assert(TT_PCI_PROBE_Q38_TOKEN_FRONTIER_OBSERVE==B_DEVICE_OP_CODES_END+0x6608);
 unsigned positives=0,refusals=0,copyFaults=0;
 for(unsigned count:{0U,1U,2U,17U,34U,8192U}){
  Reset();value=count;limit=count==8192?8192:34;P::Packet packet;
  assert(P::RequestShape(packet) && !P::ReplyShape(packet));
  assert(Q38TokenFrontierControl(&cookie,&packet,sizeof(packet))==B_OK && P::ReplyShape(packet)
   && packet.reply.ready && packet.reply.detail[7]==count && context.observes==1 && !context.retains
   && copies==2 && scopeEntries==1 && scopeExits==1 && !locks);positives++;
 }
 for(unsigned bit=0;bit<sizeof(P::Packet)*8;bit++){
  Reset();P::Packet packet;reinterpret_cast<uint8_t*>(&packet)[bit/8]^=uint8_t(1U<<(bit%8));const auto before=packet;
  assert(!P::RequestShape(packet) && Q38TokenFrontierControl(&cookie,&packet,sizeof(packet))==B_BAD_VALUE
   && copies==1 && !scopeEntries && !context.observes && !memcmp(&packet,&before,sizeof(packet)));refusals++;
 }
 for(unsigned bytes=0;bytes<=129;bytes++)if(bytes!=sizeof(P::Packet)){
  Reset();P::Packet packet;
  assert(Q38TokenFrontierControl(&cookie,&packet,bytes)==B_BAD_VALUE && !copies && !scopeEntries);refusals++;
 }
 for(unsigned test=0;test<6;test++){
  Reset();P::Packet packet;const auto before=packet;
  if(test==0)uid=1;if(test==1)cookie.magic^=1;if(test==2)cookie.team=24;if(test==3)failThread=true;
  assert(Q38TokenFrontierControl(test==4?nullptr:&cookie,test==5?nullptr:&packet,sizeof(packet))!=B_OK
   && !copies && !scopeEntries && !context.observes && !memcmp(&packet,&before,sizeof(packet)));refusals++;
 }
 for(unsigned test=0;test<2;test++){
  Reset();P::Packet packet;const auto before=packet;
  if(test)loseAdmission=true;else admit=false;
  assert(Q38TokenFrontierControl(&cookie,&packet,sizeof(packet))==-82 && copies==1
   && scopeEntries==1 && scopeExits==1 && !locks && !context.observes && !memcmp(&packet,&before,sizeof(packet)));refusals++;
 }
 for(unsigned test=1;test<=2;test++){
  Reset();mode=test;P::Packet packet;
  assert(Q38TokenFrontierControl(&cookie,&packet,sizeof(packet))==(test==1?B_DEV_NOT_READY:B_NOT_ALLOWED)
   && P::ReplyShape(packet) && !packet.reply.ready && packet.reply.unsafe==unsigned(test==1)
   && context.observes==1 && copies==2 && !context.retains && !locks);positives++;
 }
 {
  Reset();mutateUser=true;value=17;P::Packet packet;
  assert(Q38TokenFrontierControl(&cookie,&packet,sizeof(packet))==B_OK && P::ReplyShape(packet)
   && packet.request.operation==P::Observe && packet.reply.detail[7]==17 && context.observes==1);positives++;
 }
 for(unsigned fail=1;fail<=2;fail++)for(unsigned kind=0;kind<3;kind++){
  Reset();failCopy=fail;mode=kind;P::Packet packet;const auto before=packet;
  assert(Q38TokenFrontierControl(&cookie,&packet,sizeof(packet))==-99 && copies==fail
   && !locks && context.observes==unsigned(fail==2)
   && context.retains==unsigned(fail==2 && kind!=2) && !memcmp(&packet,&before,sizeof(packet)));
  copyFaults++;
 }
 for(unsigned bad=0;bad<15;bad++){
  Reset();P::Packet packet;assert(Q38TokenFrontierControl(&cookie,&packet,sizeof(packet))==B_OK);
  switch(bad){
   case 0:packet.reply.version=0;break;case 1:packet.reply.operation=0;break;case 2:packet.reply.status=-1;break;
   case 3:packet.reply.ready=2;break;case 4:packet.reply.unsafe=1;break;case 5:packet.reply.reserved[1]=1;break;
   case 6:packet.reply.detail[0]=0;break;case 7:packet.reply.detail[2]=11;break;case 8:packet.reply.detail[3]=1;break;
   case 9:packet.reply.detail[4]=62;break;case 10:packet.reply.detail[7]=35;break;case 11:packet.reply.detail[8]=8193;break;
   case 12:packet.reply.detail[9]=0;break;case 13:packet.reply.detail[10]=1;break;
   case 14:packet.request.reserved[5]=1;break;default:assert(false);
  }
  assert(!P::ReplyShape(packet));refusals++;
 }
 printf("PASS actual typed128B packet+extracted private adapter/%u DATA positives/%u request,range,scope,reply refusals/%u input+held-reply-copy faults/immutable USER snapshot/copy locked/metadata retention without subsequent observation;FD/source/observer/quarantine are MOCK,NOT genuine owner/public driver route/model/physical/clock/TPS proof\n",positives,refusals,copyFaults);
}
