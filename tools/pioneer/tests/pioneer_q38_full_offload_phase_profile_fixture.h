/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE_FIXTURE_H
#define PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE_FIXTURE_H
// Fabricated timing DATA only. No native owner,device or model computation.
static void FillFullPhaseMetric(PioneerQ38TokenPhaseProfileControl::Packet& p,unsigned committed,bool invalid=false,unsigned prompts=0){
 namespace Q=PioneerQ38SelectedPhaseProfileData;
 uint64_t now=UINT64_C(1)<<40;
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
 Q::Initialize(p.words);
#else
 (void)prompts;Q::Initialize(p.words,true,now);
#endif
 auto mark=[&](unsigned next){now+=19;Q::Advance(p.words,true,now,next);};
 for(unsigned token=0;token<committed;token++){
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
  Q::Position(p.words,token,prompts,true,now);
#endif
  for(unsigned layer=0;layer<64;layer++){
   mark(Q::Metadata);mark(layer%4==3?Q::Dense:Q::Recurrent);
#if defined(PIONEER_Q38_BF16_FULL_PROFILE) || defined(PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE)
   for(unsigned job=0;job<(layer%4==3?7U:8U);job++)
    for(unsigned phase:{Q::MatrixPrepare,Q::MatrixDispatch,Q::MatrixGatherWait,Q::MatrixFinish,
      layer%4==3?Q::Dense:Q::Recurrent})mark(phase);
#endif
  }
  for(unsigned next:{Q::HeadNormalize,Q::HeadMatrix,Q::Feedback,Q::StateCommit,Q::Publication})mark(next);
 }
 mark(Q::FinalStop);Q::Advance(p.words,true,now+19,Q::Phases,committed);
 if(invalid){p.words[Q::State]=Q::Invalid;p.words[Q::Error]=Q::ClockUnstable;}
 auto& r=p.reply;r.version=r.operation=r.started=r.ready=1;
 auto* d=r.detail;d[0]=d[1]=d[6]=1;d[2]=4;d[4]=59;
 d[7]=Q::Closed(p.words);d[8]=committed;d[9]=60;d[10]=p.words[Q::State];d[11]=p.words[Q::Error];
 assert(PioneerQ38TokenPhaseProfileControl::ReplyShape(p));
}
#endif
