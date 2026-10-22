/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_MATRIX_CONTROLLER_H
#define PIONEER_Q38_FFN_MATRIX_CONTROLLER_H
#include <stddef.h>
#include "PioneerQ38FfnBanks.h"
#include "PioneerQ38FfnMatrixImage.h"
#include "PioneerQ38FfnMatrixProof.h"
#include "PioneerQ38DispatchClose.h"
#if !defined(PIONEER_Q38_DIRECT_SRCB_QUALIFICATION) || !defined(PIONEER_Q38_SENTINEL_POLL_QUALIFICATION) \
 || defined(PIONEER_Q38_RAW_UNPACK_CACHE_QUALIFICATION) || defined(PIONEER_Q38_LOCAL_Q4_QUALIFICATION) \
 || defined(PIONEER_Q38_Q4_NIBBLE_QUALIFICATION) || defined(PIONEER_Q38_Q4_DIRECT_QUALIFICATION) \
 || defined(PIONEER_Q38_BATCH4_FETCH_QUALIFICATION) || defined(PIONEER_Q38_BATCH_RAW_QUALIFICATION) \
 || defined(PIONEER_Q38_SIGNED_MAG2_QUALIFICATION) || defined(PIONEER_Q38_PEER_PHASE_QUALIFICATION) \
 || defined(PIONEER_Q38_COMMAND_REUSE_QUALIFICATION) || defined(PIONEER_Q38_TILE_PREFETCH_QUALIFICATION) \
 || defined(PIONEER_Q38_FRONTEND_BARRIER_QUALIFICATION) || defined(PIONEER_Q38_HOST_PHASE_PROFILE)
