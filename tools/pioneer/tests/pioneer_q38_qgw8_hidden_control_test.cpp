/* SPDX-License-Identifier: MIT */
// Actual tag129 handler with MOCK scope/reader/user-copy, no hardware.
#define B_DEVICE_OP_CODES_END 1000
#define _KERNEL_MODE 1
#define PIONEER_Q38_QGW8_HIDDEN_NATIVE_129 1
#define PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128 1
#define PIONEER_Q38_QGW8_NATIVE_INSPECTION 1
#define PIONEER_Q38_HYBRID_FULL_NATIVE 1
#define PIONEER_Q38_FULL_OFFLOAD_NATIVE 1
#define PIONEER_Q38_TOKEN_NATIVE_CONTEXT 1
#define TT_PIONEER_DRIVER_RELOAD 1
#define PIONEER_DRIVER_BUILD_TAG 0x2026110700010129ULL
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8HiddenProtocol.h"
#include "actual_hidden_result.inc"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
using status_t=int;
static constexpr int B_OK=0,B_NOT_ALLOWED=-5,B_BAD_VALUE=-6,B_NO_MEMORY=-7,B_DEV_NOT_READY=-8,CopyFault=-19;
struct thread_info {int team=17;};struct PioneerProbeCookie {unsigned magic=0x50313030;int team=17;};
struct PioneerStoreContext {};struct tt_resident_session_result {};
static PioneerStoreContext context;
static int uid=0,threadStatus=0,threadTeam=17,scopeStatus=0,copyFail=0,readMode=0;
static bool scopeAdmitted=true;
static unsigned copies=0,scopes=0,reads=0,retains=0,locked=0;
static int find_thread(void*){return 1;}
static int get_thread_info(int,thread_info* t){t->team=threadTeam;return threadStatus;}
static int geteuid(){return uid;}
class PioneerQ38TokenCallScope {
public:
 explicit PioneerQ38TokenCallScope(PioneerProbeCookie*){assert(!locked);locked++;scopes++;}
 ~PioneerQ38TokenCallScope(){assert(locked==1);locked--;}
 PioneerStoreContext* Context(){return scopeAdmitted?&context:nullptr;}
 int Status(){return scopeStatus;}
};
static int user_memcpy(void* to,const void* from,size_t bytes){
 copies++;assert(locked==unsigned(copies==2));
 if(copyFail==3 && copies==2){std::memcpy(to,from,192+13*4);return CopyFault;}
 if(int(copies)==copyFail)return CopyFault;
 std::memcpy(to,from,bytes);return 0;
}
static void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope& scope){assert(locked==1 && scope.Context()==&context);retains++;}
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&){assert(locked==1 && &c==&context);retains++;}
static bool ReadQ38TokenNativeHiddenChunk(PioneerStoreContext& c,unsigned part,uint32_t* data,
 unsigned words,PioneerQ38TokenHiddenReadResult& r){
 assert(locked==1 && &c==&context && words==PioneerQ38Qgw8Hidden::PartWords(part));reads++;
 const PioneerQ38TokenHiddenReadResult pristine;assert(!std::memcmp(&r,&pristine,sizeof(r)));
 for(unsigned i=0;i<4096;i++)assert(data[i]==0);
 if(readMode==1)return false; // pre-IO refusal
 r.started=1;r.phase=4;r.part=part;r.firstWord=part*4096;r.wordCount=words;
 r.position=0;r.completedLayers=readMode==4?63:64;r.terminalKind=1;r.committed=1;r.packets=1;r.bytesRead=words*4;
 for(unsigned i=0;i<words;i++)data[i]=i==3?0x7fc01234U:(part*4096+i)^0x3f000000U;
 if(readMode==2 || readMode==3){r.unsafe=unsigned(readMode==3);return false;}
 r.ready=1;return true;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8HiddenControl.inc"
namespace P=PioneerQ38Qgw8Hidden;
static std::unique_ptr<P::Packet> Fresh(unsigned part=0){
 auto p=std::make_unique<P::Packet>();p->request.magic=P::Magic;p->request.version=1;
 p->request.part=part;p->request.words=P::PartWords(part);return p;
}
static void Reset(){uid=threadStatus=scopeStatus=copyFail=readMode=0;threadTeam=17;scopeAdmitted=true;
 copies=scopes=reads=retains=0;assert(!locked);}
int main(){
 PioneerProbeCookie cookie;unsigned early=0;
 for(unsigned part=0;part<2;part++){
  Reset();auto p=Fresh(part);const auto request=p->request;
  assert(Q38Qgw8HiddenControl(&cookie,p.get(),sizeof(*p))==B_OK && P::ReplyShape(*p));
  assert(!std::memcmp(&request,&p->request,sizeof(request)) && copies==2 && scopes==1 && reads==1 && !retains && !locked);
  for(unsigned i=0;i<4096;i++)assert(p->words[i]==(i<p->request.words?(i==3?0x7fc01234U:(part*4096+i)^0x3f000000U):0U));
 }
 auto reject=[&](P::Packet& p){Reset();assert(Q38Qgw8HiddenControl(&cookie,&p,sizeof(p))==B_BAD_VALUE);
  assert(copies==1 && !scopes && !reads && !retains && !locked);early++;};
 for(unsigned at=0;at<sizeof(P::Request);at++)for(unsigned bit=0;bit<8;bit++){
  auto p=Fresh();reinterpret_cast<unsigned char*>(&p->request)[at]^=1U<<bit;reject(*p);
 }
 for(unsigned at=0;at<sizeof(P::Reply);at++){auto p=Fresh();reinterpret_cast<unsigned char*>(&p->reply)[at]=1;reject(*p);}
 for(unsigned at=0;at<4096;at++){auto p=Fresh();p->words[at]=1;reject(*p);}
 for(int mode=1;mode<=4;mode++){
  Reset();readMode=mode;auto p=Fresh();const int expected=mode>=3?B_DEV_NOT_READY:B_NOT_ALLOWED;
  assert(Q38Qgw8HiddenControl(&cookie,p.get(),sizeof(*p))==expected);
  assert(copies==2 && scopes==1 && reads==1 && retains==unsigned(mode==4) && !p->reply.ready && !p->reply.words);
  for(auto word:p->words)assert(!word);
 }
 for(int mode=0;mode<=4;mode++)for(int failure:{2,3}){
  Reset();readMode=mode;copyFail=failure;auto p=Fresh();
  assert(Q38Qgw8HiddenControl(&cookie,p.get(),sizeof(*p))==CopyFault);
  assert(copies==2 && scopes==1 && reads==1 && retains==unsigned(mode==0 || mode==3 || mode==4) && !locked);
  if(failure==2)assert(P::RequestShape(*p));
  else{
   assert(p->reply.ready==unsigned(mode==0));
   for(unsigned i=0;i<4096;i++)assert(p->words[i]==(mode==0 && i<13?(i==3?0x7fc01234U:i^0x3f000000U):0U));
  }
 }
 Reset();copyFail=1;auto p=Fresh();assert(Q38Qgw8HiddenControl(&cookie,p.get(),sizeof(*p))==CopyFault);
 assert(copies==1 && !scopes && !reads && !retains);
 for(int status:{B_OK,CopyFault}){
  Reset();scopeAdmitted=false;scopeStatus=status;p=Fresh();
  assert(Q38Qgw8HiddenControl(&cookie,p.get(),sizeof(*p))==(status?status:B_NOT_ALLOWED));
  assert(copies==1 && scopes==1 && !reads && !retains && !locked && P::RequestShape(*p));
 }
 for(int fault=0;fault<7;fault++){
  Reset();p=Fresh();PioneerProbeCookie altered=cookie;int expected=B_BAD_VALUE;
  if(fault==0){uid=1;expected=B_NOT_ALLOWED;}if(fault==1)altered.magic=0;
  if(fault==5){threadStatus=CopyFault;expected=CopyFault;}if(fault==6){threadTeam=18;expected=B_NOT_ALLOWED;}
  assert(Q38Qgw8HiddenControl(fault==2?nullptr:&altered,fault==3?nullptr:p.get(),
   sizeof(*p)-(fault==4?1:0))==expected);
  assert(!copies && !scopes && !reads && !retains && !locked);
 }
 printf("PASS ACTUAL tag129 handler/2 raw parts/%u malformed requests/UID+FD+team+scope+reader+full/partial copy faults/retention; MOCK scope and reader, NO hardware\n",early);
}
