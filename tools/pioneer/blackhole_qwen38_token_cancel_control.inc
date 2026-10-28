/* SPDX-License-Identifier: MIT */
// Separate experimental parent variant ONLY. Request is a monotonic RAW L1
// control word in this exact exclusive lease, never a ready/completion flag.
// Checkpoints follow real closed layers, the complete output head, and actual
// token publication. Never interrupt mutable commit or assume STOP succeeded.
#if !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1
#error Exact persistent-token closed-boundary cancellation variant required
#endif
extern "C" __attribute__((noinline,section(".conv.cancel"))) void q38_token_cancellation_point(){
 auto* p=Owner();if(p->phase!=1 && p->phase!=4 && p->phase!=6)scalar_fault_park();
 Scope();unsigned request=*Words(T::CancelLocal);Fence();
 if(!request)return;
 if(request!=T::CancelMagic || *Words(T::CancelLocal)!=request)scalar_fault_park();
 unsigned stage=p->phase==1?1:p->phase==4?2:3;
 uint32_t header[128];for(unsigned i=0;i<128;i++)header[i]=Words(L::Head)[i];
 if(!H::Shape(header,p->boot,p->state[Bank],p->state[Workspace]))scalar_fault_park();
 p->state[CancelStage]=stage;p->phase=7;HeadSeal(header);
 if(stage>1)InputSeal();
 // All119 retained last jobs/sources/reset states/doorbells/counters must
 // close BEFORE the first STOP write. No stale candidate stands for work.
 for(unsigned w=0;w<119;w++)Closed(header,w);
 for(unsigned w=0;w<119;w++){
  unsigned n=Words(R::Nonces)[w];if(n>=0x7ffffffe)scalar_fault_park();
  uint32_t job[64];for(unsigned i=0;i<64;i++)job[i]=0;
  unsigned stopLayer=stage==1?p->state[Layer]:64;
  const unsigned head[]={G::Magic,n+1,G::Stop,w+1,G::Peer(w+1),stopLayer,p->state[Position],9998};
  for(unsigned i=0;i<8;i++){job[i]=head[i];job[8+i]=p->boot[8+i];}
  if(!G::JobShape(job,PeerBoot(w),n,0))scalar_fault_park();
  for(unsigned i=0;i<64;i++)Words(R::PeerJobs+w*256)[i]=job[i];
  Dispatch(header,w);
 }
 for(unsigned w=0;w<119;w++)Release(header,w);
 for(unsigned w=0;w<119;w++)Closed(header,w,true);
 HeadSeal(header);if(stage>1)InputSeal();
 for(unsigned i=0;i<64;i++)Words(T::TerminalLocal)[i]=0;
 const unsigned terminal[]={0,1,3,p->state[Committed],p->state[Position],119,p->state[Completed],stage>1?973U:0U,p->state[Slot],1,stage};
 for(unsigned i=0;i<11;i++)Words(T::TerminalLocal)[i]=terminal[i];
 for(unsigned i=0;i<8;i++){Words(T::TerminalLocal)[12+i]=p->boot[8+i];Words(T::TerminalLocal)[36+i]=p->state[Counts+i];}
 for(unsigned i=0;i<16;i++)Words(T::TerminalLocal)[20+i]=p->boot[16+i];
 // Inactive GDN state/current KV from a partial token is NOT committed or
 // reusable by this receipt. Native cleanup must discard/reinitialize that
 // mutable context; only the actually proved STOP closes the worker lease.
 Fence();Words(T::TerminalLocal)[0]=T::TerminalMagic;Fence();q38_global_chain_stopped();
}