#error FFN overlay requires the exact qualified original-layout direct/sentinel cohort,never raw-cache or mixed peer images
#endif
// OFFLINE controller ONLY. NO native adapter/driver selects this header.
// Requires exact existing qualified direct-SrcB/sentinel cohort;not a new
// peer bootstrap or permission to change images under an existing ABI.
// IO.FfnMatrixScope is CPU-only proof of the SAME retained global context,
// lock/cookie/thread/driver/model identity+ALL writer exclusions+host owners.
// IO.FfnMatrixInputReady is CPU-only proof of an independently COMPLETED
// exact producer/source lease for this owner/layer/token/role. NEVER derive
// it from caller booleans,a raw GPU tag,initialization count or this request.
// These native contracts are intentionally UNIMPLEMENTED;ordinary builds
// stay blocked. Retain the complete context on any observed uncertainty.
class PioneerQ38FfnMatrixLease {
public:
 const PioneerQ38FfnBanks* owner=nullptr;
 const PioneerResidentBanks* banks=nullptr;
 const tt_dram_samples_result* proof=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;
 PioneerQ38FfnIdentity identity={};
 bool saved=false,dirty=false,ready=false,running=false,unsafe=false,closed=false;
 uint32_t savedPrefix[4096]={},savedMain[2048]={},savedFrame[64]={},savedRows[119]={};
 uint32_t source[120][16]={};
 PioneerQ38FfnMatrixAuthority authority;
 PioneerQ38FfnMatrixLease()=default;
 PioneerQ38FfnMatrixLease(const PioneerQ38FfnMatrixLease&)=delete;
 PioneerQ38FfnMatrixLease& operator=(const PioneerQ38FfnMatrixLease&)=delete;
}; // HEAP ONLY:never put this complete owner/saved authority on kernel stack
struct PioneerQ38FfnMatrixResult {
 uint32_t state=0,started=0,completed=0,held=0,nonce=0,recovery_required=0;
 uint32_t marker=0,phase=0,count=0,pc[2]={},before[8]={},after[8]={},stats[64]={};
};
static inline bool PioneerQ38FfnMatrixBound(const PioneerQ38FfnMatrixLease& l,const tt_dram_samples_result& d,
 const PioneerResidentBanks& b,const PioneerQ38FfnBanks& s,const PioneerQ38DispatchLease& c)
{return l.owner==&s && l.banks==&b && l.proof==&d && l.cohort==&c;}
static inline bool PioneerQ38FfnMatrixNonces(const PioneerQ38DispatchLease& c,const PioneerQ38FfnMatrixAuthority& a)
{
 // Reserve ONE further root/peer nonce for the parent's explicit STOP.
 if(c.rootNonce>=0x7ffffffd || !a.header[5] || a.header[5]>119 || a.header[6]!=c.rootNonce+1)return false;
 for(unsigned w=0;w<a.header[5];w++)if(c.peerNonce[w]>=0x7ffffffd || a.task[w][3]!=c.peerNonce[w]+1)return false;
 return true;
}
template<class IO> bool PioneerQ38FfnMatrixFail(IO& io,PioneerResidentBanks& banks,PioneerQ38FfnBanks& state,
 PioneerQ38DispatchLease& cohort,PioneerQ38FfnMatrixLease& lease)
{
 lease.unsafe=true;state.Fault();PioneerQ38DispatchQuarantine(cohort,banks);
 io.QuarantineHostPage();io.FfnMatrixRetainHostOwners();
 return false; // MEMORY ONLY:NO card reads/hold/STOP/END/ARC/restore/free
}
template<class IO> bool PioneerQ38FfnMatrixScope(IO& io,const tt_dram_samples_result& dram,const PioneerResidentBanks& banks,
 const PioneerQ38FfnBanks& state,const PioneerQ38DispatchLease& cohort,const PioneerQ38FfnMatrixLease& lease)
{
 if(!PioneerQ38FfnMatrixBound(lease,dram,banks,state,cohort) || lease.unsafe || lease.closed
  || !state.OwnsMatrixLease(&lease) || !state.MatchesInitializationOwner(banks,dram,lease.identity)
  || !state.InitializationPublished(lease.identity) || cohort.unsafe || cohort.closed || !cohort.booted || cohort.prepared!=120
  || cohort.rootRunning!=lease.running
  || !io.FfnMatrixScope(lease,state,cohort,lease.identity))return false;
 for(unsigned w=0;w<120;w++) {
  const auto& n=cohort.worker[w];const auto& store=n.master?*n.master:n.store;
  if(n.image.unsafe || !n.image.ready || !n.image.saved || n.image.running!=bool(w || lease.running)
   || n.noc.recovery_required || store.pending || store.recovery_required || !store.ready)return false;
  for(unsigned i=0;i<16;i++)if(cohort.authority.source[w][i]!=lease.source[w][i])return false;
 }
 return true; // CPU-only;does NOT select a core or read an MMIO register
}
template<class IO> bool PioneerQ38FfnMatrixSources(IO& io,const tt_dram_samples_result& dram,const PioneerResidentBanks& banks,
 const PioneerQ38FfnBanks& state,PioneerQ38DispatchLease& cohort,const PioneerQ38FfnMatrixLease& lease,uint32_t reset)
{
 return PioneerQ38FfnMatrixScope(io,dram,banks,state,cohort,lease)
  && io.SelectRuntimeCore(0) && io.FfnMatrixPrefixStable()
  && PioneerQ38DispatchSources(io,dram,banks,cohort.worker[0],reset);
}
template<class IO> bool PioneerQ38FfnMatrixCodeMatches(IO& io)
{
 if(!io.FfnMatrixPrefixStable() || !io.Q38PanelMappingStable() || io.RuntimeCodeRead(0)!=TT_Q38_DISPATCH_BOOT)return false;
 for(unsigned i=0;i<4096;i++)if(io.FfnMatrixPrefixRead(i)!=PioneerQ38FfnMatrixCodeWord(true,i))return false;
 for(unsigned i=0;i<2048;i++)if(io.Q38PanelRead(0x72000+i*4)!=PioneerQ38FfnMatrixCodeWord(false,i))return false;
 return true;
}
template<class IO> bool PioneerQ38FfnMatrixPrepare(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& banks,
 PioneerQ38FfnBanks& state,PioneerQ38DispatchLease& cohort,const PioneerQ38FfnIdentity& id,PioneerQ38FfnMatrixLease& lease)
{
 if(lease.owner || lease.banks || lease.proof || lease.cohort || !(lease.identity==PioneerQ38FfnIdentity{}) || lease.saved || lease.dirty
  || lease.ready || lease.running || lease.unsafe || lease.closed || !state.MatchesInitializationOwner(banks,dram,id)
  || !state.InitializationPublished(id) || PioneerQ38DispatchUnsafe(cohort) || !cohort.booted || cohort.closed
  || cohort.prepared!=120 || !io.FfnMatrixScope(lease,state,cohort,id))return false;
 if(!state.ClaimMatrix(id,&lease))return false;
 lease.owner=&state;lease.banks=&banks;lease.proof=&dram;lease.cohort=&cohort;lease.identity=id;
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)lease.source[w][i]=cohort.authority.source[w][i];
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
 cohort.inputReuse.valid=false; // no host reuse hint survives this different root
