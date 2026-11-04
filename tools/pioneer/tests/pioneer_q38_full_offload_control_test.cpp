/* SPDX-License-Identifier: MIT */
// Actual control/copy/admission body,MOCK owner and execution endpoints.
// This does NOT simulate numerical success or native cleanup authority.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#define B_DEVICE_OP_CODES_END 1000
#include "full_offload_role_defines.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadProtocol.h"
using status_t=int;
static constexpr status_t B_OK=0,B_NOT_ALLOWED=-5,B_BAD_VALUE=-6,B_NO_MEMORY=-7,B_DEV_NOT_READY=-8,CopyFault=-19;
struct thread_info{int team=17;};
struct PioneerProbeCookie{unsigned magic=0x50313030;int team=17;};
struct PioneerStoreContext{void* q38TokenNative=reinterpret_cast<void*>(1);};
struct tt_resident_session_result{};
struct PioneerQ38TokenBackendResult{uint32_t version=1,started=0,stage=0,ready=0,unsafe=0,complete=0,reserved[2]={},detail[8]={};};
struct PioneerQ38TokenExecutionResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,armed=0,released=0,
 acknowledged=0,cancelRequested=0,firstPC=0,secondPC=0,reserved[4]={};};
struct PioneerQ38TokenTerminalResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,cancelled=0,
 committed=0,position=0,completedLayers=0,stage=0,firstPC=0,secondPC=0,reserved[2]={};};
struct PioneerQ38TokenHoldResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,held=0,firstPC=0,secondPC=0,reserved[6]={};};
struct PioneerQ38TokenReadResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,firstRecord=0,
 recordCount=0,packets=0,bytesRead=0,drained=0,firstPC=0,secondPC=0,reserved[2]={};};
