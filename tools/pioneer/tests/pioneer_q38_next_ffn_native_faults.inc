/* SPDX-License-Identifier: MIT */
// Included INSIDE the actual native65 test after genuine native63. TEST
// forks preserve the actual in-flight owner stack,not caller-ready flags.
auto checkNextFfnFault=[&](unsigned fault,bool ran){
 bool failedClose=fault==6 && ran?!CloseQ38ResidentDecoderModel(*f.c,f.decoderResult):false;
 printf("NATIVE65_FAULT_OBS kind%u ran%u injected%u phase%u worker%u word%u installed%u booted%u runs%u stopped%u published%u actualStarts%u learnedReads%llu\n",
  fault,unsigned(ran),unsigned(f.transport->NextFfnFaultInjected()),f.decoderResult.phase,f.decoderResult.worker,f.decoderResult.word,
  f.decoderResult.nextFfnInstalled,f.decoderResult.nextFfnBooted,f.decoderResult.nextFfnRootRuns,f.decoderResult.nextFfnStopped,
  f.decoderResult.nextFfnPublished,f.transport->NextFfnStarts(),(unsigned long long)source->NextFfnReads());
 assert(f.transport->NextFfnFaultInjected());
 if(fault==6)assert(ran && failedClose && f.decoderResult.nextFfnPublished==1);
 else assert(!ran && !f.decoderResult.nextFfnPublished);
 assert(!nextFfn->Current() && !m->decoder.NextFfnInput() && !m->decoder.NextFfnHiddenInput() && !m->decoder.NextFfnCompleted());
 if(fault==1)assert(!f.transport->NextFfnStarts() && !f.decoderResult.nextFfnInstalled && !source->NextFfnReads());
 if(fault==2)assert(f.transport->NextFfnStarts()==119 && f.decoderResult.nextFfnBooted==119 && !f.decoderResult.nextFfnRootRuns && !source->NextFfnReads());
 if(fault==3)assert(f.transport->NextFfnStarts()==120 && f.decoderResult.nextFfnBooted==119 && f.decoderResult.nextFfnRootRuns==1 && !source->NextFfnReads());
 if(fault==7)assert(f.transport->NextFfnStarts()==1 && !f.decoderResult.nextFfnBooted && !f.decoderResult.nextFfnRootRuns && !source->NextFfnReads());
 if(fault==15)assert(!f.transport->NextFfnStarts() && f.decoderResult.nextFfnInstalled==120 && !source->NextFfnReads());
 if(fault==16)assert(f.transport->NextFfnStarts()==119 && f.decoderResult.nextFfnBooted==118 && !f.decoderResult.nextFfnRootRuns && !source->NextFfnReads());
 if(fault<=3 || fault==7 || fault==15 || fault==16)assert(f.io.wholeWorkspace==beforeNextFfn);
 if(fault==4 || fault==8 || fault==10 || fault==11)assert(f.transport->NextFfnStarts()==120 && source->NextFfnReads()==65280 && !f.decoderResult.nextFfnStopped);
 if(fault==5)assert(f.transport->NextFfnStarts()==121 && f.decoderResult.nextFfnStopped==118 && source->NextFfnReads()==65280);
 if(fault==12 || fault==13 || fault==14)assert(f.transport->NextFfnStarts()==121 && f.decoderResult.nextFfnStopped==119 && source->NextFfnReads()==65280);
 if(fault==7 || (fault>=10 && fault<=16))assert(f.transport->Touches()==f.transport->NextFfnFaultTouch());
 assert(f.io.recurrentWords==beforeNextFfnState);for(auto word:f.io.historyWords)assert(!word);
 f.UnknownNative();unsigned host=f.io.touches,device=f.transport->Touches();
 assert(!RunQ38NextFfnResidual(*f.c,forged) && !RunQ38NextResidualRms(*f.c,forged) && !RunQ38NextGdnOutput(*f.c,forged)
  && !RunQ38NextGdn(*f.c,forged) && !RunQ38NextConv(*f.c,forged) && !RunQ38NextProjections(*f.c,forged)
  && !RunQ38ResidentFfn(*f.c,forged) && !CloseQ38ResidentDecoderModel(*f.c,forged) && !EndFactory(*f.c)
  && f.io.touches==host && f.transport->Touches()==device);
 printf("PASS SAME ACTUAL native65 fault%u emitted code/frame/source/receipt/PC/restore/one-shot boot-RUN-STOP-HOLD-publication clock failure;ALL owners retained/ZERO following IO-reset-restore-retry-END\n",fault);
};
if(nextFfnFaults)for(unsigned fault:{1U,2U,3U,7U,9U,15U,16U}){
 pid_t child=fork();assert(child>=0);if(!child){
  f.transport->NextFfnFault(fault);checkNextFfnFault(fault,RunQ38NextFfnResidual(*f.c,f.decoderResult));_exit(0);
 }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
}
unsigned lateNextFfnFault=0;
struct LateNextFfnContext {ProjectionNativeTransport* transport;unsigned* fault;};
LateNextFfnContext lateNextFfn={f.transport.get(),&lateNextFfnFault};
if(nextFfnFaults)f.transport->NextFfnRunTerminalCheckpoint([](void* context){
 auto& late=*static_cast<LateNextFfnContext*>(context);
 for(unsigned fault:{4U,5U,6U,8U,10U,11U,12U,13U,14U}){
  pid_t child=fork();assert(child>=0);if(!child){*late.fault=fault;late.transport->NextFfnFault(fault);return;}
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
},&lateNextFfn);
