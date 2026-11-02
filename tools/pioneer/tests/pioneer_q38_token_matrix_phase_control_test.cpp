/* SPDX-License-Identifier: MIT */
// Actual extracted packet/adapter/metadata methods. Native authority/OS and
// all metric/STOP DATA are MOCK;not a real owner or physical timing proof.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <initializer_list>
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenMatrixPhasePacket.h"
#include "actual_matrix_result.inc"
using status_t=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_NOT_ALLOWED=-2,B_DEV_NOT_READY=-3,B_NO_MEMORY=-4};
struct Runtime {
 bool matrixSlotsInitialized=true,matrixPhaseRead[120]={},restoring=false,l1Restored=false,committed=true;
 uint32_t terminalNonces[120]={};
 bool CommittedRecordsRead()const{return committed;}
#include "actual_matrix_nonce_owner.inc"
};
struct State {
 enum {RootHeld=1,Other=2};unsigned phase=RootHeld;bool unsafe=false;Runtime* runtime=nullptr;
 bool Unsafe()const{return unsafe;}
#include "actual_matrix_nonce_state.inc"
};
struct Context {unsigned reads=0,retains=0;Runtime owner;State state;State* q38TokenNative=&state;};
struct PioneerProbeCookie {uint32_t magic=0x50313030;int team=23;Context* context=nullptr;};
struct thread_info {int team=0;};
static Context context;static PioneerProbeCookie cookie;
static unsigned uid=0,copies=0,failCopy=0,scopeEntries=0,scopeExits=0,locks=0,mode=0;
static bool failThread=false,admit=true,loseAdmission=false,mutateUser=false;
static bool failAllocation=false;
void* operator new(std::size_t bytes,const std::nothrow_t&) noexcept{
 if(failAllocation)return nullptr;
 try{return ::operator new(bytes);}catch(...){return nullptr;}
}
static unsigned TestUid(){return uid;}
static int find_thread(const char*){return 17;}
static status_t get_thread_info(int thread,thread_info* out){assert(thread==17);if(failThread)return -81;out->team=23;return B_OK;}
static status_t TestCopy(void* to,const void* from,size_t bytes){
 using P=PioneerQ38TokenMatrixPhaseControl::Packet;
 assert(bytes==sizeof(P));copies++;assert(locks==unsigned(copies==2));if(copies==failCopy)return -99;
 memcpy(to,from,bytes);
 if(copies==1 && loseAdmission)admit=false;
 if(copies==1 && mutateUser){auto* p=const_cast<P*>(static_cast<const P*>(from));p->request.worker=0;p->words[0]=UINT32_MAX;}
 return B_OK;
}
struct PioneerQ38TokenCallScope {
 PioneerProbeCookie* cookie;bool owned=false;
 explicit PioneerQ38TokenCallScope(PioneerProbeCookie* p):cookie(p){scopeEntries++;assert(!locks);if(admit){locks=1;owned=true;}}
 ~PioneerQ38TokenCallScope(){scopeExits++;if(owned){assert(locks==1);locks=0;}}
 ::Context* Context(){return owned?cookie->context:nullptr;}
 status_t Status()const{return owned?B_OK:-82;}
};
static bool ReadQ38TokenNativeMatrixPhase(Context& c,unsigned worker,uint32_t* out,PioneerQ38TokenMatrixPhaseResult& r){
 namespace Q=PioneerQ38TokenMatrixPhaseData;
 assert(locks==1 && worker && worker<120);c.reads++;for(unsigned i=0;i<Q::Words;i++)assert(!out[i]);
 if(mode==2){out[7]=0x13579bdf;return false;}
 r.started=1;r.phase=1;c.owner.matrixPhaseRead[worker]=true;
 if(mode==1){out[0]=0x13579bdf;out[15]=0xffffffff;r.unsafe=1;r.worker=43;r.word=5;return false;}
 out[0]=Q::Magic;out[1]=Q::Version;out[2]=17;out[3]=worker;out[4]=12;out[5]=17408;out[6]=3;out[7]=1;
 out[8]=7;out[9]=3;out[10]=11;out[11]=5;out[12]=13;out[13]=7;
 if(mode==3)memset(out,0,Q::Words*4);
 if(mode==4){out[7]=0;for(unsigned i=8;i<14;i++)out[i]=0;}
 if(mode==5)out[0]^=1;
 c.owner.terminalNonces[worker]=mode==6?0:mode==7?0x7fffffff:mode==8?16:19;
 r.phase=4;r.word=63;r.worker=worker;r.ready=1;
 r.metricState=Q::Classify(*reinterpret_cast<uint32_t(*)[Q::Words]>(out),worker,19);
 r.valid=r.metricState==Q::Intervals;r.nonce=out[2];r.inputs=out[5];r.groups=out[6];
 if(mode==9)r.groups^=1; // internally inconsistent MOCK native result
 return true;
}
static void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope& scope){
 assert(locks==1 && scope.Context()==&context);context.retains++;
}
#define geteuid TestUid
#define user_memcpy TestCopy
#include "actual_matrix_control.inc"
#undef user_memcpy
#undef geteuid
static void Reset(){
 assert(!locks);context={};context.state.runtime=&context.owner;context.q38TokenNative=&context.state;
 cookie={};cookie.context=&context;
 uid=copies=failCopy=scopeEntries=scopeExits=mode=0;failThread=loseAdmission=mutateUser=failAllocation=false;admit=true;
}
int main(){
 namespace P=PioneerQ38TokenMatrixPhaseControl;
 unsigned good=0,refused=0,faults=0;
 for(unsigned worker=1;worker<120;worker++)for(unsigned trial=0;trial<9;trial++){
  Reset();mode=trial;P::Packet p;p.request.worker=worker;assert(P::RequestShape(p) && !P::ReplyShape(p));
  const status_t expected=mode==1 || mode>=6?B_DEV_NOT_READY:mode==2?B_NOT_ALLOWED:B_OK;
  const bool ready=mode==0 || mode==3 || mode==4 || mode==5;
  assert(Q38TokenMatrixPhaseControl(&cookie,&p,sizeof(p))==expected && P::ReplyShape(p)
   && p.reply.ready==unsigned(ready) && p.reply.unsafe==unsigned(mode==1 || mode>=6)
   && p.reply.stopNonce==(ready?19U:0U) && copies==2 && context.reads==1
   && context.retains==unsigned(mode>=6) && scopeEntries==1 && scopeExits==1 && !locks);good++;
 }
 for(unsigned bit=0;bit<sizeof(P::Packet)*8;bit++){
  Reset();P::Packet p;p.request.worker=1;reinterpret_cast<uint8_t*>(&p)[bit/8]^=uint8_t(1U<<(bit%8));
  if(P::RequestShape(p))continue; // valid alternative worker is not malformed
  const auto before=p;
  assert(Q38TokenMatrixPhaseControl(&cookie,&p,sizeof(p))==B_BAD_VALUE && copies==1
   && !scopeEntries && !context.reads && !memcmp(&p,&before,sizeof(p)));refused++;
 }
 for(unsigned bytes=0;bytes<=193;bytes++)if(bytes!=sizeof(P::Packet)){
  Reset();P::Packet p;p.request.worker=1;assert(Q38TokenMatrixPhaseControl(&cookie,&p,bytes)==B_BAD_VALUE && !copies && !scopeEntries);refused++;
 }
 for(unsigned trial=0;trial<6;trial++){
  Reset();P::Packet p;p.request.worker=1;const auto before=p;
  if(trial==0)uid=1;if(trial==1)cookie.magic^=1;if(trial==2)cookie.team=24;if(trial==3)failThread=true;
  assert(Q38TokenMatrixPhaseControl(trial==4?nullptr:&cookie,trial==5?nullptr:&p,sizeof(p))!=B_OK
   && !copies && !scopeEntries && !context.reads && !memcmp(&p,&before,sizeof(p)));refused++;
 }
 for(bool lose:{false,true}){
  Reset();loseAdmission=lose;admit=lose;P::Packet p;p.request.worker=1;const auto before=p;
  assert(Q38TokenMatrixPhaseControl(&cookie,&p,sizeof(p))==-82 && copies==1 && scopeEntries==1 && scopeExits==1
   && !locks && !context.reads && !memcmp(&p,&before,sizeof(p)));refused++;
 }
 {Reset();failAllocation=true;P::Packet p;p.request.worker=1;const auto before=p;
  assert(Q38TokenMatrixPhaseControl(&cookie,&p,sizeof(p))==B_NO_MEMORY && !copies
   && !scopeEntries && !context.reads && !memcmp(&p,&before,sizeof(p)));refused++;}
 {Reset();mutateUser=true;P::Packet p;p.request.worker=119;assert(Q38TokenMatrixPhaseControl(&cookie,&p,sizeof(p))==B_OK
  && P::ReplyShape(p) && p.request.worker==119 && context.reads==1);good++;}
 for(unsigned fail=1;fail<=2;fail++)for(unsigned trial=0;trial<9;trial++){
  Reset();failCopy=fail;mode=trial;P::Packet p;p.request.worker=1;const auto before=p;
  assert(Q38TokenMatrixPhaseControl(&cookie,&p,sizeof(p))==-99 && copies==fail && !locks
   && context.reads==unsigned(fail==2) && context.retains==unsigned(fail==2 && trial!=2)
   && !memcmp(&p,&before,sizeof(p)));faults++;
 }
 {Reset();mode=9;P::Packet p;p.request.worker=1;const auto before=p;
  assert(Q38TokenMatrixPhaseControl(&cookie,&p,sizeof(p))==B_DEV_NOT_READY && copies==1
   && context.reads==1 && context.retains==1 && !memcmp(&p,&before,sizeof(p)) && !locks);faults++;}
 // Actual metadata methods with explicitly fabricated lifecycle fields.
 for(unsigned trial=0;trial<10;trial++){
  Reset();auto& o=context.owner;o.matrixPhaseRead[119]=true;o.terminalNonces[119]=19;
  assert(context.state.MatrixPhaseStopNonce(119)==19);
  unsigned worker=119;
  switch(trial){
   case 0:worker=0;break;case 1:worker=120;break;case 2:worker=UINT32_MAX;break;
   case 3:o.matrixSlotsInitialized=false;break;case 4:o.matrixPhaseRead[119]=false;break;
   case 5:o.restoring=true;break;case 6:o.l1Restored=true;break;case 7:o.committed=false;break;
   case 8:context.state.phase=State::Other;break;case 9:context.state.unsafe=true;break;
  }
  assert(!context.state.MatrixPhaseStopNonce(worker));refused++;
 }
 printf("PASS actual192B matrix packet+extracted adapter+2 sealed-nonce metadata methods:%u MOCK ready,invalid-data,unsafe,refused,bad-STOP positives/%u malformed request,length,scope,metadata refusals/%u request+locked reply-copy+internal inconsistency faults/immutable snapshot+same lock through copy/NO candidate-derived STOP/unsafe retains once per adapter path;MOCK native reader,lifecycle,FD,OS,quarantine,NOT genuine native owner,physical timing,model,Work or deployment proof\n",good,refused,faults);
}
