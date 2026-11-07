/* SPDX-License-Identifier: MIT */
// Real persistent-parent control. Token commit and terminal cohort cleanup
// are distinct events. No native launch/client capability is created here.
static unsigned Prompt(unsigned position){
 const auto* p=Owner();if(!p->state[SessionReady] || position>=p->state[PromptCount])scalar_fault_park();
 Transfer(p->state[Workspace]+T::PromptOffset+(position&~15U)*4,64,false);
 unsigned token=Words(R::Scratch)[position&15];if(token>=248320 || (!position && token!=p->state[Initial]))scalar_fault_park();
 return token;
}
static __attribute__((noinline,section(".conv.token"))) void Bootstrap(){
 auto* p=Owner();uint32_t header[128],feedback[16];
 if(p->phase!=5 || p->state[SessionReady])scalar_fault_park();
 Transfer(p->state[Workspace]+T::SessionOffset,512,false);
 for(unsigned i=0;i<128;i++)Words(T::SessionLocal)[i]=Words(R::Scratch)[i];
 const auto* session=reinterpret_cast<const uint32_t*>(T::SessionLocal);
 if(!T::Session(session,p->boot,p->state[Bank],p->state[Workspace]))scalar_fault_park();
 p->state[Limit]=session[5];p->state[PromptCount]=session[6];p->state[Initial]=session[7];
 p->state[StopEos]=session[32];p->state[Eos0]=session[33];p->state[Eos1]=session[34];p->state[SessionReady]=1;
 p->state[NextToken]=Prompt(0);
 Transfer(p->state[Workspace]+L::HeadOffset,512,false);
 for(unsigned i=0;i<128;i++)header[i]=Words(L::Head)[i]=Words(R::Scratch)[i];
 if(!H::Shape(header,p->boot,p->state[Bank],p->state[Workspace]))scalar_fault_park();
 // Bootstrap's head input is explicitly empty, not a host-computed hidden
 // vector. The genuine first embedding is decoded from learned Q4_K below.
 for(unsigned i=0;i<5120;i++)Words(R::Original)[i]=Words(R::OriginalCopy)[i]=Words(R::Input)[i]=Words(R::InputShadow)[i]=0;
 for(unsigned i=0;i<2560;i++)Words(R::Native)[i]=Words(R::NativeShadow)[i]=0;
 for(unsigned i=0;i<32;i++)Words(R::Scales)[i]=Words(R::ScaleShadow)[i]=0;
 HeadSeal(header);Feedback(header,p->state[NextToken],feedback);
 if(feedback[2]!=p->state[NextToken] || feedback[3] || feedback[14] || feedback[15]!=1)scalar_fault_park();
 FeedbackSeal(header);
}
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
// Only this distinct timing BUILD relocates the unchanged small helper to
// text, retaining the exact existing conv ROM extent despite timing calls.
static __attribute__((noinline,section(".text.q38_phase_state_seal")))
#else
static __attribute__((noinline,section(".conv.token")))
#endif
void StateSeal(const uint32_t* h,const uint32_t* data,unsigned layer,bool feedback){
 HeadSeal(h);
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE
 (void)feedback; // immutable numeric scans bracket the ENTIRE transaction
#else
 InputSeal();if(feedback)FeedbackSeal(h);
#endif
 if(!S::Shape(data,layer,Owner()->state[Bank],Owner()->state[Workspace]))scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(S::Local)[i]!=data[i])scalar_fault_park();
}
static __attribute__((noinline,section(".conv.token"))) void CommitState(const uint32_t* h,bool feedback){
 auto* p=Owner();if(p->phase!=4 || p->state[Completed]!=64 || p->state[Committed]!=p->state[Position]
  || p->state[Slot]!=(p->state[Position]&1))scalar_fault_park();
#ifdef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE
 // Copy packets use only disjoint Scratch/Weight/descriptor buffers. Keep
 // EVERY metadata/source/counter/command/readback check inside the loop.
 // Detect numeric damage before any copy and again BEFORE frontier flip.
 HeadSeal(h);InputSeal();if(feedback)FeedbackSeal(h);
#endif
 unsigned completed=0;
 for(unsigned layer=0;layer<64;layer++){
  if(L::Dense(layer))continue;
  unsigned package=0;if(!L::Address(p->state[Workspace],layer,package))scalar_fault_park();
  Transfer(package+S::Offset,256,false);uint32_t data[64];
  for(unsigned i=0;i<64;i++)data[i]=Words(S::Local)[i]=Words(R::Scratch)[i];
  StateSeal(h,data,layer,feedback);
  for(unsigned span=0;span<7;span++){
   const auto* d=data+8+span*8;
   for(unsigned at=0;at<d[5];at+=16384){
    unsigned bytes=d[5]-at;if(bytes>16384)bytes=16384;
    StateSeal(h,data,layer,feedback);
    Packet(h,PioneerQ38ProjectionRoute(p->state[Bank]),data[5]+d[2]*12+at,bytes,false);
    for(unsigned i=0;i<bytes/4;i++)Words(R::Weight)[i]=Words(R::Scratch)[i];
    Packet(h,PioneerQ38ProjectionRoute(d[0]),d[1]+at,bytes,true);
    Packet(h,PioneerQ38ProjectionRoute(d[0]),d[1]+at,bytes,false);
    for(unsigned i=0;i<bytes/4;i++)if(Words(R::Weight)[i]!=Words(R::Scratch)[i])scalar_fault_park();
    StateSeal(h,data,layer,feedback);
   }
  }
  completed++;
 }
 if(completed!=48)scalar_fault_park();
 HeadSeal(h);InputSeal();if(feedback)FeedbackSeal(h);
 // Reached ONLY after all64 bodies,973 actual outputs and all48 history
 // copies/readbacks. Inactive GDN slots were written by the real bodies;
 // dense bodies appended current KV rows. One private frontier/slot flip.
 // A fault partway through copying poisons the context; NEVER rollback/retry.
 p->state[Slot]^=1;p->state[Committed]++;p->phase=6;HeadSeal(h);
}
#ifdef PIONEER_Q38_TOKEN_FRONTIER_IMAGE
// One aligned L1 telemetry word in the ALREADY zeroed progress array.
// Called ONLY after the complete existing publication/readback/seal below.
// No CSR,NoC packet,token-record ABI,worker state or lifecycle grant changes.
static __attribute__((noinline,section(".text.q38_token_frontier"))) void PublishFrontier(unsigned committed){
 *Words(L::Progress+3*4)=committed;Fence();
}
#endif
static __attribute__((noinline,section(".conv.token"))) void PublishToken(const uint32_t* h,unsigned bestRow,unsigned bestBits,bool feedback){
 auto* p=Owner();if(p->phase!=6 || p->state[Committed]!=p->state[Position]+1)scalar_fault_park();
 uint32_t record[64];for(unsigned i=0;i<64;i++)record[i]=0;
 const unsigned head[]={0,1,p->state[Position],p->state[Token],bestRow,bestBits,p->state[Committed],p->state[Slot],
  unsigned(p->state[Position]+1<p->state[PromptCount]),unsigned(feedback),64,973};
 for(unsigned i=0;i<12;i++)record[i]=head[i];
 for(unsigned i=0;i<8;i++){record[12+i]=p->boot[8+i];record[36+i]=p->state[Counts+i];}
 for(unsigned i=0;i<16;i++)record[20+i]=p->boot[16+i];
 record[44]=p->state[Committed];record[45]=48;record[46]=119;
 unsigned address=p->state[Workspace]+T::OutputOffset+p->state[Position]*256;
 HeadSeal(h);InputSeal();if(feedback)FeedbackSeal(h);
 for(unsigned i=0;i<64;i++)Words(R::Scratch)[i]=record[i];
 Packet(h,PioneerQ38ProjectionRoute(p->state[Bank]),address,256,true);
 Packet(h,PioneerQ38ProjectionRoute(p->state[Bank]),address,256,false);
 for(unsigned i=0;i<64;i++)if(Words(R::Scratch)[i]!=record[i])scalar_fault_park();
 record[0]=T::TokenMagic;*Words(R::Scratch)=record[0];
 Packet(h,PioneerQ38ProjectionRoute(p->state[Bank]),address,4,true);
 Packet(h,PioneerQ38ProjectionRoute(p->state[Bank]),address,256,false);
 for(unsigned i=0;i<64;i++)if(Words(R::Scratch)[i]!=record[i])scalar_fault_park();
 HeadSeal(h);InputSeal();if(feedback)FeedbackSeal(h);
 // This is a MODEL token/state event, NOT permission to release workers,
 // memory or clocks. Only the separate real cohort STOP closes the session.
#ifdef PIONEER_Q38_TOKEN_FRONTIER_IMAGE
 PublishFrontier(p->state[Committed]); // observation DATA only,never terminal/HOLD/END authority
#endif
}
static __attribute__((noreturn,noinline,section(".conv.token"))) void Finish(const uint32_t* h,unsigned reason){
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
 q38_token_phase_mark(QPhase::FinalStop);
#endif
 auto* p=Owner();if(p->phase!=6 || !reason || reason>2)scalar_fault_park();
 for(unsigned w=0;w<119;w++){
  unsigned n=Words(R::Nonces)[w];if(n>=0x7ffffffe)scalar_fault_park();
  uint32_t j[64];for(unsigned i=0;i<64;i++)j[i]=0;
  const unsigned head[]={G::Magic,n+1,G::Stop,w+1,G::Peer(w+1),64,p->state[Position],9999};
  for(unsigned i=0;i<8;i++){j[i]=head[i];j[8+i]=p->boot[8+i];}
  if(!G::JobShape(j,PeerBoot(w),n,0))scalar_fault_park();
  for(unsigned i=0;i<64;i++)Words(R::PeerJobs+w*256)[i]=j[i];
  Dispatch(h,w);
 }
 for(unsigned w=0;w<119;w++)Release(h,w);
 for(unsigned w=0;w<119;w++)Closed(h,w,true);
 HeadSeal(h);InputSeal();
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
 // Telemetry closes AFTER119 real STOPs, BEFORE the unchanged terminal
 // publication. This is not permission to HOLD,restore,release or succeed.
 q38_token_phase_complete(p->state[Committed]);
#endif
 for(unsigned i=0;i<64;i++)Words(T::TerminalLocal)[i]=0;
 const unsigned terminal[]={0,1,reason,p->state[Committed],p->state[Position],119,64,973,p->state[Slot],1};
 for(unsigned i=0;i<10;i++)Words(T::TerminalLocal)[i]=terminal[i];
 for(unsigned i=0;i<8;i++){Words(T::TerminalLocal)[12+i]=p->boot[8+i];Words(T::TerminalLocal)[36+i]=p->state[Counts+i];}
 for(unsigned i=0;i<16;i++)Words(T::TerminalLocal)[20+i]=p->boot[16+i];
 Fence();Words(T::TerminalLocal)[0]=T::TerminalMagic;Fence();q38_global_chain_stopped();
}
#ifdef PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE
#if PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE != 1 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || (!defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) && !defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU)) || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE)
#error Full-card EOG firmware requires its separate exact pushed or BF16 ROOT image build
#endif
#include "PioneerQ38FullOffloadEog.h"
// Text has space;the legacy auxiliary ROM has only48 bytes spare. Keep the
// extra model-specific EOG comparisons out of that almost-full ROM.
static __attribute__((noinline,section(".text.q38_full_offload_eog"))) bool FullOffloadEog(unsigned id){
 return PioneerQ38FullOffloadEog::Contains(id);
}
#endif
static __attribute__((noinline,section(".conv.token"))) unsigned RunHead(){
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
 q38_token_phase_mark(QPhase::HeadNormalize);
#endif
 uint32_t header[128];auto* p=Owner();
 Transfer(p->state[Workspace]+L::HeadOffset,512,false);
 for(unsigned i=0;i<128;i++)header[i]=Words(L::Head)[i]=Words(R::Scratch)[i];
 if(!H::Shape(header,p->boot,p->state[Bank],p->state[Workspace]))scalar_fault_park();
 HeadSeal(header);Normalize(header);unsigned accepted=0,rows=0,bestRow=248320,bestBits=0;
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
 q38_token_phase_mark(QPhase::HeadMatrix);
#endif
 for(unsigned w=0;w<119;w++)Closed(header,w);
 for(unsigned wave=0;wave<9;wave++){
  InputSeal();unsigned workers=wave==8?21:119;
  for(unsigned w=0;w<workers;w++){
   uint32_t job[64];
#ifdef PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION
   if(!H::Job(header,wave*119+w,Words(R::Nonces)[w],p->state[Position],PeerBoot(w),job))scalar_fault_park();
#else
   if(!T::Job(header,p->state[Position],wave*119+w,Words(R::Nonces)[w],PeerBoot(w),job))scalar_fault_park();
#endif
   for(unsigned i=0;i<64;i++)Words(R::PeerJobs+w*256)[i]=job[i];
   Dispatch(header,w);
  }
  for(unsigned w=0;w<workers;w++)Release(header,w);
  for(unsigned w=0;w<workers;w++){Gather(header,w,wave*119+w,bestRow,bestBits,rows);accepted++;}
  HeadSeal(header);InputSeal();
 }
 if(accepted!=973 || rows!=248320 || bestRow>=248320)scalar_fault_park();
 for(unsigned w=0;w<119;w++)Closed(header,w);
 unsigned reason=p->state[Position]+1>=p->state[Limit]?1:
#ifdef PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE
  p->state[StopEos] && p->state[Position]+1>=p->state[PromptCount] && FullOffloadEog(bestRow)?2:0;
#else
  p->state[StopEos] && p->state[Position]+1>=p->state[PromptCount] && (bestRow==p->state[Eos0] || bestRow==p->state[Eos1])?2:0;
#endif
#ifdef PIONEER_Q38_TOKEN_CANCEL_BOUNDARY
 q38_token_cancellation_point(); // fully closed head, BEFORE mutable commit
#endif
 unsigned next=bestRow;
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
 q38_token_phase_mark(QPhase::Feedback);
#endif
 if(!reason){
  if(p->state[Position]+1<p->state[PromptCount])next=Prompt(p->state[Position]+1);
  uint32_t feedback[16];Feedback(header,next,feedback);
  if(feedback[2]!=next || feedback[3]!=p->state[Position]+1 || feedback[14] || feedback[15]!=1)scalar_fault_park();
 }
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
 q38_token_phase_mark(QPhase::StateCommit);
#endif
 CommitState(header,!reason);
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE
 q38_token_phase_mark(QPhase::Publication);
#endif
 PublishToken(header,bestRow,bestBits,!reason);
#ifdef PIONEER_Q38_TOKEN_CANCEL_BOUNDARY
 q38_token_cancellation_point(); // finish an already-started commit first
#endif
 if(reason)Finish(header,reason);
 FeedbackSeal(header);return next;
}
