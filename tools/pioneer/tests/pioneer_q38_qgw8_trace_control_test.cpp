/* SPDX-License-Identifier: MIT */
// Actual inspection control with synthetic credentials, copy, scope and IO.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <new>
#include <cstdio>
#include <initializer_list>
using status_t=int;
enum {B_OK=0,B_NOT_ALLOWED=-1,B_BAD_VALUE=-2,B_NO_MEMORY=-3,B_DEV_NOT_READY=-4,B_COPY_ERROR=-5};
struct thread_info{int team=0;};static int uid=0,threadTeam=3,copyCalls=0,failCopy=0,readCalls=0,retains=0,copyRetains=0,readMode=0;
static int fake_uid(){return uid;}
#define geteuid fake_uid
static int find_thread(void*){return 1;}
static status_t get_thread_info(int,thread_info* t){t->team=threadTeam;return B_OK;}
static status_t user_memcpy(void* dst,const void* src,size_t n){if(++copyCalls==failCopy){memcpy(dst,src,n/2);return B_COPY_ERROR;}memcpy(dst,src,n);return B_OK;}
struct PioneerProbeCookie{uint32_t magic=0x50313030;int team=3;};
struct PioneerStoreContext{};static PioneerStoreContext context;static bool contextReady=true;
struct PioneerQ38TokenCallScope {explicit PioneerQ38TokenCallScope(PioneerProbeCookie*){}
 PioneerStoreContext* Context(){return contextReady?&context:nullptr;}status_t Status(){return B_NOT_ALLOWED;}};
struct PioneerQ38TokenHiddenReadResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,part=0;
 uint32_t firstWord=0,wordCount=0,position=0,completedLayers=0,terminalKind=0,committed=0,packets=0,bytesRead=0;
};
struct tt_resident_session_result{};
static void RetainQ38DispatchOwner(PioneerStoreContext&,tt_resident_session_result&){retains++;}
static void Q38TokenControlReplyFailure(PioneerQ38TokenCallScope&){copyRetains++;}
static bool ReadQ38TokenNativeTraceChunk(PioneerStoreContext&,unsigned ordinal,uint32_t* out,unsigned words,PioneerQ38TokenHiddenReadResult& r){
 readCalls++;if(readMode==1)return false;if(readMode==2){r.started=1;r.unsafe=1;return false;}
 r.started=r.ready=1;r.phase=4;r.part=ordinal;r.firstWord=(ordinal/2)*5120+(ordinal%2)*4096;
 r.wordCount=words;r.completedLayers=64;r.terminalKind=r.committed=r.packets=1;r.bytesRead=words*4;
 if(readMode==3)r.firstWord++;out[0]=0x3f800000;return true;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8TraceControlInspection.inc"

int main(){
 namespace P=PioneerQ38Qgw8LayerTracePacket;
 PioneerProbeCookie cookie;P::Packet packet={};
 auto reset=[&](unsigned ordinal){packet={};packet.request={P::Magic,P::Version,ordinal,ordinal/2,ordinal%2,ordinal%2?1024U:4096U,1,0,{}};
  uid=0;threadTeam=3;copyCalls=failCopy=readCalls=retains=copyRetains=readMode=0;contextReady=true;};
 for(unsigned ordinal=0;ordinal<128;ordinal++){
  reset(ordinal);assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_OK);
  assert(P::ReplyShape(packet) && readCalls==1 && copyCalls==2 && !retains && !copyRetains && packet.words[0]==0x3f800000);
 }
 reset(0);uid=1;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !copyCalls);
 reset(0);threadTeam=4;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !copyCalls);
 reset(0);failCopy=1;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_COPY_ERROR && !readCalls && !retains);
 reset(0);packet.request.reserved[0]=1;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_BAD_VALUE && !readCalls);
 reset(0);contextReady=false;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !readCalls);
 reset(0);readMode=1;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !packet.reply.ready && !packet.words[0]);
 reset(0);readMode=2;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_DEV_NOT_READY && packet.reply.unsafe && !packet.words[0]);
 reset(0);failCopy=2;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_COPY_ERROR && copyRetains==1 && !retains);
 reset(0);readMode=3;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_DEV_NOT_READY && retains==1 && !copyRetains && !packet.words[0]);
 reset(0);readMode=3;failCopy=2;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_COPY_ERROR && retains==1 && !copyRetains);
 reset(0);readMode=1;failCopy=2;assert(Q38Qgw8TraceControlInspection(&cookie,&packet,sizeof(packet))==B_COPY_ERROR && !retains && !copyRetains);
 puts("PASS trace control synthetic credential/shape/copy/retention cases; NO DEVICE OR REAL USER COPY");
}
