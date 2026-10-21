/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_ROOT_CONTROLLER_H
#define PIONEER_Q38_RESIDENT_GDN_ROOT_CONTROLLER_H
#ifndef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
#error Separate ROOT0 readback diagnostic ONLY,not ordinary Work selection
#endif
#include "PioneerQ38StateZeroController.h"
#include "PioneerQ38ResidentGdnShape.h"
#include "PioneerQ38ResidentGdnRootImage.h"
#include "PioneerQ38ResidentGdnRootProtocol.h"
// Separate ROOT0 qualification controller,NEVER model/Work selection.
// Its gated native entry must retain context/cookie/PCI/ALL hosts/clock on
// unsafe/lost reply or unaccepted CLOSE,interlock ALL other card commands
// and require explicit known-clean END. No model capability.
struct PioneerQ38ResidentGdnRootLease {
 bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false,closed=false;
 uint32_t nonce=0,readSlot=0;
 uint64_t position=0;
 PioneerQ38GdnIdentity identity={};
 const PioneerQ38GdnBanks* owner=nullptr;
 const PioneerResidentBanks* banks=nullptr;
 const tt_dram_samples_result* proof=nullptr;
};
static inline bool PioneerQ38RgdnRootPc(uint32_t pc) { return pc==0x72038 || pc==0x7203c; }
static inline bool PioneerQ38RgdnRootBound(const PioneerQ38GdnBanks& state,const PioneerResidentBanks& banks,
 const tt_dram_samples_result& dram,const PioneerQ38ResidentGdnRootLease& lease) {
 return !lease.unsafe && !lease.closed && lease.owner==&state && lease.banks==&banks && lease.proof==&dram
  && state.MatchesInitializationOwner(banks,dram,lease.identity) && state.InitializationPublished(lease.identity);
}
template<class IO> bool PioneerQ38RgdnRootFail(IO& io,PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,
 PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
 PioneerQ38ResidentGdnRootLease& lease) {
 lease.unsafe=true; state.Fault(); banks.Quarantine(); store.recovery_required=true; bulk.unsafe=true;
 io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,transfer); PioneerQuarantineWideOutput(io,wide);
 return false; // MEMORY ONLY:NO hold/read/restore/END/reset/retry/release
}
template<class IO> bool PioneerQ38RgdnRootSources(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
 const PioneerQ38GdnBanks& state,const PioneerResidentBanks& banks,const tt_host_page_result& input,
 const PioneerResidentLease& store,const PioneerBulkUploadLease& bulk,const PioneerOutputBufferLease& transfer,
 const PioneerWideOutputLease& wide,const PioneerQ38ResidentGdnRootLease& lease,uint32_t reset) {
 if (!io.ResidentGdnBeforeRoot() || !PioneerQ38RgdnRootBound(state,banks,dram,lease)
  || !store.ready || store.pending || store.recovery_required || !bulk.saved || !bulk.ready || bulk.unsafe
  || !transfer.ready || transfer.pending || transfer.quarantined || !wide.ready || !wide.memory_type_set
  || wide.released || wide.quarantined || wide.pending) return false;
 return io.ResidentGdnOtherCoresHeld() && io.SelectRuntimeCore(0) && io.Q38PanelMappingStable() && io.BulkMappingStable()
  && io.ResetRead()==reset && io.ResetRead()==reset && PioneerRuntimeSources(io,dram,noc,store,0x402)
  && BlackholeResidentIdle(io) && BlackholeHostReadMapStable(io,input)
  && PioneerOutputBufferMapStable(io,transfer,input) && PioneerWideOutputMapStable(io,wide,input,transfer);
}
template<class IO> bool PioneerQ38RgdnRootCode(IO& io) {
 if (!io.Q38PanelMappingStable() || io.RuntimeCodeRead(0)!=TT_Q38_RGDN_ROOT_BOOT) return false;
 for (uint32_t at=0x70000;at<0x74000;at+=4)
  if (io.Q38PanelRead(at)!=(at<0x72000?0:PioneerQ38ResidentGdnRootWord((at-0x72000)/4))) return false;
 return true;
}
template<class IO> bool PioneerQ38RgdnRootPrepare(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
 PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const PioneerQ38GdnIdentity& identity,const tt_host_page_result& input,
 PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
 PioneerQ38ResidentGdnRootLease& lease) {
 if (lease.saved || lease.dirty || lease.ready || lease.running || lease.unsafe || lease.awaiting || lease.closed || lease.nonce || lease.position
  || lease.owner || lease.banks || lease.proof || lease.readSlot || lease.identity.session || lease.identity.sequence
  || lease.identity.modelEpoch || lease.identity.driverGeneration || !io.ResidentGdnBeforeRoot()
  || !state.MatchesInitializationOwner(banks,dram,identity) || !state.InitializationPublished(identity)) return false;
 uint32_t* saved=io.Q38PanelSavedWords(); if (!saved || !io.SpanViewCapacity(TT_Q38_SPAN_VIEW_WORDS+1)) return false;
 lease.identity=identity; lease.owner=&state; lease.banks=&banks; lease.proof=&dram;
 auto fail=[&]() { return PioneerQ38RgdnRootFail(io,state,banks,store,bulk,transfer,wide,lease); };
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
 saved[0]=io.RuntimeCodeRead(0);
 for (unsigned w=0;w<TT_Q38_SPAN_VIEW_WORDS;w++) saved[w+1]=io.Q38PanelRead(TT_Q38_SPAN_VIEW_BASE+w*4);
 lease.saved=true;
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
 lease.dirty=true;
 for (uint32_t at=0x70000;at<0x74000;at+=4) io.Q38PanelWrite(at,at<0x72000?0:PioneerQ38ResidentGdnRootWord((at-0x72000)/4));
 io.RuntimeCodeWrite(0,TT_Q38_RGDN_ROOT_BOOT);
 if (!PioneerQ38RgdnRootCode(io) || !PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
 lease.ready=true; return true;
}
template<class IO> bool PioneerQ38RgdnRootRun(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
 PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,
 PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
 PioneerQ38ResidentGdnRootLease& lease,tt_q38_rgdn_root_request& job) {
 auto& r=job.result; r={}; r.version=1; r.state=TT_RUNTIME_CONFIG_FAILED;
 for (auto& w:job.next) w=0;
 for (auto& w:job.output) w=0;
 if (!PioneerQ38RgdnRootValid(job) || !lease.ready || lease.unsafe || lease.running || lease.awaiting || lease.closed
  || lease.nonce>=0x7ffffffe || lease.position==UINT64_MAX || lease.readSlot>=2) return false;
 auto fail=[&]() {
  r.state=TT_RUNTIME_COMPLETION_FAILED; r.recovery_required=1; r.awaiting=0;
  for (auto& w:job.next) w=0;
  for (auto& w:job.output) w=0;
  return PioneerQ38RgdnRootFail(io,state,banks,store,bulk,transfer,wide,lease);
 };
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800) || !PioneerQ38RgdnRootCode(io)) return fail();
 PioneerQ38GdnBankExtent read,write;
 if (!state.InitializeHalf(lease.identity,0,lease.readSlot,0,read) || !state.InitializeHalf(lease.identity,0,lease.readSlot^1,0,write)
  || read.bytes!=32768 || write.bytes!=32768 || read.pages!=8 || write.pages!=8) return fail();
 uint32_t d[64]={}; d[0]=TT_Q38_RGDN_MARKER; d[1]=1; d[2]=128; d[3]=64;
 d[4]=read.bank; d[5]=read.address; d[6]=write.bank; d[7]=write.address; d[8]=lease.nonce+1; d[11]=lease.readSlot;
 d[12]=read.route; d[13]=write.route; d[14]=32768; d[15]=129;
 for (unsigned w=0;w<16;w++) d[16+w]=w?noc.source[0][w]:store.config|0x10000;
 const uint64_t epochs[]={lease.identity.session,lease.identity.sequence,lease.identity.modelEpoch,lease.identity.driverGeneration,lease.position};
 for (unsigned e=0;e<5;e++) { d[32+e*2]=uint32_t(epochs[e]); d[33+e*2]=uint32_t(epochs[e]>>32); }
 if (!PioneerQ38ResidentGdnShape(d)) return fail();
 const uint32_t local[]={0x74000,TT_Q38_RGDN_DESCRIPTOR,0x8e000,0x8e200,0x8e400,0x8e500};
 const uint32_t header[]={0x56383351,128,64,0},gates[]={job.decay,job.beta};
 const uint32_t* values[]={header,d,job.query,job.key,job.value,gates}; const unsigned words[]={4,64,128,128,64,2};
 for (unsigned part=0;part<6;part++) {
  for (unsigned w=0;w<words[part];w++) io.Q38PanelWrite(local[part]+w*4,values[part][w]);
  for (unsigned w=0;w<words[part];w++) if (io.Q38PanelRead(local[part]+w*4)!=values[part][w]) return fail();
 }
 for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
 for (unsigned w=0;w<3;w++) if (io.RuntimeStatusRead(w)) return fail();
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
 for (unsigned w=0;w<8;w++) r.before[w]=io.ResidentNocRead(PioneerQ38SpanCounter(w));
 uint64_t began=io.Q38CohortNow(); if (began>UINT64_MAX-UINT64_C(30000000)) return fail();
 lease.running=true; lease.nonce=d[8]; r.nonce=lease.nonce; r.position=lease.position; r.read_slot=lease.readSlot; r.write_slot=lease.readSlot^1;
 store.l1_dirty=true; store.l1_restored=false;
 io.RuntimeResetWrite(0x87ffff); io.Pause();
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x87ffff)) return fail();
 r.started=1; io.RuntimeResetWrite(0x46800); if (io.ResetRead()!=0x46800) return fail();
 for (unsigned attempt=0;attempt<65536;attempt++) {
  uint64_t now=io.Q38CohortNow(); if (now<began || now-began>=UINT64_C(30000000)) return fail();
  r.marker=io.RuntimeStatusRead(2); if (r.marker && r.marker!=TT_Q38_RGDN_MARKER) return fail();
  if (r.marker) {
   r.phase=io.RuntimeStatusRead(0); r.count=io.RuntimeStatusRead(1);
   if (r.phase!=0x7200c || r.count!=64) return fail();
   io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
   if (!PioneerQ38RgdnRootPc(r.pc[0]) || !PioneerQ38RgdnRootPc(r.pc[1])) return fail();
   r.completed=1; break;
  }
  io.Pause();
 }
 if (!r.completed || !PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x46800)
  || !PioneerQ38RgdnRootCode(io)) return fail();
 for (unsigned w=0;w<8;w++) r.after[w]=io.ResidentNocRead(PioneerQ38SpanCounter(w));
 for (unsigned w=0;w<32;w++) r.summary[w]=io.Q38PanelRead(TT_Q38_RGDN_SUMMARY+w*4);
 if (!PioneerQ38ResidentGdnSummary(r.summary,d,r.before,r.after)) return fail();
 for (unsigned w=0;w<64;w++) if (io.Q38PanelRead(TT_Q38_RGDN_DESCRIPTOR+w*4)!=d[w]) return fail();
 for (unsigned part=0;part<6;part++) for (unsigned w=0;w<words[part];w++)
  if (io.Q38PanelRead(local[part]+w*4)!=values[part][w]) return fail(); // frozen debug inputs through completion
 const uint32_t command[]={write.address+16384,0,write.route,0x9c000,0,129,0,0,16384,0};
 for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)!=command[w]) return fail();
 PioneerQ38GdnBankExtent confirmedRead,confirmedWrite;
 if (!state.InitializeHalf(lease.identity,0,lease.readSlot,0,confirmedRead) || !state.InitializeHalf(lease.identity,0,lease.readSlot^1,0,confirmedWrite)
  || confirmedRead.bank!=read.bank || confirmedRead.address!=read.address || confirmedRead.route!=read.route || confirmedRead.index!=read.index
  || confirmedWrite.bank!=write.bank || confirmedWrite.address!=write.address || confirmedWrite.route!=write.route || confirmedWrite.index!=write.index) return fail();
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x46800)) return fail();
 io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
 if (!PioneerQ38RgdnRootPc(r.pc[0]) || !PioneerQ38RgdnRootPc(r.pc[1])) return fail();
 // ALL independent completion proofs precede the FIRST post-run hold.
 io.RuntimeResetWrite(0x87ffff); io.Pause();
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x87ffff)) return fail();
 io.RuntimeResetWrite(0x47800); io.Pause();
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
 r.held=1;
 for (unsigned w=0;w<8192;w++) { uint32_t v=io.Q38PanelRead(0x98000+w*4); if ((v&0x7f800000)==0x7f800000) return fail(); job.next[w]=v; }
 for (unsigned w=0;w<64;w++) { uint32_t v=io.Q38PanelRead(0x8e800+w*4); if ((v&0x7f800000)==0x7f800000) return fail(); job.output[w]=v; }
 if (!PioneerQ38RgdnRootCode(io) || !PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
 for (unsigned w=0;w<8;w++) if (io.ResidentNocRead(PioneerQ38SpanCounter(w))!=r.after[w]) return fail();
 lease.running=false; lease.awaiting=true; r.awaiting=1; r.state=TT_RUNTIME_PASSED;
 return true; // slot/position NOT committed until explicit CPU-debug ACCEPT
}
static inline bool PioneerQ38RgdnRootAccept(PioneerQ38ResidentGdnRootLease& lease,const tt_q38_rgdn_root_accept& a) {
 if (a.version!=1 || a.nonce!=lease.nonce || !a.nonce || a.reserved[0] || a.reserved[1] || !lease.awaiting
  || lease.unsafe || lease.running || lease.closed || !lease.ready || lease.position==UINT64_MAX) return false;
 lease.readSlot^=1; lease.position++; lease.awaiting=false; return true; // diagnostic half0 ONLY,NOT a model-token commit
}
template<class IO> bool PioneerQ38RgdnRootClose(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
 PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const tt_host_page_result& input,PioneerResidentLease& store,
 PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
 PioneerQ38ResidentGdnRootLease& lease,bool knownClean) {
 if (lease.unsafe) return false;
 if (lease.closed) return true;
 auto fail=[&]() { return PioneerQ38RgdnRootFail(io,state,banks,store,bulk,transfer,wide,lease); };
 if (!knownClean || lease.running) return fail();
 if (lease.awaiting) return false; // unaccepted result:INERT,caller must retain on END/CLOSE
 if (!lease.saved) { if (lease.ready || lease.dirty) return fail(); lease.closed=true; return true; }
 if (!lease.ready || !PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)
  || !PioneerQ38RgdnRootCode(io)) return fail();
 const uint32_t* saved=io.Q38PanelSavedWords(); if (!saved) return fail();
 for (unsigned w=0;w<TT_Q38_SPAN_VIEW_WORDS;w++) io.Q38PanelWrite(TT_Q38_SPAN_VIEW_BASE+w*4,saved[w+1]);
 for (unsigned w=0;w<TT_Q38_SPAN_VIEW_WORDS;w++) if (io.Q38PanelRead(TT_Q38_SPAN_VIEW_BASE+w*4)!=saved[w+1]) return fail();
 io.RuntimeCodeWrite(0,saved[0]); if (io.RuntimeCodeRead(0)!=saved[0]) return fail();
 if (!PioneerQ38RgdnRootSources(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,0x47800)) return fail();
 lease.ready=lease.dirty=false; lease.closed=true; return true;
}
#endif
