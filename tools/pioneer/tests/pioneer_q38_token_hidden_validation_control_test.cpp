/* SPDX-License-Identifier: MIT */
// ACTUAL handler;FD scope,source/lifecycle/reader and user-copy endpoints MOCK.
// Does not execute model instructions or touch a device.
#define B_DEVICE_OP_CODES_END 1000
#include "hidden_validation_role_defines.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenHiddenValidationProtocol.h"
#include "actual_hidden_result.inc"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
using status_t=int;
static constexpr int B_OK=0,B_NOT_ALLOWED=-5,B_BAD_VALUE=-6,B_NO_MEMORY=-7,B_DEV_NOT_READY=-8,CopyFault=-19;
struct thread_info {int team=17;};
struct PioneerProbeCookie {unsigned magic=0x50313030;int team=17;};
struct PioneerStoreContext {};
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
static void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope& scope){
 assert(locked==1 && scope.Context()==&context);retains++;
}
static bool ReadQ38TokenNativeHiddenChunk(PioneerStoreContext& c,unsigned part,uint32_t* data,
 unsigned words,PioneerQ38TokenHiddenReadResult& r){
 assert(locked==1 && &c==&context && words==PioneerQ38TokenHiddenValidation::PartWords(part));reads++;
 const PioneerQ38TokenHiddenReadResult pristine;
 assert(!std::memcmp(&r,&pristine,sizeof(r)));
 for(unsigned i=0;i<4096;i++)assert(data[i]==0);
 if(readMode==1)return false; // pre-IO refusal
 r.started=1;r.phase=41;r.part=part;r.firstWord=part*4096;r.wordCount=words;
 r.position=0;r.completedLayers=1;r.terminalKind=2;r.committed=0;r.packets=1;r.bytesRead=words*4;
 for(unsigned i=0;i<words;i++)data[i]=(i==3?0x7fc01234U:(part*4096+i)^0x3f000000U);
 if(readMode==2 || readMode==3){r.unsafe=unsigned(readMode==3);return false;} // partial failing DATA
 r.ready=1;return true;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenHiddenValidationControl.inc"
namespace P=PioneerQ38TokenHiddenValidation;
static std::unique_ptr<P::Packet> Fresh(unsigned part=0){
 auto p=std::make_unique<P::Packet>();std::memset(p.get(),0,sizeof(*p));
 p->request.magic=P::Magic;p->request.version=P::Version;p->request.part=part;p->request.words=P::PartWords(part);
 return p;
}
static void Reset(){uid=threadStatus=scopeStatus=copyFail=readMode=0;threadTeam=17;scopeAdmitted=true;
 copies=scopes=reads=retains=0;assert(!locked);}
int main(){
 PioneerProbeCookie cookie;unsigned early=0;
 for(unsigned part=0;part<2;part++){
  Reset();auto p=Fresh(part);const auto request=p->request;
  assert(Q38TokenHiddenValidationControl(&cookie,p.get(),sizeof(*p))==B_OK);
  assert(!std::memcmp(&request,&p->request,sizeof(request)) && copies==2 && scopes==1 && reads==1 && !retains && !locked);
  const auto& r=p->reply;assert(r.version==1 && r.part==part && r.started==1 && r.ready==1 && !r.unsafe
   && r.words==P::PartWords(part) && !r.status && r.detail[0]==1 && r.detail[1]==1 && r.detail[2]==41
   && r.detail[7]==part && r.detail[8]==part*4096 && r.detail[9]==r.words && r.detail[11]==1);
  for(auto value:r.reserved)assert(!value);
  for(unsigned i=0;i<4096;i++)assert(p->words[i]==(i<r.words?(i==3?0x7fc01234U:(part*4096+i)^0x3f000000U):0U));
 }
 // Every request-header bit change except the valid part0->part1 pair fails
 // without scope/reader activity. Every reply BYTE/payload WORD must be zero.
 auto reject=[&](P::Packet& p){Reset();assert(Q38TokenHiddenValidationControl(&cookie,&p,sizeof(p))==B_BAD_VALUE);
  assert(copies==1 && !scopes && !reads && !retains && !locked);early++;};
 for(unsigned at=0;at<sizeof(P::Request);at++)for(unsigned bit=0;bit<8;bit++){
  auto p=Fresh();reinterpret_cast<unsigned char*>(&p->request)[at]^=1U<<bit;reject(*p);
 }
 for(unsigned at=0;at<sizeof(P::Reply);at++){
  auto p=Fresh();reinterpret_cast<unsigned char*>(&p->reply)[at]=1;reject(*p);
 }
 for(unsigned at=0;at<4096;at++){auto p=Fresh();p->words[at]=1;reject(*p);}
 for(unsigned at=0;at<PioneerQ38TokenControl::Context;at++){
  auto p=std::make_unique<PioneerQ38TokenControl::Packet>();std::memset(p.get(),0,sizeof(*p));
  auto& r=p->request;r.magic=PioneerQ38TokenControl::Magic;r.version=1;r.operation=PioneerQ38TokenControl::Start;
  r.prompts=2;r.limit=3;p->prompt[0]=248319;p->prompt[1]=17;
  assert(P::ControlRequestShape(*p));p->prompt[at]^=1;assert(!P::ControlRequestShape(*p));
 }
 for(int mode=1;mode<=3;mode++){
  Reset();readMode=mode;auto p=Fresh();
  assert(Q38TokenHiddenValidationControl(&cookie,p.get(),sizeof(*p))==(mode==3?B_DEV_NOT_READY:B_NOT_ALLOWED));
  assert(copies==2 && scopes==1 && reads==1 && !retains && !p->reply.ready && !p->reply.words);
  for(auto word:p->words)assert(!word);
 }
 for(int mode=0;mode<=3;mode++){
  Reset();readMode=mode;copyFail=2;auto p=Fresh();
  assert(Q38TokenHiddenValidationControl(&cookie,p.get(),sizeof(*p))==CopyFault);
  assert(copies==2 && scopes==1 && reads==1 && retains==unsigned(mode==0 || mode==3) && !locked);
  assert(P::RequestShape(*p)); // this fixture injects this fault BEFORE copying
 }
 // Real user_memcpy is not an atomic transaction: a fault may copy a prefix.
 // Such a prefix contains ONLY already-validated or cleared DATA;retain the
 // SAME ownership under the lock even when partial reply delivery occurred.
 for(int mode=0;mode<=3;mode++){
  Reset();readMode=mode;copyFail=3;auto p=Fresh();
  assert(Q38TokenHiddenValidationControl(&cookie,p.get(),sizeof(*p))==CopyFault);
  assert(copies==2 && scopes==1 && reads==1 && retains==unsigned(mode==0 || mode==3) && !locked);
  assert(p->reply.ready==unsigned(mode==0));
  for(unsigned i=0;i<4096;i++)assert(p->words[i]==(mode==0 && i<13?(i==3?0x7fc01234U:i^0x3f000000U):0U));
 }
 Reset();copyFail=1;auto p=Fresh();assert(Q38TokenHiddenValidationControl(&cookie,p.get(),sizeof(*p))==CopyFault);
 assert(copies==1 && !scopes && !reads && !retains);
 for(int status:{B_OK,CopyFault}){
  Reset();scopeAdmitted=false;scopeStatus=status;p=Fresh();
  assert(Q38TokenHiddenValidationControl(&cookie,p.get(),sizeof(*p))==(status?status:B_NOT_ALLOWED));
  assert(copies==1 && scopes==1 && !reads && !retains && !locked && P::RequestShape(*p));
 }
 for(int fault=0;fault<7;fault++){
  Reset();p=Fresh();PioneerProbeCookie altered=cookie;status_t expected=B_BAD_VALUE;
  if(fault==0){uid=1;expected=B_NOT_ALLOWED;}
  if(fault==1)altered.magic=0;
  if(fault==5){threadStatus=CopyFault;expected=CopyFault;}
  if(fault==6){threadTeam=18;expected=B_NOT_ALLOWED;}
  assert(Q38TokenHiddenValidationControl(fault==2?nullptr:&altered,fault==3?nullptr:p.get(),
   sizeof(*p)-(fault==4?1:0))==expected);
  assert(!copies && !scopes && !reads && !retains && !locked);
 }
 std::printf("PASS ACTUAL tag75 hidden handler/2 parts/raw NaN preserved/zero tail/%u early shape refusals/all8192 changed fixed-prompt words refused/UID+FD+team+scope+full+partial copy faults/failed read DATA cleared/same-lock reply+retain/no following cleanup;LIFECYCLE+READER MOCK,not genuine native/model/physical/numerical/Work/TPS proof\n",early);
}
