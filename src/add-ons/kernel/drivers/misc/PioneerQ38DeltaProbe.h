/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DELTA_PROBE_H
#define PIONEER_Q38_DELTA_PROBE_H
#include "PioneerQ38PanelProbe.h"
#include "PioneerQ38DeltaProbeProtocol.h"
#include "PioneerQ38DeltaProbeImage.h"
struct PioneerQ38DeltaProbeLease {
 bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false;
 uint32_t nonce=0;
};
template<class IO> bool PioneerQ38DeltaProbeCode(IO& io)
{
 if (io.RuntimeCodeRead(0)!=TT_Q38_DELTA_BOOT) return false;
 for (unsigned w=0;w<TT_Q38_DELTA_CODE_BYTES/4;w++)
  if (io.Q38PanelRead(TT_Q38_DELTA_CODE+w*4)!=PioneerQ38DeltaProbeWord(w)) return false;
 return true;
}
template<class IO> bool PioneerQ38DeltaProbePrepare(IO& io,const tt_dram_samples_result& dram,
 const tt_compute_noc_result& noc,PioneerResidentLease& store,const PioneerResidentBanks& banks,PioneerQ38DeltaProbeLease& d)
{
 if (d.unsafe || d.running || d.awaiting || !PioneerQ38PanelHeld(io,dram,noc,store,banks)) return false;
 if (d.ready) return PioneerQ38DeltaProbeCode(io);
 uint32_t* saved=io.Q38PanelSavedWords();
 if (!saved || d.saved) return false;
 for (unsigned i=0;i<3;i++) { uint32_t v=io.RuntimeControlRead(i); if (i?v!=0:(v&1)!=0) return false; }
 saved[0]=io.RuntimeCodeRead(0);
 for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) saved[w+1]=io.Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+w*4);
 d.saved=true;
 if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) { d.unsafe=true; store.recovery_required=true; return false; }
 d.dirty=true;
 for (unsigned w=0;w<TT_Q38_DELTA_CODE_BYTES/4;w++) io.Q38PanelWrite(TT_Q38_DELTA_CODE+w*4,PioneerQ38DeltaProbeWord(w));
 io.RuntimeCodeWrite(0,TT_Q38_DELTA_BOOT);
 d.ready=PioneerQ38DeltaProbeCode(io);
 if (!d.ready) { d.unsafe=true; store.recovery_required=true; }
 return d.ready;
}
template<class IO> bool PioneerQ38DeltaProbeInputs(IO& io,const tt_q38_delta_probe& job,bool write)
{
 const uint32_t descriptor[]={0x56383351,128,64,0},gates[]={job.decay,job.beta};
 const uint32_t* data[]={descriptor,job.prior,job.query,job.key,job.value,gates};
 const unsigned at[]={0x74000,0x90000,0x8e000,0x8e200,0x8e400,0x8e500},count[]={4,8192,128,128,64,2};
 for (unsigned part=0;part<6;part++) {
  if (write) for (unsigned w=0;w<count[part];w++) io.Q38PanelWrite(at[part]+w*4,data[part][w]);
  for (unsigned w=0;w<count[part];w++) if (io.Q38PanelRead(at[part]+w*4)!=data[part][w]) return false;
 }
 return true;
}
template<class IO> bool PioneerQ38DeltaProbeRun(IO& io,const tt_dram_samples_result& dram,
 const tt_compute_noc_result& noc,PioneerResidentLease& store,const PioneerResidentBanks& banks,
 PioneerQ38DeltaProbeLease& d,tt_q38_delta_probe& job)
{
 auto& r=job.result; r={}; r.version=TT_Q38_DELTA_PROBE_VERSION; r.state=TT_RUNTIME_CONFIG_FAILED;
 for (auto& v:job.next) v=0;
 for (auto& v:job.output) v=0;
 if (!PioneerQ38DeltaProbeValid(job) || !d.ready || d.unsafe || d.running || d.awaiting || d.nonce==UINT32_MAX) return false;
 // Once a prepared owner fails ANY proof, further I/O is forbidden.
 if (!PioneerQ38PanelHeld(io,dram,noc,store,banks) || !PioneerQ38DeltaProbeCode(io)) goto uncertain;
 r.prepared=1;
 if (!PioneerQ38DeltaProbeInputs(io,job,true)) goto uncertain;
 for (unsigned w=0;w<8192;w++) io.Q38PanelWrite(0xa0000+w*4,0x7fc00000);
 for (unsigned w=0;w<64;w++) io.Q38PanelWrite(0x8e800+w*4,0x7fc00000);
 for (unsigned w=0;w<8192;w++) if (io.Q38PanelRead(0xa0000+w*4)!=0x7fc00000) goto uncertain;
 for (unsigned w=0;w<64;w++) if (io.Q38PanelRead(0x8e800+w*4)!=0x7fc00000) goto uncertain;
 store.l1_dirty=true; store.l1_restored=false;
 for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
 for (unsigned w=0;w<3;w++) if (io.RuntimeStatusRead(w)) goto uncertain;
 if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) goto uncertain;
 // Pending BEFORE the first reset write; caller must explicitly ACCEPT
 // only after debug comparison. CLOSE/END without it retains everything.
 d.awaiting=true; r.nonce=++d.nonce;
 io.RuntimeResetWrite(0x87ffff); io.Pause();
 if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerQ38PanelSources(io,dram,noc,store,banks)) goto uncertain;
 for (unsigned i=0;i<8;i++) r.before[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
 d.running=true; r.started=1; r.state=TT_RUNTIME_COMPLETION_FAILED;
 io.RuntimeResetWrite(0x46800);
 for (unsigned attempt=0;attempt<10000;attempt++) {
  r.polls++;
  if (io.ResetRead()!=0x46800) goto uncertain;
  r.phase=io.RuntimeStatusRead(0); r.count=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
  if (r.marker==0x46564451) goto uncertain;
  if (r.phase==TT_Q38_DELTA_DONE && r.count==64 && r.marker==0x31564451) {
   io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
   if (!PioneerQ38DeltaProbeTerminal(r.pc[0]) || !PioneerQ38DeltaProbeTerminal(r.pc[1])) goto uncertain;
   r.completed=1; break;
  }
  io.Pause();
 }
 if (!r.completed || !PioneerQ38PanelSources(io,dram,noc,store,banks) || !PioneerQ38DeltaProbeCode(io)
  || !PioneerQ38DeltaProbeInputs(io,job,false)) goto uncertain;
 for (unsigned i=0;i<8;i++) {
  r.after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i)); if (r.after[i]!=r.before[i]) goto uncertain;
 }
 // No numerical CPU reference in kernel. Every export finite; debug
 // caller compares ALL8256 words BEFORE sending the separate ACCEPT.
 for (unsigned w=0;w<8192;w++) {
  uint32_t bits=io.Q38PanelRead(0xa0000+w*4); if ((bits&0x7f800000)==0x7f800000) goto uncertain; job.next[w]=bits;
 }
 for (unsigned w=0;w<64;w++) {
  uint32_t bits=io.Q38PanelRead(0x8e800+w*4); if ((bits&0x7f800000)==0x7f800000) goto uncertain; job.output[w]=bits;
 }
 if (io.ResetRead()!=0x46800 || !PioneerQ38PanelSources(io,dram,noc,store,banks)
  || io.RuntimeStatusRead(0)!=TT_Q38_DELTA_DONE || io.RuntimeStatusRead(1)!=64 || io.RuntimeStatusRead(2)!=0x31564451
  || !PioneerQ38DeltaProbeTerminal(io.RuntimePcRead()) || !PioneerQ38DeltaProbeTerminal(io.RuntimePcRead())) goto uncertain;
 for (unsigned i=0;i<8;i++) if (io.ResidentNocRead(PioneerQ38PanelCounter(i))!=r.before[i]) goto uncertain;
 io.RuntimeResetWrite(0x87ffff); io.Pause();
 if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) goto uncertain;
 io.RuntimeResetWrite(0x47800); io.Pause();
 if (!PioneerQ38PanelHeld(io,dram,noc,store,banks)) goto uncertain;
 d.running=false; r.stopped=1; r.state=TT_RUNTIME_PASSED; return true;
uncertain:
 d.unsafe=true; store.recovery_required=true; r.recovery_required=1;
 for (auto& v:job.next) v=0;
 for (auto& v:job.output) v=0;
 return false; // MEMORY ONLY after uncertainty; NO hold/reset/restore
}
static inline bool PioneerQ38DeltaProbeAccept(PioneerQ38DeltaProbeLease& d,const tt_q38_delta_accept& a)
{
 if (a.version!=TT_Q38_DELTA_PROBE_VERSION || a.reserved[0] || a.reserved[1] || !a.nonce || a.nonce!=d.nonce
  || !d.awaiting || !d.ready || d.running || d.unsafe) return false;
 d.awaiting=false; return true; // CPU metadata only; no card reads
}
template<class IO> bool PioneerQ38DeltaProbeClose(IO& io,const tt_dram_samples_result& dram,
 const tt_compute_noc_result& noc,PioneerResidentLease& store,const PioneerResidentBanks& banks,
 PioneerQ38DeltaProbeLease& d,bool clean)
{
 if (!d.saved) return clean && !d.unsafe;
 if (!clean || d.unsafe || d.running || d.awaiting) return false; // ZERO I/O
 bool safe=PioneerQ38PanelHeld(io,dram,noc,store,banks);
 const uint32_t* saved=io.Q38PanelSavedWords();
 if (!safe || !saved) { d.unsafe=true; store.recovery_required=true; return false; }
 if (d.dirty) {
  for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) io.Q38PanelWrite(TT_Q38_PANEL_VIEW_BASE+w*4,saved[w+1]);
  io.RuntimeCodeWrite(0,saved[0]);
  for (unsigned w=0;w<TT_Q38_PANEL_VIEW_WORDS;w++) if (io.Q38PanelRead(TT_Q38_PANEL_VIEW_BASE+w*4)!=saved[w+1]) {
   d.unsafe=true; store.recovery_required=true; return false;
  }
  if (io.RuntimeCodeRead(0)!=saved[0]) { d.unsafe=true; store.recovery_required=true; return false; }
  d.dirty=false;
 }
 d.ready=false; return true;
}
#endif
