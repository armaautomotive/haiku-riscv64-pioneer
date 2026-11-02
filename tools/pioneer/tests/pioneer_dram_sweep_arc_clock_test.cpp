/* SPDX-License-Identifier: MIT */
#define PIONEER_ARC_QUEUE_MODEL_ONLY
#include "pioneer_arc_queue_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramSweepArcClock.h"
#include <cstdio>
// Firmware/KernelExport endpoint model only, NOT a physical clock result.
struct StrictQueue:MockQueue {
 bool retained=false;
 unsigned touches=0,readCount=0,corruptRead=0,quarantines=0;
 explicit StrictQueue(unsigned request=0,unsigned response=0):MockQueue(request,response){}
 void Touch(){assert(!retained && "NO endpoint IO after retention");touches++;}
 uint32_t Read(uint32_t value){readCount++;return readCount==corruptRead?value^1:value;}
 uint32_t TlbRead(unsigned w){Touch();return Read(MockQueue::TlbRead(w));}
 void TlbWrite(unsigned w,uint32_t v){Touch();MockQueue::TlbWrite(w,v);}
 uint32_t CsmRead(uint32_t a){Touch();return Read(MockQueue::CsmRead(a));}
 void SetQueueBounds(uint32_t b,uint32_t r,uint32_t s){Touch();MockQueue::SetQueueBounds(b,r,s);}
 void QueueWrite(uint32_t a,uint32_t v){Touch();MockQueue::QueueWrite(a,v);}
 uint32_t MsiStatusRead(){Touch();return Read(MockQueue::MsiStatusRead());}
 void NotifyArc(){Touch();MockQueue::NotifyArc();}
 void Pause(){Touch();MockQueue::Pause();}
 void MemoryOnlyRetain(){assert(!retained);retained=true;quarantines++;}
 bool Original(){return Restored() && tlb[0]==1 && tlb[1]==2 && tlb[2]==3;}
};
int main(){
 using namespace BlackholeDramSweepArc;
 const auto arc=MockQueue::Arc();const auto health=MockQueue::Health();
 unsigned valid=0,refused=0;
 for(unsigned req=0;req<8;req++)for(unsigned resp=0;resp<8;resp++){
  StrictQueue io(req,resp);tt_arc_queue_result r={};
  assert(Queue(io,arc,health,r,0x34) && io.Original() && !io.retained && !r.recovery_required);
  assert(r.after[6]==0 && r.clock_mhz==800 && r.clock_mode==3);valid++;
 }
 unsigned writes=0,tlbWrites=0,reads=0;
 {StrictQueue io;tt_arc_queue_result r={};assert(Queue(io,arc,health,r,0x34));
  writes=io.writes;tlbWrites=io.tlbWrites;reads=io.readCount;}
 for(unsigned drop=1;drop<=writes;drop++){
  StrictQueue io;io.dropWrite=drop;tt_arc_queue_result r={};
  assert(!Queue(io,arc,health,r,0x34) && io.retained && r.recovery_required && io.quarantines==1);
  assert(io.writes==drop);refused++;
 }
 for(unsigned drop=1;drop<=tlbWrites;drop++){
  StrictQueue io;io.dropTlb=drop;tt_arc_queue_result r={};
  if(Queue(io,arc,health,r,0x34)){assert(io.Original() && !io.retained);valid++;}
  else{assert(io.retained && r.recovery_required && io.tlbWrites==drop);refused++;}
 }
 for(unsigned fault=1;fault<=reads;fault++){
  StrictQueue io;io.corruptRead=fault;tt_arc_queue_result r={};
  const bool okay=Queue(io,arc,health,r,0x34);
  if(okay){assert(io.Original() && !io.retained);valid++;}
  else{assert(io.retained && r.recovery_required && io.quarantines==1);refused++;}
 }
 for(unsigned fault=0;fault<9;fault++){
  StrictQueue io;tt_arc_queue_result r={};auto a=arc;auto h=health;uint32_t message=0x34;
  if(fault==0)io.timeout=true;
  if(fault==1)io.badReply=true;
  if(fault==2)io.changed=true;
  if(fault==3)io.failRestore=true;
  if(fault==4)io.msiStatus=0;
  if(fault==5)io.At(0x10001004)=0x405;
  if(fault==6)io.At(0x10001000)=h.data_address;
  if(fault==7)h.values[0]=0;
  if(fault==8)message=0x33;
  assert(!Queue(io,a,h,r,message) && io.retained && r.recovery_required);
  if(fault==0)assert(r.polls==2000 && io.notifications==1 && io.writes==9);
  refused++;
 }
 {StrictQueue io;tt_arc_clock_guard_result r={};
  assert(Enter(io,arc,health,r) && r.busy_mhz==1350 && r.busy_mode==3 && !r.restored);
  assert(Leave(io,arc,health,r) && r.state==TT_ARC_CLOCK_RESTORED && r.restored && r.after_mhz==800);
  assert(r.messages==5 && io.notifications==5 && io.Original() && !io.retained);valid++;}
 // Exercise every actual write position of all five protocol transactions.
 for(unsigned drop=1;drop<=writes*5;drop++){
  StrictQueue io;io.dropWrite=drop;tt_arc_clock_guard_result r={};
  const bool entered=Enter(io,arc,health,r);
  if(entered)assert(!Leave(io,arc,health,r));
  assert(io.retained && r.recovery_required && io.writes==drop && io.quarantines==1);
  const unsigned before=io.touches;assert(!Leave(io,arc,health,r) && before==io.touches);refused++;
 }
 {StrictQueue io;tt_arc_clock_guard_result r={};io.ignoreBusy=true;
  assert(!Enter(io,arc,health,r) && r.state==TT_ARC_CLOCK_NOT_SETTLED && io.notifications==18);
  const unsigned before=io.touches;assert(!Leave(io,arc,health,r) && io.touches==before);refused++;}
 {StrictQueue io;tt_arc_clock_guard_result r={};io.ignoreIdle=true;
  assert(Enter(io,arc,health,r));assert(!Leave(io,arc,health,r) && r.recovery_required && !r.restored);refused++;}
 printf("strict ARC clock endpoint model: %u valid / %u refused; zero endpoint IO after retention; NOT hardware\n",valid,refused);
}