#endif
 auto fail=[&](){return PioneerQ38FfnMatrixFail(io,banks,state,cohort,lease);};
 // Audit the complete existing cohort BEFORE the first image write. No
 // worker reset/NoC command writer/boot sequence is added by this owner.
 for(unsigned w=0;w<120;w++) {
  if(!PioneerQ38FfnMatrixScope(io,dram,banks,state,cohort,lease) || !io.SelectRuntimeCore(w)
   || !PioneerQ38DispatchSources(io,dram,banks,cohort.worker[w],w?0x46800:0x47800)
   || !PioneerQ38DispatchCodeMatches(io,w) || (w && !PioneerQ38DispatchPeerStatus(io,cohort.peerNonce[w-1],false)))return fail();
 }
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800))return fail();
 for(unsigned i=0;i<4096;i++)lease.savedPrefix[i]=io.FfnMatrixPrefixRead(i);
 for(unsigned i=0;i<2048;i++)lease.savedMain[i]=io.Q38PanelRead(0x72000+i*4);
 for(unsigned i=0;i<64;i++)lease.savedFrame[i]=io.Q38PanelRead(TT_Q38_FFN_MATRIX_FRAME+i*4);
 for(unsigned i=0;i<119;i++)lease.savedRows[i]=io.Q38PanelRead(TT_Q38_FFN_MATRIX_ROWS+i*4);
 lease.saved=true;
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800))return fail();
 lease.dirty=true;
 for(unsigned i=0;i<4096;i++)io.FfnMatrixPrefixWrite(i,PioneerQ38FfnMatrixCodeWord(true,i));
 for(unsigned i=0;i<2048;i++)io.Q38PanelWrite(0x72000+i*4,PioneerQ38FfnMatrixCodeWord(false,i));
 if(!PioneerQ38FfnMatrixCodeMatches(io) || !PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800))return fail();
 lease.ready=true;return true;
}
// No I/O:derive ALL learned extents and the complete reserved workspace
// from their EXACT owners. Does not mint or accept input-producer authority.
static inline bool PioneerQ38FfnMatrixAuthorize(const tt_dram_samples_result& dram,const PioneerResidentBanks& banks,
 const PioneerQ38FfnBanks& state,const PioneerQ38DispatchLease& cohort,const PioneerQ38FfnMatrixLease& lease,
 const uint8_t* initialized,unsigned initializedPages,const PioneerQ38FfnMatrixJob& job,PioneerQ38FfnMatrixAuthority* output)
{
 if(!PioneerQ38FfnMatrixJobValid(job) || cohort.rootNonce>=0x7ffffffd)return false;
 PioneerQ38FfnExtent workspace;
 if(!state.InitializationPublished(lease.identity) || !state.InitializeSpan(lease.identity,0,0,PioneerQ38FfnBanks::Pages,workspace))return false;
 const unsigned inputs=job.role==2?17408:5120;
 for(unsigned w=0;w<job.peers;w++) {
  PioneerQ38CohortExtent extent;
  if(cohort.peerNonce[w]>=0x7ffffffd || cohort.peerNonce[w]>cohort.rootNonce
   || !PioneerQ38CohortAuthorizePart(banks,dram,initialized,initializedPages,job.type,inputs,job.part[w],w+1,extent))return false;
 }
 if(!output)return true;
 auto& a=*output;
 for(unsigned i=0;i<16;i++)a.header[i]=0;
 a.header[0]=TT_Q38_FFN_MATRIX_MARKER;a.header[1]=a.header[2]=1;a.header[3]=job.type;a.header[4]=inputs;
 a.header[5]=job.peers;a.header[6]=cohort.rootNonce+1;a.header[7]=3;a.header[8]=1;
 for(unsigned b=0;b<8;b++)a.banks[b]=b<7?dram.translated[b][0]|(dram.translated[b][1]<<6):0;
 for(unsigned w=0;w<119;w++) {
  for(unsigned i=0;i<8;i++)a.task[w][i]=0;
  a.first[w]=job.first[w];a.extent[w]={};
  if(w<job.peers) {
   if(!PioneerQ38CohortAuthorizePart(banks,dram,initialized,initializedPages,job.type,inputs,job.part[w],w+1,a.extent[w]))return false;
   a.task[w][0]=a.extent[w].address;a.task[w][1]=job.part[w].bank;a.task[w][2]=job.part[w].groups;
   a.task[w][3]=cohort.peerNonce[w]+1;a.task[w][4]=a.extent[w].route;a.task[w][5]=lease.source[w+1][0];
  }
 }
 auto* d=a.frame;for(unsigned i=0;i<64;i++)d[i]=0;
 d[0]=TT_Q38_FFN_MATRIX_MARKER;d[1]=1;d[2]=67108864;d[3]=workspace.bank;d[4]=workspace.address;d[5]=job.role;
 d[6]=a.header[6];d[7]=job.peers;d[8]=workspace.route;d[9]=129;d[10]=job.role==0?30848:job.role==1?100480:274880;
 d[11]=job.role==2?5120:17408;d[12]=d[11]*4;d[13]=TT_Q38_FFN_MATRIX_ROWS;d[14]=inputs;d[15]=job.type;
 for(unsigned i=0;i<16;i++)d[16+i]=lease.source[0][i];
 const uint64_t id[]={lease.identity.session,lease.identity.sequence,lease.identity.modelEpoch,lease.identity.driverGeneration};
 for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(id[i]);d[33+i*2]=uint32_t(id[i]>>32);}
 d[40]=uint32_t(job.position);d[41]=uint32_t(job.position>>32);d[42]=job.layer;
 return PioneerQ38FfnMatrixShape(d,a.header,a.banks,a.task,a.first);
}
template<class IO> bool PioneerQ38FfnMatrixFrameMatches(IO& io,const PioneerQ38FfnMatrixAuthority& a)
{
 if(!io.Q38PanelMappingStable())return false;
 for(unsigned i=0;i<16;i++)if(io.Q38PanelRead(0x74000+i*4)!=a.header[i])return false;
 for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74040+i*4)!=a.banks[i])return false;
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74080+(w*8+i)*4)!=a.task[w][i])return false;
 for(unsigned i=0;i<64;i++)if(io.Q38PanelRead(TT_Q38_FFN_MATRIX_FRAME+i*4)!=a.frame[i])return false;
 for(unsigned i=0;i<119;i++)if(io.Q38PanelRead(TT_Q38_FFN_MATRIX_ROWS+i*4)!=a.first[i])return false;
 return true;
}
template<class IO> bool PioneerQ38FfnMatrixRun(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& banks,
 PioneerQ38FfnBanks& state,PioneerQ38DispatchLease& cohort,PioneerQ38FfnMatrixLease& lease,
 const uint8_t* initialized,unsigned initializedPages,const PioneerQ38FfnMatrixJob& job,PioneerQ38FfnMatrixResult& r)
{
 r={};r.state=TT_RUNTIME_CONFIG_FAILED;
 if(!PioneerQ38FfnMatrixBound(lease,dram,banks,state,cohort) || !state.OwnsMatrixLease(&lease) || lease.unsafe
  || !lease.ready || lease.running || lease.closed || cohort.rootRunning || PioneerQ38DispatchUnsafe(cohort)
  || !PioneerQ38FfnMatrixAuthorize(dram,banks,state,cohort,lease,initialized,initializedPages,job,nullptr)
  || !io.FfnMatrixInputReady(lease,job))return false; // inert configuration/source refusal
 auto fail=[&](){r.state=TT_RUNTIME_COMPLETION_FAILED;r.recovery_required=1;return PioneerQ38FfnMatrixFail(io,banks,state,cohort,lease);};
 if(!PioneerQ38FfnMatrixScope(io,dram,banks,state,cohort,lease)
  || !PioneerQ38FfnMatrixAuthorize(dram,banks,state,cohort,lease,initialized,initializedPages,job,&lease.authority))return fail();
 auto& a=lease.authority;
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800) || !PioneerQ38FfnMatrixCodeMatches(io))return fail();
 // Metadata ONLY:no native payload/scale/output range is written or read
 // by this CPU controller. Device owns all numerical copies and arithmetic.
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<8;i++)io.Q38PanelWrite(0x74080+(w*8+i)*4,a.task[w][i]);
 for(unsigned i=0;i<64;i++)io.Q38PanelWrite(TT_Q38_FFN_MATRIX_FRAME+i*4,a.frame[i]);
 for(unsigned i=0;i<119;i++)io.Q38PanelWrite(TT_Q38_FFN_MATRIX_ROWS+i*4,a.first[i]);
 for(unsigned i=0;i<16;i++)io.Q38PanelWrite(0x74000+i*4,a.header[i]);
 if(!PioneerQ38FfnMatrixFrameMatches(io,a) || !io.FfnMatrixInputReady(lease,job))return fail();
 cohort.worker[0].Store().l1_dirty=true;cohort.worker[0].Store().l1_restored=false;
 for(unsigned i=0;i<3;i++)io.RuntimeStatusWrite(i,0);
 for(unsigned i=0;i<3;i++)if(io.RuntimeStatusRead(i))return fail();
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800))return fail();
 for(unsigned i=0;i<8;i++)r.before[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
 uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return fail();
 lease.running=cohort.rootRunning=cohort.worker[0].image.running=true;r.nonce=a.header[6];
 io.RuntimeResetWrite(0x87ffff);io.Pause();
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x87ffff))return fail();
 r.started=1;io.RuntimeResetWrite(0x46800);if(io.ResetRead()!=0x46800)return fail();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  if(!PioneerQ38FfnMatrixScope(io,dram,banks,state,cohort,lease) || !io.FfnMatrixInputReady(lease,job))return fail();
  uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return fail();
  r.marker=io.RuntimeStatusRead(2);if(r.marker && r.marker!=TT_Q38_FFN_MATRIX_MARKER)return fail();
  if(r.marker) {
   r.phase=io.RuntimeStatusRead(0);if(r.phase!=0x7200c)return fail();
   r.count=io.RuntimeStatusRead(1);if(r.count!=job.peers)return fail();
   io.Pause();r.pc[0]=io.RuntimePcRead();io.Pause();r.pc[1]=io.RuntimePcRead();
   if(!PioneerQ38RootTerminalPc(r.pc[0]) || !PioneerQ38RootTerminalPc(r.pc[1]))return fail();
   r.completed=1;break;
  }
  io.Pause();
 }
 if(!r.completed || !io.FfnMatrixInputReady(lease,job) || !PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x46800)
  || !PioneerQ38FfnMatrixCodeMatches(io) || !PioneerQ38FfnMatrixFrameMatches(io,a))return fail();
 for(unsigned i=0;i<8;i++)r.after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
 for(unsigned i=0;i<64;i++)r.stats[i]=io.Q38PanelRead(0x7f800+i*4);
 if(!PioneerQ38FfnMatrixProof(a,r.before,r.after,r.stats))return fail();
 uint32_t command[10];if(!PioneerQ38FfnMatrixLastCommand(a,command))return fail();
 for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return fail();
 if(!PioneerQ38FfnMatrixAuthorize(dram,banks,state,cohort,lease,initialized,initializedPages,job,nullptr)
  || !PioneerQ38FfnMatrixJobMatches(job,a) || !PioneerQ38FfnMatrixNonces(cohort,a)
  || !PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x46800))return fail();
 io.Pause();r.pc[0]=io.RuntimePcRead();io.Pause();r.pc[1]=io.RuntimePcRead();
 if(!PioneerQ38RootTerminalPc(r.pc[0]) || !PioneerQ38RootTerminalPc(r.pc[1]))return fail();
 // ALL physical/meta/source/producer/counter/extent/code/command proofs
 // precede the FIRST postexecution hold. No output publication or CPU read.
 io.RuntimeResetWrite(0x87ffff);io.Pause();
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x87ffff))return fail();
 io.RuntimeResetWrite(0x47800);io.Pause();
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800) || !io.FfnMatrixInputReady(lease,job)
  || !PioneerQ38FfnMatrixJobMatches(job,a) || !PioneerQ38FfnMatrixNonces(cohort,a))return fail();
 lease.running=cohort.rootRunning=cohort.worker[0].image.running=false;
 cohort.rootNonce=a.header[6];for(unsigned w=0;w<job.peers;w++)cohort.peerNonce[w]=a.task[w][3];
 r.held=1;r.state=TT_RUNTIME_PASSED;return true; // ONE projection,not FFN/model readiness
}
template<class IO> bool PioneerQ38FfnMatrixClose(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& banks,
 PioneerQ38FfnBanks& state,PioneerQ38DispatchLease& cohort,PioneerQ38FfnMatrixLease& lease,bool clean)
{
 if(lease.unsafe)return false;
 if(!PioneerQ38FfnMatrixBound(lease,dram,banks,state,cohort))return false;
 if(lease.closed)return clean && state.MatchesInitializationOwner(banks,dram,lease.identity);
 if(!state.OwnsMatrixLease(&lease))return false;
 auto fail=[&](){return PioneerQ38FfnMatrixFail(io,banks,state,cohort,lease);};
 if(!clean || lease.running || cohort.rootRunning || !lease.saved || !lease.dirty || !lease.ready
  || !PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800) || !PioneerQ38FfnMatrixCodeMatches(io))return fail();
 // No STOP/END/ARC/host release. Restore this private overlay ONLY while
 // every peer is independently at its current WAIT/source/code proof.
 for(unsigned w=1;w<120;w++)if(!PioneerQ38FfnMatrixScope(io,dram,banks,state,cohort,lease) || !io.SelectRuntimeCore(w)
  || !PioneerQ38DispatchSources(io,dram,banks,cohort.worker[w],0x46800) || !PioneerQ38DispatchCodeMatches(io,w)
  || !PioneerQ38DispatchPeerStatus(io,cohort.peerNonce[w-1],false))return fail();
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800))return fail();
 for(unsigned i=0;i<4096;i++)io.FfnMatrixPrefixWrite(i,lease.savedPrefix[i]);
 for(unsigned i=0;i<4096;i++)if(io.FfnMatrixPrefixRead(i)!=lease.savedPrefix[i])return fail();
 for(unsigned i=0;i<2048;i++)io.Q38PanelWrite(0x72000+i*4,lease.savedMain[i]);
 for(unsigned i=0;i<2048;i++)if(io.Q38PanelRead(0x72000+i*4)!=lease.savedMain[i])return fail();
 for(unsigned i=0;i<64;i++)io.Q38PanelWrite(TT_Q38_FFN_MATRIX_FRAME+i*4,lease.savedFrame[i]);
 for(unsigned i=0;i<64;i++)if(io.Q38PanelRead(TT_Q38_FFN_MATRIX_FRAME+i*4)!=lease.savedFrame[i])return fail();
 for(unsigned i=0;i<119;i++)io.Q38PanelWrite(TT_Q38_FFN_MATRIX_ROWS+i*4,lease.savedRows[i]);
 for(unsigned i=0;i<119;i++)if(io.Q38PanelRead(TT_Q38_FFN_MATRIX_ROWS+i*4)!=lease.savedRows[i])return fail();
 if(!PioneerQ38FfnMatrixSources(io,dram,banks,state,cohort,lease,0x47800) || !PioneerQ38DispatchCodeMatches(io,0)
  || !state.CloseMatrix(lease.identity,&lease))return fail();
 lease.ready=lease.dirty=false;lease.closed=true;return true; // complete parent/hosts/clock remain owned
}
#endif
