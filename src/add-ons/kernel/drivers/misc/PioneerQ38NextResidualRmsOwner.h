/* SPDX-License-Identifier: MIT */
// Included INSIDE the SAME decoder after its actual native62 owner. No
// caller-ready facade,initial RMS retag,CPU products or new public route.
#if !defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE != 1
#error Exact separate native63 SAME native62 residual+RMS consumer required
#endif
 friend class PioneerQ38NextFfnProducer;
 PioneerQ38NextResidualRmsState* nextResidual=nullptr;PioneerQ38NextFfnProducer nextFfnInput;
 unsigned nextResidualClaimEpoch=0,nextResidualPublishedEpoch=0;
 template<class IO> bool NextResidualAlive(IO& io)const{return nextResidual && (!nextResidual->timing || nextResidual->deadline.Observe(io.Q38CohortNow()));}
 template<class IO> bool NextResidualGuard(IO& io,unsigned w){return NextResidualAlive(io) && Guard(io,w) && NextResidualAlive(io);}
 template<class IO> bool NextResidualMemory(IO& io,uint32_t a,uint32_t& v){return NextResidualAlive(io) && io.ProjectionGraphMemoryReadChecked(0,a,v) && NextResidualAlive(io);}
 template<class IO> bool NextResidualPanel(IO& io,uint32_t a,uint32_t& v){if(!NextResidualAlive(io))return false;v=io.Q38PanelRead(a);return NextResidualAlive(io);}
 template<class IO> bool NextResidualStatus(IO& io,unsigned a,uint32_t& v){if(!NextResidualAlive(io))return false;v=io.RuntimeStatusRead(a);return NextResidualAlive(io);}
 template<class IO> bool NextResidualPc(IO& io,uint32_t& v){if(!NextResidualAlive(io))return false;v=io.RuntimePcRead();return NextResidualAlive(io);}
 template<class IO> bool NextResidualNoc(IO& io,unsigned a,uint32_t& v){if(!NextResidualAlive(io))return false;v=io.ResidentNocRead(a);return NextResidualAlive(io);}
 template<class IO> bool NextResidualPause(IO& io){if(!NextResidualAlive(io))return false;io.Pause();return NextResidualAlive(io);}
 template<class IO> bool NextResidualReset(IO& io,uint32_t v){if(!NextResidualAlive(io))return false;io.RuntimeResetWrite(v);return NextResidualAlive(io);}
 bool NextResidualData()const{
  if(!nextResidual || !projection || !history || !nextOutput || !nextOutput->done || !nextOutput->stopped
   || !nextOutput->bootPrepared || !nextOutput->rootProof || !nextOutput->rootHeld || !NextOutputData()
   || nextResidual->parent!=nextOutput || nextResidual->input!=&nextAttentionResult || !nextOutputClaimed
   || !nextOutputPublishedEpoch || nextOutputPublishedEpoch>=0x7ffffffe || !nextResidualClaimEpoch
   || nextResidualClaimEpoch!=nextOutputPublishedEpoch+1 || nextResidual->epoch!=nextResidualClaimEpoch
   || ((phase<47 || phase>49) && phase!=7 && phase!=8
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
    && !NextFfnRetainedPhase()
#endif
    )
   || nextAttentionResult.owner!=this || nextAttentionResult.state!=nextOutput || nextAttentionResult.parent!=&nextAttention
   || nextAttentionResult.canonical!=projection->canonical || nextAttentionResult.ledger!=projection->ledger
   || nextAttentionResult.pages!=projection->pages || !(nextAttentionResult.identity==projection->identity)
   || nextAttentionResult.token!=projection->token || nextAttentionResult.position || projection->position
   || nextAttentionResult.bank!=projection->bank || nextAttentionResult.layer!=1 || nextAttentionResult.phase!=nextOutputPublishedEpoch
   || (nextResidual->timing?nextOutput->active!=&nextResidual->deadline:nextOutput->active!=nullptr)
   || !nextResidual->plan.Matches(*projection->canonical,1,0,projection->token,projection->bank,projection->identity,
    projection->cohort->authority.source[0],nextResidual->epoch)
   || !nextResidual->plan.LearnedPagesPresent(projection->ledger,projection->pages)
   || !PioneerQ38NextResidualRmsLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  const auto* d=nextResidual->plan.Descriptor();
  if(!d || nextAttentionResult.resultAddress!=d[4]+TT_Q38_NEXT_GDN_OUTPUT_RESULT
   || nextAttentionResult.residualAddress!=nextInput.residualAddress || nextAttentionResult.residualAddress!=d[4]+TT_Q38_RMS_BACKUP)return false;
  for(unsigned i=0;i<16;i++)if(nextAttentionResult.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(!nextOutput->image[w] || !nextOutput->held[w] || (w && !nextOutput->stopReceipt[w]))return false;
  return true; // historical SAME producer fields,never its expired Current()
 }
 template<class IO> bool NextResidualImage(IO& io){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(0);i++){
   uint32_t address=PioneerQ38ProjectionGraphCodeAddress(0,i),v;
   if(i<=10240 || PioneerQ38ProjectionGraphProgramImmutable(0,address)){
    uint32_t expected=i<=10240?PioneerQ38NextResidualRmsProgramWord(i):PioneerQ38NextGdnOutputProgramWord(0,address);
    if(!NextResidualMemory(io,address,v) || v!=expected)return false;
   }
  }
  return NextResidualAlive(io); // old ROOT main exact;only existing40KiB prefix replaced
 }
 template<class IO> bool NextResidualTerminal(IO& io){
  uint32_t marker,entry,count;
  if(!NextResidualStatus(io,2,marker) || marker!=TT_Q38_NEXT_RESIDUAL_RMS_MARKER || !NextResidualStatus(io,0,entry)
   || entry!=0x6400c || !NextResidualStatus(io,1,count) || count!=5120)return false;
  if(!projection->running[0])return resets[0]==0x47800 && nextResidual->held && nextResidual->receipt;
  // A held core's PC is not sampled as a live execution receipt.
  for(unsigned i=0;i<2;i++){uint32_t pc;if(!NextResidualPause(io) || !NextResidualPc(io,pc) || (pc!=0x64038 && pc!=0x6403c))return false;}
  return NextResidualAlive(io);
 }
 template<class IO> bool NextResidualCurrent(IO& io){
  if(!NextResidualAlive(io) || !NextResidualData() || !nextResidual->image || !NextResidualImage(io))return false;
  if(nextResidual->staged){
   const auto* d=nextResidual->plan.Descriptor();
   for(unsigned i=0;i<64;i++){uint32_t v;if(!NextResidualMemory(io,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,v) || v!=d[i])return false;}
  }
  if(nextResidual->receipt){
   if(!NextResidualTerminal(io))return false;
   for(unsigned i=0;i<32;i++){uint32_t v;if(!NextResidualPanel(io,TT_Q38_FFN_RESIDENT_SUMMARY+i*4,v) || v!=nextResidual->accepted[i])return false;}
  }
  return NextResidualAlive(io);
 }
 template<class IO> bool NextResidualReceipt(IO& io){
  if(!NextResidualGuard(io,0) || !NextResidualTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !NextResidualAlive(io)
   || !NextResidualCurrent(io))return false;
  uint32_t summary[32],after[8],v;
  for(unsigned i=0;i<32;i++)if(!NextResidualPanel(io,TT_Q38_FFN_RESIDENT_SUMMARY+i*4,summary[i]))return false;
  for(unsigned i=0;i<8;i++)if(!NextResidualNoc(io,PioneerQ38PanelCounter(i),after[i]))return false;
  const auto* d=nextResidual->plan.Descriptor();
  if(!PioneerQ38NextResidualRmsSummary(summary,d,nextResidual->before,after))return false;
  const uint32_t last[]={0xa4000,0,129,d[4]+16384,0,d[8],0,0x212,4096,0};
  for(unsigned i=0;i<10;i++)if(!NextResidualNoc(io,i*4,v) || v!=last[i])return false;
  for(unsigned i=0;i<32;i++)nextResidual->accepted[i]=summary[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  nextResidual->receipt=verified[0]=true;
  return NextResidualGuard(io,0) && NextResidualCurrent(io);
 }
 bool MatchesNextFfn(const PioneerQ38NextFfnProducer& v)const{
  if(&v!=&nextFfnInput || v.owner!=this || !nextResidual || !nextResidual->done || !nextResidual->image
   || !nextResidual->staged || !nextResidual->receipt || !nextResidual->held || !complete || closed || Unsafe() || phase!=49
   || !NextResidualData() || !MatchesNativeOwner(*projection,*history) || nextResidualPublishedEpoch!=nextResidualClaimEpoch
   || v.state!=nextResidual || v.parent!=&nextAttentionResult || v.canonical!=projection->canonical || v.ledger!=projection->ledger
   || v.pages!=projection->pages || !(v.identity==projection->identity) || v.token!=projection->token || v.position || v.bank!=projection->bank
   || v.layer!=1 || v.phase!=nextResidualPublishedEpoch)return false;
  const auto* d=nextResidual->plan.Descriptor();
  if(v.normalizedAddress!=d[4] || v.residualAddress!=d[4]+TT_Q38_RMS_BACKUP)return false;
  for(unsigned i=0;i<16;i++)if(v.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800
   || !verified[w] || !nextOutput->image[w] || !nextOutput->held[w] || (w && !nextOutput->stopReceipt[w]))return false;
  return !projection->cohort->rootRunning;
 }
 bool MatchesNextFfnCohort(const PioneerQ38NextFfnProducer& v,const PioneerQ38DispatchLease& c)const{return MatchesNextFfn(v) && projection->cohort==&c;}
 bool NextResidualCloseReady()const{
  if(nextResidual)return phase==49 && nextFfnInput.Current();
  if(nextOutput)return phase==46 && nextAttentionResult.Current();
  if(nextGdn)return phase==41 && nextAttention.Current();
  if(nextConv)return phase==36 && nextConvolution.Current();
  if(nextProjection)return phase==31 && nextProjections.Current();
  if(nextRms)return phase==26 && nextRms->done && nextInput.Current();
  if(ffn)return phase==24 && ffn->done && ffn->stopped && ffnHidden.Current();
  if(residual)return phase==18 && residual->done && postAttention.Current();
  return output?phase==16 && output->done && output->stopped:phase==12;
 }
public:
 bool NextResidualRmsCompleted()const{return nextFfnInput.Current();}
 const PioneerQ38NextFfnProducer* NextFfnInput()const{return nextFfnInput.Current()?&nextFfnInput:nullptr;}
 template<class IO> bool RunNextResidualRms(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(nextResidual || nextFfnInput.owner || nextResidualClaimEpoch || nextResidualPublishedEpoch || !nextAttentionResult.Current()
   || phase!=46 || !complete || !projection || !history || !nextOutput || nextOutput->active
   || !MatchesNativeOwner(*projection,*history) || !NextOutputData() || !nextOutputPublishedEpoch || nextOutputPublishedEpoch>=0x7ffffffe
   || r.nextResidualInstalled || r.nextResidualRuns || r.nextResidualHeld || r.nextResidualPublished
   || !PioneerQ38NextResidualRmsLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  auto* state=new(std::nothrow) PioneerQ38NextResidualRmsState;if(!state)return false;
  auto& p=*projection;state->parent=nextOutput;state->input=&nextAttentionResult;state->epoch=nextOutputPublishedEpoch+1;
  if(!state->plan.Build(*p.canonical,1,0,p.token,p.bank,p.identity,p.cohort->authority.source[0],state->epoch)
   || !state->plan.LearnedPagesPresent(p.ledger,p.pages)){delete state;return false;}
  // Claim genuine output AND actual native58 residual BEFORE first IO.
  nextResidual=state;nextResidualClaimEpoch=state->epoch;phase=47;r.phase=47;complete=false;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !NextResidualGuard(io,w) || !NextOutputCurrent(io,w))return Fail(io,r);
  }
  r.worker=0;if(!NextResidualGuard(io,0))return Fail(io,r);
  for(unsigned i=0;i<=10240;i++){
   r.word=i;if((i%256==0 && !NextResidualGuard(io,0))
    || !Write(io,0,PioneerQ38ProjectionGraphCodeAddress(0,i),PioneerQ38NextResidualRmsProgramWord(i)))return Fail(io,r);
   r.nextResidualInstalled++;
  }
  state->image=true;if(!NextResidualGuard(io,0) || !NextResidualImage(io))return Fail(io,r);
  const auto* d=state->plan.Descriptor();
  for(unsigned i=0;i<64;i++)if(!Write(io,0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,d[i]))return Fail(io,r);
  for(unsigned i=0;i<32;i++)if(!Write(io,0,TT_Q38_FFN_RESIDENT_SUMMARY+i*4,0))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);uint32_t v;if(!NextResidualStatus(io,i,v) || v)return Fail(io,r);}
  state->staged=true;for(unsigned i=0;i<8;i++)state->before[i]=counts[0][i];
  if(!NextResidualGuard(io,0) || !NextResidualCurrent(io))return Fail(io,r);
  resets[0]=0x87ffff;
  if(!NextResidualReset(io,resets[0]) || !NextResidualPause(io) || !NextResidualGuard(io,0))return Fail(io,r);
  if(!state->deadline.Begin(io.Q38CohortNow()))return Fail(io,r);
  state->timing=true;nextOutput->active=&state->deadline; // SAME one-shot deadline also brackets deep peer reads
  if(!NextResidualAlive(io))return Fail(io,r);
  phase=48;r.phase=48;verified[0]=false;
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;resets[0]=0x46800;
  if(!NextResidualReset(io,resets[0]) || !NextResidualGuard(io,0))return Fail(io,r);
  r.nextResidualRuns++;bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!NextResidualGuard(io,0))return Fail(io,r);
   uint32_t marker;if(!NextResidualStatus(io,2,marker) || (marker && marker!=TT_Q38_NEXT_RESIDUAL_RMS_MARKER))return Fail(io,r);
   if(marker){if(!NextResidualReceipt(io))return Fail(io,r);done=true;break;}
   uint32_t pc;if(!NextResidualPause(io) || !NextResidualPc(io,pc)
    || (pc!=0x6000 && (pc<0x64000 || pc>=0x64000+sizeof(PioneerQ38NextResidualRmsCode) || pc==0x6403c || pc==0x64058)))return Fail(io,r);
  }
  if(!done || !NextResidualGuard(io,0) || !NextResidualTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;
  if(!NextResidualReset(io,resets[0]) || !NextResidualPause(io) || !NextResidualGuard(io,0))return Fail(io,r);
  resets[0]=0x47800;
  if(!NextResidualReset(io,resets[0]) || !NextResidualPause(io) || !NextResidualGuard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;p.resets[0]=resets[0];state->held=true;r.nextResidualHeld=1;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !NextResidualGuard(io,w) || !Current(io,w) || !NextResidualAlive(io))return Fail(io,r);
  }
  if(!NextResidualAlive(io))return Fail(io,r);
  state->done=complete=true;phase=49;r.phase=49;nextResidualPublishedEpoch=state->epoch;auto& v=nextFfnInput;
  v.owner=this;v.state=state;v.parent=&nextAttentionResult;v.canonical=p.canonical;v.ledger=p.ledger;v.pages=p.pages;v.identity=p.identity;
  v.token=p.token;v.position=p.position;v.bank=p.bank;v.layer=1;v.phase=state->epoch;
  v.normalizedAddress=d[4];v.residualAddress=d[4]+TT_Q38_RMS_BACKUP;
  for(unsigned i=0;i<16;i++)v.source[i]=p.cohort->authority.source[0][i];
  if(!MatchesNextFfn(v) || !NextResidualAlive(io))return Fail(io,r);
  state->timing=false;nextOutput->active=nullptr;r.nextResidualPublished=1;return true;
 }
private:
