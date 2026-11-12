/* SPDX-License-Identifier: MIT */
// Actual control adapter body with mocked owner and I/O; no device/model.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <cstdio>
#define PIONEER_Q38_QGWF_RATE_NATIVE_144 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenControlPacket.h"
using status_t=int;
constexpr int B_OK=0,B_NOT_ALLOWED=-1,B_BAD_VALUE=-2,B_NO_MEMORY=-3,B_DEV_NOT_READY=-4;
namespace P=PioneerQ38TokenControl;
namespace PioneerQ38FullOffload {static bool RequestShape(const P::Packet& p){return P::RequestShape(p);}}
struct thread_info{int team=7;};struct PioneerProbeCookie{unsigned magic=0x50313030;int team=7;};
struct PioneerStoreContext{void* q38TokenNative=reinterpret_cast<void*>(1);};
struct tt_resident_session_result{};
struct PioneerQ38TokenBackendResult{uint32_t version=1,started=0,stage=0,ready=0,unsafe=0,complete=0,reserved[2]={},detail[8]={};};
struct PioneerQ38TokenTerminalResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,cancelled=0,committed=0,position=0,completedLayers=0,stage=0,firstPC=0,secondPC=0,reserved[2]={};};
struct PioneerQ38TokenFrontierResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,committed=0,limit=0,prompts=0,reserved[6]={};};
struct PioneerQ38TokenExecutionResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,cancelRequested=0,reserved[9]={};};
struct PioneerQ38TokenHoldResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,reserved[9]={};};
struct PioneerQ38TokenReadResult{uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,recordCount=0,reserved[8]={};};
static_assert(sizeof(PioneerQ38TokenBackendResult)==64 && sizeof(PioneerQ38TokenTerminalResult)==64
 && sizeof(PioneerQ38TokenExecutionResult)==64 && sizeof(PioneerQ38TokenHoldResult)==64 && sizeof(PioneerQ38TokenReadResult)==64);
static PioneerStoreContext context;static unsigned locked=0,copies=0,retains=0,terminalCalls=0,frontierCalls=0;
static bool terminalReady=false,terminalOkay=true,frontierOkay=true,frontierUnsafe=false,copyFault=false;
static unsigned frontierCount=0;
static int geteuid(){return 0;}static int find_thread(void*){return 1;}
static int get_thread_info(int,thread_info* info){info->team=7;return B_OK;}
static int user_memcpy(void* dst,const void* src,size_t bytes){copies++;if(copyFault && copies==2)return -9;std::memcpy(dst,src,bytes);return B_OK;}
class PioneerQ38TokenCallScope {public:
 explicit PioneerQ38TokenCallScope(PioneerProbeCookie*){assert(!locked);locked=1;}
 ~PioneerQ38TokenCallScope(){assert(locked);locked=0;}
 PioneerStoreContext* Context(){return &context;}int Status(){return B_NOT_ALLOWED;}
};
static void RetainQ38DispatchOwner(PioneerStoreContext&,tt_resident_session_result&){assert(locked);retains++;}
static PioneerQ38TokenBackendResult BeginQ38TokenNativeBackendScoped(PioneerQ38TokenCallScope&,unsigned,const uint32_t*,unsigned,unsigned,bool,unsigned,unsigned){return {};}
static PioneerQ38TokenBackendResult FinishQ38TokenNativeBackendScoped(PioneerQ38TokenCallScope&){return {};}
static bool ObserveQ38TokenNativeTerminal(PioneerStoreContext&,PioneerQ38TokenTerminalResult& r){
 assert(locked);terminalCalls++;r.started=1;r.phase=terminalReady?11:9;r.ready=terminalReady;r.committed=terminalReady?17:0;return terminalOkay;
}
static bool ObserveQ38TokenNativeFrontier(PioneerStoreContext&,PioneerQ38TokenFrontierResult& r){
 assert(locked);frontierCalls++;r.started=1;r.ready=frontierOkay;r.unsafe=frontierUnsafe;r.committed=frontierCount;return frontierOkay;
}
static bool CancelQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult&){return false;}
static bool HoldQ38TokenNativeStopped(PioneerStoreContext&,PioneerQ38TokenHoldResult&){return false;}
static bool ReadQ38TokenNativeRecords(PioneerStoreContext&,unsigned,uint32_t*,PioneerQ38TokenReadResult&){return false;}
#include "actual_qgwf_rate_control.inc"
static std::unique_ptr<P::Packet> Packet(){auto p=std::make_unique<P::Packet>();p->request.magic=P::Magic;p->request.version=1;p->request.operation=P::Observe;return p;}
int main(){PioneerProbeCookie cookie;
 for(unsigned ready=0;ready<2;ready++){
  auto p=Packet();terminalReady=ready;frontierCount=ready?17:3;
  assert(Q38TokenNativeControl(&cookie,p.get(),sizeof(*p))==B_OK && p->reply.ready==ready && p->reply.detail[14]==frontierCount+1 && !p->reply.unsafe);
 }
 {auto p=Packet();terminalOkay=false;unsigned before=frontierCalls;
  assert(Q38TokenNativeControl(&cookie,p.get(),sizeof(*p))==B_NOT_ALLOWED && frontierCalls==before && !p->reply.detail[14]);terminalOkay=true;}
 {auto p=Packet();frontierOkay=false;frontierUnsafe=true;
  assert(Q38TokenNativeControl(&cookie,p.get(),sizeof(*p))==B_DEV_NOT_READY && p->reply.unsafe && !p->reply.ready && !p->reply.detail[14]);}
 {auto p=Packet();frontierOkay=true;frontierUnsafe=false;copies=0;copyFault=true;unsigned before=retains;
  assert(Q38TokenNativeControl(&cookie,p.get(),sizeof(*p))==-9 && retains==before+1 && !locked);}
 assert(terminalCalls==5 && frontierCalls==4);
 puts("PASS actual rate Observe adapter: same lock, pending/terminal counts, fault and reply-copy retention; MOCK endpoints only");
}
