/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GRAPH_CSR_PROBE_H
#define PIONEER_Q38_GRAPH_CSR_PROBE_H
#include "PioneerQ38DispatchBootstrap.h"
// Fixed remote-CSR transport qualification on the EXISTING legacy cohort.
// No new FFN code,selector,mutable range,host DMA/reset/STOP/END/ARC/release.
// Caller MUST hold the real native context/global call lock/exclude ALL
// other writers. This template supplies NO substitute native scope grant.
// Before bootstrap ALL120 held,or after clean bootstrap ROOT held and ALL
//119 peers at their current idle WAIT. Never call during actual graph jobs.
struct PioneerQ38GraphCsrResult {uint32_t version=1,started=0,passed=0,unsafe=0,live=0,workers=0,packets=0,words=0,lastWorker=0,lastGroup=0;};
static inline uint32_t PioneerQ38GraphCsrOffset(unsigned group)
{const uint32_t at[]={0x204,0x208,0x20c,0x210,0x214,0x220,0x228};
 return group<16?BlackholeDramSourceOffset(group):group<23?at[group-16]:UINT32_MAX;}
static inline unsigned PioneerQ38GraphCsrWords(unsigned group)
{return group<23?1:0;} // NoC MMIO accesses MUST be exactly4bytes,not bursts.
template<class IO> bool PioneerQ38GraphCsrIdle(IO& io){for(unsigned i=0;i<4;i++)if(io.ResidentNocRead(0x40+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(io.ResidentNocRead(0x240+i*4)&255)return false;
 return true;}
template<class IO> bool PioneerQ38GraphCsrPciQuiet(IO& io,unsigned peer)
{
 if(io.HostPciRead(peer,4,2)!=0x402)return false;
 uint32_t cap=io.HostPciRead(peer,0x34,1);uint64_t visited=0;bool found=false;
 for(unsigned hop=0;cap && hop<48;hop++){
  if(cap<0x40 || cap>0xfc || (cap&3) || (visited&(UINT64_C(1)<<(cap/4))))return false;
  visited|=UINT64_C(1)<<(cap/4);uint32_t header=io.HostPciRead(peer,cap,2);
  if((header&255)==0x10){
   if(found || cap>0xf4)return false;
   uint32_t version=io.HostPciRead(peer,cap+2,2)&15;if(version!=1 && version!=2)return false;
   uint32_t status=io.HostPciRead(peer,cap+0xa,2);if(status>0xffff || (status&0x2f))return false;
   found=true;
  }
  cap=(header>>8)&255;
 }
 return !cap && found;
}
template<class IO> bool PioneerQ38GraphCsrGuard(IO& io,const tt_dram_samples_result& dram,const PioneerResidentBanks& banks,
 PioneerQ38DispatchLease& lease,unsigned w,bool pending=false)
{
 if(w>=120 || lease.unsafe || lease.closed || lease.rootRunning || lease.prepared!=120 || !banks.Matches(dram)
  || lease.bootAttempted!=lease.booted || (pending && w))return false;
 auto& node=lease.worker[w];const auto& store=node.Store();
 if(!node.image.saved || !node.image.ready || node.image.unsafe || node.image.running!=bool(w && lease.booted)
  || !store.ready || store.recovery_required || store.pending!=pending || node.noc.recovery_required || !io.SelectRuntimeCore(w)
  || !io.Q38PanelMappingStable())return false;
 unsigned reset=w && lease.booted?0x46800:0x47800;
 if(io.ResetRead()!=reset || io.ResetRead()!=reset)return false;
 // Fail at the FIRST bad observed field,without more MMIO/packets/selects.
 for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=lease.authority.source[w][i])return false;
 for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=dram.source[i])return false;
 for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
 return PioneerQ38GraphCsrIdle(io);
}
template<class IO> bool PioneerQ38GraphCsrCode(IO& io,unsigned w)
{
 if(io.RuntimeCodeRead(0)!=TT_Q38_DISPATCH_BOOT)return false;
 for(unsigned a=0x70000;a<0x74000;a+=4)if(!w || PioneerQ38PanelImmutable(a)) {
  uint32_t expected=a>=0x72000?PioneerQ38DispatchRoleWord(w,(a-0x72000)/4):
#ifdef PIONEER_Q38_INT_SFPU
   PioneerQ38DispatchPrefixWord(w,a);
#else
   0;
#endif
  if(io.Q38PanelRead(a)!=expected)return false;
 }
 return true;
}
template<class IO> bool PioneerQ38GraphCsrWait(IO& io,unsigned nonce)
{
 for(unsigned i=0;i<16;i++){unsigned expected=i<2?nonce:i==3?unsigned(TT_Q38_PEER_MARKER):0;
  if(io.Q38PanelRead(0x74140+i*4)!=expected)return false;}
 for(unsigned i=0;i<2;i++){unsigned pc=io.RuntimePcRead();if(pc!=TT_Q38_PEER_WAIT_LOAD && pc!=TT_Q38_PEER_WAIT_BRANCH)return false;}
 return true;
}
template<class IO> bool PioneerQ38GraphCsrProbe(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& banks,
 PioneerQ38DispatchLease& lease,PioneerQ38GraphCsrResult& r)
{
 // One private attempt. Invalid preconditions are inert;an ALREADY unsafe
 // context never gets another select/MMIO/cleanup attempt from this helper.
 if(r.started || r.passed || r.unsafe || lease.unsafe || lease.closed || lease.rootRunning || lease.prepared!=120
  || lease.bootAttempted!=lease.booted || !banks.Matches(dram))return false;
 r.started=1;r.live=lease.booted;
 auto fail=[&](){r.unsafe=1;PioneerQ38DispatchQuarantine(lease,banks);io.QuarantineHostPage();return false;};
 auto guard=[&](unsigned w,bool pending=false){return PioneerQ38GraphCsrGuard(io,dram,banks,lease,w,pending);};
 for(unsigned w=0;w<120;w++)if(!guard(w) || !PioneerQ38GraphCsrCode(io,w)
  || (w && lease.booted && !PioneerQ38GraphCsrWait(io,lease.peerNonce[w-1])))return fail();
 for(unsigned w=1;w<120;w++){
  r.lastWorker=w;uint32_t target[8];if(!guard(w))return fail();
  for(unsigned i=0;i<8;i++){
   target[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
   if(i==6 && target[2]!=target[6])return fail();
  }
  for(unsigned group=0;group<23;group++){
   r.lastGroup=group;const unsigned offset=PioneerQ38GraphCsrOffset(group),words=PioneerQ38GraphCsrWords(group),local=0x75000+(offset&31);
   if(!guard(0))return fail();
   uint32_t before[8];for(unsigned i=0;i<8;i++){
    before[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(i==6 && before[2]!=before[6])return fail();
   }
   const uint32_t cmd[]={0xffb20000+offset,0,PioneerWorkerX(w)|(PioneerWorkerY(w)<<6),local,0,129,0,0,words*4,0};
   auto& root=lease.worker[0];root.Store().pending=true;root.image.dirty=true;
   // Exact fixed READ command only;no caller offsets/routes/payloads.
   for(unsigned i=0;i<10;i++)io.ResidentNocWrite(i*4,cmd[i]);
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=cmd[i])return fail();
   io.ResidentNocWrite(0x40,1);r.packets++;
   bool done=false;
   for(unsigned poll=0;poll<65536 && !done;poll++){
    unsigned busy=io.ResidentNocRead(0x40);if(busy>1)return fail();done=!busy;
    for(unsigned i=0;i<8;i++){unsigned delta=(i==0 || i==1 || i==2 || i==3 || i==6)?1U:0U;
     uint32_t observed=io.ResidentNocRead(PioneerQ38PanelCounter(i))-before[i];
     if(observed>delta)return fail();
     done &= observed==delta;}
   }
   if(!done || !guard(0,true))return fail();
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=cmd[i])return fail();
   // Compare EACH response BEFORE another packet/target select. These
   // expected words came from physical host baselines,not peer proof DATA.
   const unsigned counter[]={4,0,1,2,3,5,7};
   for(unsigned i=0;i<words;i++){
    unsigned expected=group<16?lease.authority.source[w][group]:target[counter[group-16]];
    if(io.Q38PanelRead(local+i*4)!=expected)return fail();
    r.words++;
   }
   root.Store().pending=false;
  }
  if(!guard(w) || (lease.booted && !PioneerQ38GraphCsrWait(io,lease.peerNonce[w-1])))return fail();
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=target[i])return fail();
  r.workers++;
 }
 for(unsigned w=0;w<120;w++)if(!guard(w) || !PioneerQ38GraphCsrCode(io,w)
  || (w && lease.booted && !PioneerQ38GraphCsrWait(io,lease.peerNonce[w-1])))return fail();
 if(!guard(0))return fail();
 r.passed=1;return true;
}
#endif
