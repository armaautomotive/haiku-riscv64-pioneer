/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_COHORT_PROTOCOL_H
#define PIONEER_DRAM_COHORT_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#define TT_PCI_PROBE_DRAM_COHORT_SUPPORT (B_DEVICE_OP_CODES_END+0x63c0)
#define TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT (B_DEVICE_OP_CODES_END+0x63c1)
#define TT_PCI_PROBE_DRAM_COHORT_RUN (B_DEVICE_OP_CODES_END+0x63c2)
// Fixed pointer-free DATA;no arbitrary bank/address/code/clock/reset selector.
struct tt_pioneer_dram_cohort_timing {
 uint32_t version,state;
 uint64_t start_lower_us,start_upper_us,end_lower_us,end_upper_us;
 uint64_t elapsed_min_us,elapsed_max_us,cycles,bytes;
};
struct tt_pioneer_dram_cohort_reply {
 uint32_t version,phase,retained,complete,busy_mhz,idle_mhz;
 uint32_t views_closed,pci_closed,reservation_closed,preflight;
 uint32_t sweep[268];
 tt_pioneer_dram_cohort_timing timing;
};
struct tt_pioneer_dram_cohort {
 uint32_t version,sweeps,queue_depth,nonce,reserved[4];
 uint64_t expected_tag,expected_generation;
 tt_pioneer_dram_cohort_reply reply;
};
static_assert(sizeof(tt_pioneer_dram_cohort_timing)==72,"fixed timing DATA");
static_assert(sizeof(tt_pioneer_dram_cohort_reply)==1184,"fixed native receipt,no hidden padding");
static_assert(sizeof(tt_pioneer_dram_cohort)==1232,"fixed pointer-free cohort request");
static inline bool PioneerDramCohortZero(const void* p,size_t size){
 const auto* bytes=static_cast<const uint8_t*>(p);
 for(size_t i=0;i<size;i++)if(bytes[i])return false;
 return true;
}
static inline bool PioneerDramCohortParameters(const tt_pioneer_dram_cohort& r){
 if(r.version!=1 || !r.nonce || !r.sweeps || r.sweeps>256 || !r.queue_depth || r.queue_depth>16
  || (r.queue_depth&(r.queue_depth-1)) || !PioneerDramCohortZero(r.reserved,sizeof(r.reserved))
  || r.expected_tag!=UINT64_C(0x2026110300010077) || !r.expected_generation)return false;
 return true;
}
static inline bool PioneerDramCohortRequest(const tt_pioneer_dram_cohort& r){
 return PioneerDramCohortParameters(r) && PioneerDramCohortZero(&r.reply,sizeof(r.reply));
}
// Independent receipt grammar only,NEVER native completion/recovery authority.
static inline bool PioneerDramCohortReply(const tt_pioneer_dram_cohort& r,uint32_t op){
 if(!PioneerDramCohortParameters(r))return false;
 const auto& p=r.reply;const auto* s=p.sweep;const auto& t=p.timing;
 if(p.version!=6 || p.retained || s[0]!=6 || t.version!=1)return false;
 if(op==TT_PCI_PROBE_DRAM_COHORT_SUPPORT || op==TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT){
  if(op==TT_PCI_PROBE_DRAM_COHORT_SUPPORT){
   if(p.phase || p.complete || p.preflight || p.views_closed || p.pci_closed || p.reservation_closed)return false;
  }else if(p.phase!=3 || p.preflight!=1 || p.complete || p.views_closed!=1 || p.pci_closed!=1 || p.reservation_closed!=1)return false;
  if(p.busy_mhz || p.idle_mhz || !PioneerDramCohortZero(s+1,267*4) || t.state
   || !PioneerDramCohortZero(&t.start_lower_us,64))return false;
  return true;
 }
 if(op!=TT_PCI_PROBE_DRAM_COHORT_RUN || p.phase!=3 || p.complete!=1 || p.preflight
  || p.views_closed!=1 || p.pci_closed!=1 || p.reservation_closed!=1
  || p.busy_mhz<=800 || p.busy_mhz>1400 || p.idle_mhz!=800 || s[1]!=1 || s[2]!=10 || s[3]
  || s[4]!=255 || s[5]!=255 || s[6]!=255 || s[7]!=1 || s[8]!=493457 || s[9]!=493457
  || s[10]!=7*r.queue_depth*4096 || s[267])return false;
 uint32_t routes[7]={};
 for(unsigned role=0;role<8;role++){
  const auto* v=s+11+role*32;
  const uint64_t a=(uint64_t(v[9])<<32)|v[8],b=(uint64_t(v[11])<<32)|v[10];
  const uint64_t bytes=UINT64_C(8388608)*r.sweeps*(role?1:7);
  if(v[0]!=0x36435244 || v[1]!=6 || v[2]!=role || v[3]!=r.nonce || v[4]!=r.sweeps || v[5]!=r.queue_depth
   || ((uint64_t(v[7])<<32)|v[6])!=bytes || b<=a || ((uint64_t(v[13])<<32)|v[12])!=b-a
   || v[15]!=512*r.sweeps*(role?1:7))return false;
  if(role){
   const unsigned x=v[14]&63,y=v[14]>>6;
   if((x!=17 && x!=18) || (y!=12 && y!=15 && y!=18 && y!=21))return false;
   for(unsigned k=0;k<role-1;k++)if(v[14]==routes[k])return false;
   routes[role-1]=v[14];
   if(!PioneerDramCohortZero(v+18,14*4))return false;
  }else{
   if(v[14]<28 || v[14]>21+7*65536 || v[16]!=7 || v[17]!=v[14]+14)return false;
   uint64_t last=0;
   for(unsigned k=1;k<=7;k++){
    const uint64_t release=(uint64_t(v[17+k*2])<<32)|v[16+k*2];
    if(release<=last || release>=b-a)return false;
    last=release;
   }
   if(t.cycles!=b-a || t.bytes!=bytes)return false;
  }
 }
 if(t.state==2)return !t.elapsed_min_us && !t.elapsed_max_us;
 return t.state==1 && t.start_lower_us && t.start_upper_us>=t.start_lower_us
  && t.end_lower_us>t.start_upper_us && t.end_upper_us>=t.end_lower_us
  && t.elapsed_min_us==t.end_lower_us-t.start_upper_us
  && t.elapsed_max_us==t.end_upper_us-t.start_lower_us
  && t.elapsed_max_us>=t.elapsed_min_us;
}
#endif
