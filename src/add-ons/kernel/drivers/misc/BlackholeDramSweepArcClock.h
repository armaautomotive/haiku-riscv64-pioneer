/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_DRAM_SWEEP_ARC_CLOCK_H
#define BLACKHOLE_DRAM_SWEEP_ARC_CLOCK_H
#include "BlackholeArcClockGuard.h"
// Separate strict transport for the read-sweep native lifetime. The ordinary
// ARC diagnostic intentionally has bounded rollback retries; do NOT inherit
// those here. Any failed observation retains immediately, without another
// queue/TLB write, firmware request, reset, disposal or reservation release.
namespace BlackholeDramSweepArc {
template<class IO> bool Fail(IO& io,tt_arc_queue_result& r){
 r.recovery_required=1;io.MemoryOnlyRetain();return false;
}
template<class IO> bool Configure(IO& io,bool apb,tt_arc_queue_result& r){
 for(unsigned w=0;w<3;w++){
  const uint32_t v=BlackholeArcQueueTlbWord(apb,w);io.TlbWrite(w,v);
  if(io.TlbRead(w)!=v)return Fail(io,r);
 }
 return true;
}
template<class IO> bool Queue(IO& io,const tt_arc_scratch_result& arc,
 const tt_arc_telemetry_result& h,tt_arc_queue_result& r,uint32_t message){
 r={};r.version=1;r.request_restored=r.tlb_restored=1;
 if((message!=0x34 && message!=0x52 && message!=0x54)
  || arc.version!=1 || arc.state!=TT_ARC_SCRATCH_READY || !arc.tlb_restored
  || h.version!=1 || h.state!=TT_ARC_TELEMETRY_OK || !h.tlb_restored
  || (h.present_mask&0x3c1)!=0x3c1 || h.values[0]!=0x12050001
  || h.values[6]!=0xf7 || h.values[7]!=0x5515 || h.values[8] || h.values[9]!=0xfff
  || !h.entry_count || h.entry_count>128
  || !BlackholeTelemetryRange(h.tags_address,8+h.entry_count*4)
  || !BlackholeTelemetryRange(h.data_address,h.entry_count*4)
  || !BlackholeArcQueueRange(arc.message_queue,16))return Fail(io,r);
 for(unsigned w=0;w<3;w++)r.tlb_before[w]=io.TlbRead(w);
 if(r.tlb_before[2]&0xffffc000)return Fail(io,r);
 for(unsigned w=0;w<3;w++)if(io.TlbRead(w)!=r.tlb_before[w])return Fail(io,r);
 r.tlb_restored=0;r.state=TT_ARC_QUERY_BAD_DESCRIPTOR;
 if(!Configure(io,false,r))return false;
 for(unsigned w=0;w<4;w++)r.descriptor[w]=io.CsmRead(arc.message_queue+w*4);
 const uint32_t all=r.descriptor[0];
 if(r.descriptor[1]!=0x404 || r.descriptor[2] || r.descriptor[3]
  || !BlackholeArcQueueRange(all,4*288)
  || BlackholeArcQueueOverlap(all,4*288,arc.message_queue,16)
  || BlackholeArcQueueOverlap(all,4*288,h.tags_address,8+h.entry_count*4)
  || BlackholeArcQueueOverlap(all,4*288,h.data_address,h.entry_count*4))return Fail(io,r);
 r.state=TT_ARC_QUERY_NOTIFY_FAILED;
 if(!Configure(io,true,r))return false;
 r.msi_status=io.MsiStatusRead();
 if((r.msi_status&0x301)!=1)return Fail(io,r);
 if(!Configure(io,false,r))return false;
 for(unsigned w=0;w<4;w++)if(io.CsmRead(arc.message_queue+w*4)!=r.descriptor[w])return Fail(io,r);
 r.base=all+3*288;r.state=TT_ARC_QUERY_BUSY;
 uint32_t current[8]={},saved[8]={};
 BlackholeArcQueueHeader(io,r.base,r.before);io.Pause();BlackholeArcQueueHeader(io,r.base,current);
 if(!BlackholeArcQueueHeaderValid(r.before) || !BlackholeArcQueueSame(current,r.before)
  || r.before[0]!=r.before[4] || r.before[1]!=r.before[5])return Fail(io,r);
 r.request_address=r.base+32+(r.before[0]%4)*32;
 r.response_address=r.base+160+(r.before[1]%4)*32;
 io.SetQueueBounds(r.base,r.request_address,r.response_address);
 for(unsigned w=0;w<8;w++)saved[w]=io.CsmRead(r.request_address+w*4);
 for(unsigned w=0;w<8;w++)if(io.CsmRead(r.request_address+w*4)!=saved[w])return Fail(io,r);
 BlackholeArcQueueHeader(io,r.base,current);
 if(!BlackholeArcQueueSame(current,r.before))return Fail(io,r);
 r.request_restored=0;r.state=TT_ARC_QUERY_STAGE_FAILED;
 for(unsigned w=0;w<8;w++){
  const uint32_t v=w?0:message;io.QueueWrite(r.request_address+w*4,v);
  if(io.CsmRead(r.request_address+w*4)!=v)return Fail(io,r);
 }
 BlackholeArcQueueHeader(io,r.base,current);
 if(!BlackholeArcQueueSame(current,r.before))return Fail(io,r);
 const uint32_t request=(r.before[0]+1)%8,response=(r.before[1]+1)%8;
 r.state=TT_ARC_QUERY_SUBMIT_FAILED;r.submitted=1;io.QueueWrite(r.base,request);
 if(io.CsmRead(r.base)!=request)return Fail(io,r);
 r.state=TT_ARC_QUERY_NOTIFY_FAILED;
 if(!Configure(io,true,r))return false;
 r.msi_status=io.MsiStatusRead();if((r.msi_status&0x301)!=1)return Fail(io,r);
 io.NotifyArc();if(!Configure(io,false,r))return false;
 r.state=TT_ARC_QUERY_TIMEOUT;
 for(unsigned poll=0;poll<2000;poll++){
  r.polls++;BlackholeArcQueueHeader(io,r.base,current);
  if(!BlackholeArcQueueHeaderValid(current) || current[0]!=request || current[1]!=r.before[1]
   || (current[4]!=r.before[4] && current[4]!=request)
   || (current[5]!=r.before[5] && current[5]!=response)
   || (current[6]!=r.before[6] && current[6]!=uint32_t(r.before[6]+1)))return Fail(io,r);
  if(current[4]==request && current[5]==response && current[6]==uint32_t(r.before[6]+1)){
   r.responded=1;break;
  }
  io.Pause();
 }
 if(!r.responded)return Fail(io,r);
 uint32_t words[8];for(unsigned w=0;w<8;w++)words[w]=io.CsmRead(r.response_address+w*4);
 r.status=words[0];r.clock_mhz=words[1];r.clock_mode=words[2];
 bool valid=!r.status && (message==0x34?
  r.clock_mhz>=200 && r.clock_mhz<=1400 && r.clock_mode>=1 && r.clock_mode<=3:
  !r.clock_mhz && !r.clock_mode);
 for(unsigned w=3;w<8;w++)valid&=!words[w];
 if(!valid){r.state=TT_ARC_QUERY_BAD_RESPONSE;return Fail(io,r);}
 BlackholeArcQueueHeader(io,r.base,r.after);
 if(!BlackholeArcQueueSame(current,r.after))return Fail(io,r);
 r.state=TT_ARC_QUERY_ACK_FAILED;io.QueueWrite(r.base+4,response);
 if(io.CsmRead(r.base+4)!=response)return Fail(io,r);
 current[1]=response;
 BlackholeArcQueueHeader(io,r.base,r.after);
 if(!BlackholeArcQueueSame(current,r.after))return Fail(io,r);
 io.Pause();BlackholeArcQueueHeader(io,r.base,r.after);
 if(!BlackholeArcQueueSame(current,r.after))return Fail(io,r);
 r.acknowledged=1;r.state=TT_ARC_QUERY_RESTORE_FAILED;
 for(unsigned w=0;w<8;w++){
  io.QueueWrite(r.request_address+w*4,saved[w]);
  if(io.CsmRead(r.request_address+w*4)!=saved[w])return Fail(io,r);
 }
 for(unsigned w=0;w<8;w++)if(io.CsmRead(r.request_address+w*4)!=saved[w])return Fail(io,r);
 BlackholeArcQueueHeader(io,r.base,current);
 if(!BlackholeArcQueueSame(current,r.after))return Fail(io,r);
 r.request_restored=1;
 for(unsigned w=0;w<3;w++){
  io.TlbWrite(w,r.tlb_before[w]);r.tlb_after[w]=io.TlbRead(w);
  if(r.tlb_after[w]!=r.tlb_before[w])return Fail(io,r);
 }
 for(unsigned w=0;w<3;w++)if(io.TlbRead(w)!=r.tlb_before[w])return Fail(io,r);
 r.tlb_restored=1;r.state=TT_ARC_QUERY_PASSED;return true;
}
template<class IO> bool Message(IO& io,const tt_arc_scratch_result& a,const tt_arc_telemetry_result& h,
 tt_arc_clock_guard_result& r,uint32_t message){
 if(r.recovery_required)return false;
 r.messages++;r.last_message=message;
 const bool okay=Queue(io,a,h,r.last,message);r.recovery_required|=r.last.recovery_required;return okay;
}
template<class IO> bool Enter(IO& io,const tt_arc_scratch_result& a,const tt_arc_telemetry_result& h,tt_arc_clock_guard_result& r){
 r={};r.version=1;r.restored=1;r.last.request_restored=r.last.tlb_restored=1;
 if(!(h.present_mask&4) || !h.values[2] || h.values[2]>70U*65536)return false;
 r.state=TT_ARC_CLOCK_QUERY_FAILED;
 if(!Message(io,a,h,r,0x34))return false;
 r.before_mhz=r.last.clock_mhz;r.before_mode=r.last.clock_mode;r.state=TT_ARC_CLOCK_REFUSED;
 if(r.before_mhz!=800 || r.before_mode!=3)return false;
 r.busy_requested=1;r.restored=0;r.state=TT_ARC_CLOCK_BUSY_FAILED;
 if(!Message(io,a,h,r,0x52))return false;
 r.busy_acknowledged=1;r.state=TT_ARC_CLOCK_NOT_SETTLED;
 for(unsigned i=0;i<16;i++){
  io.Pause();if(!Message(io,a,h,r,0x34))return false;
  r.busy_mhz=r.last.clock_mhz;r.busy_mode=r.last.clock_mode;
  if(r.busy_mode!=3)return false;
  if(r.busy_mhz>800 && r.busy_mhz<=1400){r.state=TT_ARC_CLOCK_BUSY_VERIFIED;return true;}
 }
 return false;
}
template<class IO> bool Leave(IO& io,const tt_arc_scratch_result& a,const tt_arc_telemetry_result& h,tt_arc_clock_guard_result& r){
 if(r.recovery_required || !r.busy_requested || r.state!=TT_ARC_CLOCK_BUSY_VERIFIED)return false;
 r.state=TT_ARC_CLOCK_IDLE_FAILED;if(!Message(io,a,h,r,0x54))return false;
 r.idle_acknowledged=1;r.state=TT_ARC_CLOCK_RESTORE_FAILED;
 for(unsigned i=0;i<16;i++){
  io.Pause();if(!Message(io,a,h,r,0x34))return false;
  r.after_mhz=r.last.clock_mhz;r.after_mode=r.last.clock_mode;
  if(r.after_mode!=3)return false;
  if(r.after_mhz==r.before_mhz){r.restored=1;r.state=TT_ARC_CLOCK_RESTORED;return true;}
 }
 r.recovery_required=1;return false;
}
} // namespace BlackholeDramSweepArc
#endif
