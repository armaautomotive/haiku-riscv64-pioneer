/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_SWEEP_PROTOCOL_H
#define PIONEER_DRAM_SWEEP_PROTOCOL_H
#include <stdint.h>
#define TT_PCI_PROBE_DRAM_SWEEP_SUPPORT (B_DEVICE_OP_CODES_END+0x62e0)
#define TT_PCI_PROBE_DRAM_SWEEP_PREFLIGHT (B_DEVICE_OP_CODES_END+0x62e1)
#define TT_PCI_PROBE_DRAM_SWEEP_RUN (B_DEVICE_OP_CODES_END+0x62e2)
// Fixed DATA only:no pointers, addresses, arbitrary firmware/code/reset flags.
// The inner sweep.closed word is NOT full native END or a cleanup capability.
struct tt_pioneer_dram_sweep_reply {
 uint32_t version,phase,retained,complete,busy_mhz,idle_mhz;
 uint32_t views_closed,pci_closed,reservation_closed;
 uint32_t sweep[28];
 uint32_t preflight;
};
struct tt_pioneer_dram_sweep {
 uint32_t version,bank,chunk_shift,sweeps,nonce,reserved[3];
 uint64_t expected_tag,expected_generation;
 tt_pioneer_dram_sweep_reply reply;
};
static_assert(sizeof(tt_pioneer_dram_sweep_reply)==152,"fixed38-word diagnostic DATA");
static_assert(sizeof(tt_pioneer_dram_sweep)==200,"fixed pointer-free diagnostic request");
static inline bool PioneerDramSweepParameters(const tt_pioneer_dram_sweep& r){
 if(r.version!=1 || r.bank>=7 || !r.nonce || !r.sweeps || r.sweeps>256
  || (r.chunk_shift!=6 && r.chunk_shift!=10 && r.chunk_shift!=12 && r.chunk_shift!=14)
  || r.reserved[0] || r.reserved[1] || r.reserved[2]
  || r.expected_tag!=UINT64_C(0x2026110200020076) || !r.expected_generation)return false;
 return true;
}
static inline bool PioneerDramSweepRequest(const tt_pioneer_dram_sweep& r){
 if(!PioneerDramSweepParameters(r))return false;
 const auto* words=reinterpret_cast<const uint8_t*>(&r.reply);
 for(unsigned i=0;i<sizeof(r.reply);i++)if(words[i])return false;
 return true;
}
// Independent receipt grammar,NOT native completion/cleanup authority.
static inline bool PioneerDramSweepReply(const tt_pioneer_dram_sweep& r,uint32_t op){
 if(!PioneerDramSweepParameters(r))return false;
 const auto& p=r.reply;const auto* s=p.sweep;
 if(p.version!=1 || p.retained || s[0]!=1)return false;
 if(op==TT_PCI_PROBE_DRAM_SWEEP_SUPPORT){
  if(p.phase || p.complete || p.preflight || p.busy_mhz || p.idle_mhz
   || p.views_closed || p.pci_closed || p.reservation_closed)return false;
  for(unsigned i=1;i<28;i++)if(s[i])return false;
  return true;
 }
 if(p.phase!=3 || p.views_closed!=1 || p.pci_closed!=1 || p.reservation_closed!=1)return false;
 if(op==TT_PCI_PROBE_DRAM_SWEEP_PREFLIGHT){
  if(p.preflight!=1 || p.complete || p.busy_mhz || p.idle_mhz)return false;
  for(unsigned i=1;i<28;i++)if(s[i])return false;
  return true;
 }
 if(op!=TT_PCI_PROBE_DRAM_SWEEP_RUN || p.complete!=1 || p.preflight
  || p.busy_mhz<=800 || p.busy_mhz>1400 || p.idle_mhz!=800
  || s[1]!=1 || s[2]!=10 || s[3] || s[4]!=1 || s[5]!=1 || s[6]!=1 || s[7]!=1
  || s[8]!=6490 || s[9]!=6490 || s[10]!=(1U<<r.chunk_shift)/4 || s[11]!=r.bank)return false;
 const auto* v=s+12;const uint64_t began=(uint64_t(v[9])<<32)|v[8],ended=(uint64_t(v[11])<<32)|v[10];
 const unsigned x=v[3]&63,y=v[3]>>6;
 return v[0]==0x31524244 && v[1]==1 && v[2]==r.nonce && (x==17 || x==18)
  && (y==12 || y==15 || y==18 || y==21) && v[4]==(1U<<r.chunk_shift) && v[5]==r.sweeps
  && v[6]==0x800000U*r.sweeps && v[7]==(0x800000U>>r.chunk_shift)*r.sweeps
  && ended>began && ((uint64_t(v[13])<<32)|v[12])==ended-began;
}
#endif
