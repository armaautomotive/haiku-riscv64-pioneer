/* SPDX-License-Identifier: MIT */
// Actual packet+adapter. FD,OS,held observer and retention are MOCK.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <array>
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPhaseProfilePacket.h"
#include "actual_phase_result.inc"
using status_t=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3,B_NO_MEMORY=-4};
struct Context {unsigned reads=0,retains=0;};
struct PioneerProbeCookie {uint32_t magic=0x50313030;int team=23;Context* context=nullptr;};
struct thread_info {int team=0;};
static Context context;static PioneerProbeCookie cookie;
static unsigned uid=0,copies=0,failCopy=0,scopeEntries=0,scopeExits=0,locks=0,mode=0;
static bool failThread=false,admit=true,loseAdmission=false,mutateUser=false;
static unsigned TestUid(){return uid;}
static int find_thread(const char*){return 17;}
static status_t get_thread_info(int thread,thread_info* out){assert(thread==17);if(failThread)return -81;out->team=23;return B_OK;}
static status_t TestCopy(void* to,const void* from,size_t bytes){
 using P=PioneerQ38TokenPhaseProfileControl::Packet;
 assert(bytes==sizeof(P));copies++;assert(locks==unsigned(copies==2));if(copies==failCopy)return -99;
 memcpy(to,from,bytes);
 if(copies==1 && loseAdmission)admit=false;
 if(copies==1 && mutateUser){auto* p=const_cast<P*>(static_cast<const P*>(from));p->request.operation=999;p->words[0]=0xffffffff;}
 return B_OK;
}
struct PioneerQ38TokenCallScope {
 PioneerProbeCookie* cookie;bool owned=false;
 explicit PioneerQ38TokenCallScope(PioneerProbeCookie* p):cookie(p){scopeEntries++;assert(!locks);if(admit){locks=1;owned=true;}}
 ~PioneerQ38TokenCallScope(){scopeExits++;if(owned){assert(locks==1);locks=0;}}
 ::Context* Context(){return owned?cookie->context:nullptr;}
 status_t Status()const{return owned?B_OK:-82;}
};
static std::array<uint32_t,60> RawMetric(){
 namespace Q=PioneerQ38TokenPhaseProfileData;
 std::array<uint32_t,60> p={};uint64_t now=UINT64_C(1)<<40;Q::Initialize(p.data(),true,now);
 auto mark=[&](unsigned next){now+=19;Q::Advance(p.data(),true,now,next);};
 for(unsigned token=0;token<34;token++){
  for(unsigned layer=0;layer<64;layer++){mark(Q::Metadata);mark(layer%4==3?Q::Dense:Q::Recurrent);}
  for(unsigned next:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(next);
 }
 mark(Q::FinalStop);Q::Advance(p.data(),true,now+19,Q::Phases,34);assert(Q::Closed(p.data()));return p;
}
static bool ReadQ38TokenNativePhaseProfile(Context& c,uint32_t* out,PioneerQ38TokenPhaseProfileResult& r){
 assert(locks==1);c.reads++;for(unsigned i=0;i<64;i++)assert(!out[i]);
 if(mode==2)return false;
 r.started=1;r.phase=1;
 if(mode==1){r.unsafe=1;r.worker=43;r.word=5;return false;}
 auto raw=RawMetric();if(mode==3)raw[1]^=1;
 for(unsigned i=0;i<60;i++)out[i]=raw[i];
 r.phase=4;r.word=59;r.ready=1;r.words=60;r.committed=34;
 r.valid=mode!=3;r.metricState=raw[3];r.metricError=raw[13];return true;
}
static void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope& scope){
 assert(locks==1 && scope.Context()==&context);context.retains++;
}
#define geteuid TestUid
#define user_memcpy TestCopy
#include "actual_phase_control.inc"
#undef user_memcpy
#undef geteuid
static void Reset(){
 assert(!locks);context={};cookie={};cookie.context=&context;
 uid=copies=failCopy=scopeEntries=scopeExits=mode=0;failThread=loseAdmission=mutateUser=false;admit=true;
}
int main(){
 namespace P=PioneerQ38TokenPhaseProfileControl;
 static_assert(TT_PCI_PROBE_Q38_TOKEN_PHASE_PROFILE_READ==B_DEVICE_OP_CODES_END+0x6609);
 unsigned good=0,refused=0,faults=0;
 for(unsigned trial=0;trial<4;trial++){
  Reset();mode=trial;P::Packet p;assert(P::RequestShape(p) && !P::ReplyShape(p));
  const status_t expected=mode==1?B_DEV_NOT_READY:mode==2?B_NOT_ALLOWED:B_OK;
  assert(Q38TokenPhaseProfileControl(&cookie,&p,sizeof(p))==expected && P::ReplyShape(p)
   && p.reply.ready==unsigned(mode==0 || mode==3) && p.reply.unsafe==unsigned(mode==1)
   && p.reply.detail[7]==unsigned(mode==0) && copies==2 && context.reads==1 && !context.retains
   && scopeEntries==1 && scopeExits==1 && !locks);good++;
 }
 for(unsigned bit=0;bit<sizeof(P::Packet)*8;bit++){
  Reset();P::Packet p;reinterpret_cast<uint8_t*>(&p)[bit/8]^=uint8_t(1U<<(bit%8));const auto before=p;
  assert(!P::RequestShape(p) && Q38TokenPhaseProfileControl(&cookie,&p,sizeof(p))==B_BAD_VALUE && copies==1
   && !scopeEntries && !context.reads && !memcmp(&p,&before,sizeof(p)));refused++;
 }
 for(unsigned bytes=0;bytes<=385;bytes++)if(bytes!=sizeof(P::Packet)){
  Reset();P::Packet p;assert(Q38TokenPhaseProfileControl(&cookie,&p,bytes)==B_BAD_VALUE && !copies && !scopeEntries);refused++;
 }
 for(unsigned trial=0;trial<6;trial++){
  Reset();P::Packet p;const auto before=p;
  if(trial==0)uid=1;if(trial==1)cookie.magic^=1;if(trial==2)cookie.team=24;if(trial==3)failThread=true;
  assert(Q38TokenPhaseProfileControl(trial==4?nullptr:&cookie,trial==5?nullptr:&p,sizeof(p))!=B_OK
   && !copies && !scopeEntries && !context.reads && !memcmp(&p,&before,sizeof(p)));refused++;
 }
 for(bool lose:{false,true}){
  Reset();loseAdmission=lose;admit=lose;P::Packet p;const auto before=p;
  assert(Q38TokenPhaseProfileControl(&cookie,&p,sizeof(p))==-82 && copies==1 && scopeEntries==1 && scopeExits==1
   && !locks && !context.reads && !memcmp(&p,&before,sizeof(p)));refused++;
 }
 {Reset();mutateUser=true;P::Packet p;assert(Q38TokenPhaseProfileControl(&cookie,&p,sizeof(p))==B_OK
  && P::ReplyShape(p) && p.request.operation==P::Read && context.reads==1);good++;}
 for(unsigned fail=1;fail<=2;fail++)for(unsigned trial=0;trial<4;trial++){
  Reset();failCopy=fail;mode=trial;P::Packet p;const auto before=p;
  assert(Q38TokenPhaseProfileControl(&cookie,&p,sizeof(p))==-99 && copies==fail && !locks
   && context.reads==unsigned(fail==2) && context.retains==unsigned(fail==2 && trial!=2)
   && !memcmp(&p,&before,sizeof(p)));faults++;
 }
 // All nonzero reply-reserved/padding and mismatched DATA are rejected.
 for(unsigned word=0;word<96;word++){
  Reset();P::Packet p;assert(Q38TokenPhaseProfileControl(&cookie,&p,sizeof(p))==B_OK);
  auto* words=reinterpret_cast<uint32_t*>(&p);words[word]^=1;
  assert(!P::ReplyShape(p));refused++;
 }
 printf("PASS actual384B packet+extracted phase adapter:%u valid+invalid-metric+unsafe+admission DATA positives/%u all3072 request-bits,length,scope,reply corruptions/%u request+locked reply-copy faults/immutable user snapshot/SAME lock through copy/retentionONLY no following IO;MOCK FD,OS,reader,quarantine,NOT real native owner or physical/Work/timing proof\n",good,refused,faults);
}