namespace P=PioneerQ38TokenControl;
static PioneerStoreContext context;
static unsigned uid=0,copies=0,locked=0,scopes=0,executions=0,retains=0;
static int copyFail=0,threadStatus=0,threadTeam=17,mode=0;
static bool admitted=true,ended=false,mutateInput=false;
static P::Packet* original=nullptr;
static unsigned geteuid(){return uid;}
static int find_thread(void*){return 1;}
static int get_thread_info(int,thread_info* t){t->team=threadTeam;return threadStatus;}
class PioneerQ38TokenCallScope {
public:
 explicit PioneerQ38TokenCallScope(PioneerProbeCookie*){assert(!locked);locked++;scopes++;}
 ~PioneerQ38TokenCallScope(){assert(locked==1);locked--;}
 PioneerStoreContext* Context(){return admitted && !ended?&context:nullptr;}
 int Status(){return B_NOT_ALLOWED;}
};
static status_t user_memcpy(void* to,const void* from,size_t bytes){
 copies++;if(int(copies)==copyFail)return CopyFault;
 std::memcpy(to,from,bytes);
 if(mutateInput && copies==1){assert(original);original->request.operation=P::Finish;original->prompt[0]=P::Vocabulary;}
 return B_OK;
}
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&){assert(locked==1 && &c==&context);retains++;}
static PioneerQ38TokenBackendResult BeginQ38TokenNativeBackendScoped(PioneerQ38TokenCallScope&,unsigned bank,
 const uint32_t* ids,unsigned prompts,unsigned limit,bool stopEos,unsigned eos0,unsigned eos1){
 assert(locked==1 && bank==0 && ids!=original->prompt && prompts==96 && limit==127 && stopEos && eos0==248044 && eos1==248046);
 for(unsigned i=0;i<prompts;i++)assert(ids[i]==i+17);
 executions++;PioneerQ38TokenBackendResult r;r.started=1;r.stage=8;r.ready=mode==0;r.unsafe=mode==2;return r;
}
static PioneerQ38TokenBackendResult FinishQ38TokenNativeBackendScoped(PioneerQ38TokenCallScope&){
 assert(locked==1);executions++;PioneerQ38TokenBackendResult r;r.started=1;r.stage=11;r.ready=r.complete=mode==0;r.unsafe=mode==2;
 if(r.complete)ended=true;return r;
}
template<class Value>static bool Result(Value& r){assert(locked==1);executions++;r.started=1;r.phase=4;r.unsafe=mode==2;return mode==0;}
static bool ObserveQ38TokenNativeTerminal(PioneerStoreContext&,PioneerQ38TokenTerminalResult& r){const bool okay=Result(r);r.ready=okay;return okay;}
static bool CancelQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult& r){const bool okay=Result(r);r.cancelRequested=okay;return okay;}
static bool HoldQ38TokenNativeStopped(PioneerStoreContext&,PioneerQ38TokenHoldResult& r){const bool okay=Result(r);r.ready=okay;r.held=120;return okay;}
static bool ReadQ38TokenNativeRecords(PioneerStoreContext&,unsigned records,uint32_t* output,PioneerQ38TokenReadResult& r){
 const bool okay=Result(r);r.ready=okay;r.recordCount=records;
 for(unsigned i=0;i<records*64;i++)output[i]=i+17;
 return okay;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeControl.inc"
static auto Fresh(unsigned action=P::Start){
 auto p=std::make_unique<P::Packet>();auto& q=p->request;q.magic=P::Magic;q.version=1;q.operation=action;
 if(action==P::Start){q.prompts=96;q.limit=127;q.stopEos=1;q.eos0=248044;q.eos1=248046;for(unsigned i=0;i<96;i++)p->prompt[i]=i+17;}
 if(action==P::Read)q.records=64;
 return p;
}
static void Reset(){uid=copies=scopes=executions=retains=0;copyFail=threadStatus=mode=0;threadTeam=17;admitted=true;ended=mutateInput=false;assert(!locked);}
int main(){
 PioneerProbeCookie cookie;unsigned refused=0;
 auto reject=[&](P::Packet& packet){Reset();original=&packet;
  assert(Q38TokenNativeControl(&cookie,&packet,sizeof(packet))==B_BAD_VALUE && copies==1 && !scopes && !executions && !retains);refused++;};
 for(unsigned word=9;word<16;word++){auto p=Fresh();reinterpret_cast<uint32_t*>(&p->request)[word]=1;reject(*p);}
 for(unsigned i=96;i<P::Context;i++){auto p=Fresh();p->prompt[i]=1;reject(*p);}
 for(unsigned i=0;i<sizeof(P::Reply);i++){auto p=Fresh();reinterpret_cast<uint8_t*>(&p->reply)[i]=1;reject(*p);}
 for(unsigned i=0;i<4096;i++){auto p=Fresh();p->records[i]=1;reject(*p);}
 for(unsigned action=P::Start;action<=P::Finish;action++)for(int failure=0;failure<3;failure++)for(int copy=0;copy<=2;copy++){
  Reset();mode=failure;copyFail=copy;auto p=Fresh(action);original=p.get();const auto request=p->request;
  const auto status=Q38TokenNativeControl(&cookie,p.get(),sizeof(*p));
  assert(status==(copy?CopyFault:mode==0?B_OK:mode==2?B_DEV_NOT_READY:B_NOT_ALLOWED));
  assert(scopes==unsigned(copy!=1) && executions==unsigned(copy!=1) && !locked);
  if(!copy){assert(!std::memcmp(&request,&p->request,sizeof(request)));
   if(action==P::Read)for(unsigned i=0;i<4096;i++)assert(p->records[i]==(mode==0?i+17:0));}
  assert(retains==unsigned(copy==2 && (mode==0 || mode==2 || action==P::Start) && !(action==P::Finish && mode==0)));
 }
 Reset();mutateInput=true;auto p=Fresh();original=p.get();assert(Q38TokenNativeControl(&cookie,p.get(),sizeof(*p))==B_OK && p->request.operation==P::Start);
 for(int fault=0;fault<7;fault++){
  Reset();p=Fresh();original=p.get();auto c=cookie;status_t expected=B_BAD_VALUE;
  if(fault==0){uid=1;expected=B_NOT_ALLOWED;}if(fault==1)c.magic=0;
  if(fault==5){threadStatus=CopyFault;expected=CopyFault;}if(fault==6){threadTeam=18;expected=B_NOT_ALLOWED;}
  assert(Q38TokenNativeControl(fault==2?nullptr:&c,fault==3?nullptr:p.get(),sizeof(*p)-(fault==4))==expected);
  assert(!copies && !scopes && !executions && !retains);
 }
 Reset();p=Fresh();original=p.get();admitted=false;assert(Q38TokenNativeControl(&cookie,p.get(),sizeof(*p))==B_NOT_ALLOWED && !executions && copies==1);
 PioneerQ38FullOffload::Support support={};Reset();assert(Q38FullOffloadSupport(&support,sizeof(support))==B_OK && PioneerQ38FullOffload::Supported(support) && !scopes && !executions);
 for(unsigned word=0;word<8;word++){
  Reset();support={};reinterpret_cast<uint32_t*>(&support)[word]=1;
  assert(Q38FullOffloadSupport(&support,sizeof(support))==B_BAD_VALUE && !scopes && !executions);refused++;
 }
 printf("PASS actual full-offload kernel control %u early shape refusals/all6 operations/copy failures/root+FD+team admission/kernel snapshot/no invalid partial readout;MOCK owner endpoints only\n",refused);
}
