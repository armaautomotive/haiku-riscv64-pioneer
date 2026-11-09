/* SPDX-License-Identifier: MIT */
// Actual control + actual context wrapper, synthetic session/IO endpoints.
#define main q38_context_unit_main
#include "pioneer_q38_qgw8_trace_context_test.cpp"
#undef main
#include <cstring>
#include <new>
using status_t=int;
enum {B_OK=0,B_NOT_ALLOWED=-1,B_BAD_VALUE=-2,B_NO_MEMORY=-3,B_DEV_NOT_READY=-4,B_COPY_ERROR=-5};
struct thread_info{int team=0;};static int copyCalls=0,failCopy=0,copyRetains=0,retains=0;
static int fake_uid(){return 0;}
#define geteuid fake_uid
static int find_thread(void*){return 1;}
static status_t get_thread_info(int,thread_info* t){t->team=3;return B_OK;}
static status_t user_memcpy(void* dst,const void* src,size_t n){if(++copyCalls==failCopy)return B_COPY_ERROR;memcpy(dst,src,n);return B_OK;}
struct PioneerProbeCookie{uint32_t magic=0x50313030;int team=3;PioneerStoreContext* context=nullptr;};
struct PioneerQ38TokenCallScope {PioneerProbeCookie* cookie;explicit PioneerQ38TokenCallScope(PioneerProbeCookie* c):cookie(c){}
 PioneerStoreContext* Context(){return cookie->context;}status_t Status(){return B_NOT_ALLOWED;}};
struct tt_resident_session_result{};
static void RetainQ38DispatchOwner(PioneerStoreContext&,tt_resident_session_result&){retains++;}
static void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope&){copyRetains++;}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8TraceControlInspection.inc"

int main(){
 namespace P=PioneerQ38Qgw8LayerTracePacket;
 Startup startup;Runtime runtime;int memory=0,participants=0;
 PioneerQ38TokenNativeState state;state.startup=&startup;state.runtime=&runtime;state.memory=&memory;state.participants=&participants;
 std::array<uint8_t,PioneerResidentBanks::Pages> initialized={};std::array<uint32_t,PioneerResidentBanks::Pages> hashes={};
 PioneerStoreContext c;c.q38TokenNative=&state;c.initialized=initialized.data();c.hashes=hashes.data();
 PioneerProbeCookie cookie;cookie.context=&c;
 for(unsigned ordinal=0;ordinal<128;ordinal++){
  P::Packet packet={};packet.request={P::Magic,P::Version,ordinal,ordinal/2,ordinal%2,ordinal%2?1024U:4096U,7,0,{}};
  assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_OK && P::ReplyShape(packet));
  assert(packet.words[packet.request.words-1]==42 && runtime.calls==ordinal+1 && !copyRetains && !retains);
 }
 assert(copyCalls==256);
 puts("PASS 128 trace requests through actual control+context wrapper with synthetic retained owner/IO; NO REAL DEVICE");
}
