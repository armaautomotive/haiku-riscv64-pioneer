/* SPDX-License-Identifier: MIT */
// Included INSIDE SAME real decoder;not a retagged first-layer GDN owner.
#if !defined(PIONEER_Q38_NEXT_GDN_NATIVE) || PIONEER_Q38_NEXT_GDN_NATIVE != 1
#error Exact separate native61 next-layer GDN required
#endif
 friend class PioneerQ38NextGdnProducer;
 PioneerQ38NextGdnState* nextGdn=nullptr;PioneerQ38NextGdnProducer nextAttention;
 unsigned nextGdnClaimEpoch=0; // retained CPU lineage for actual restored Close/END,never caller readiness
 bool NextGdnData()const{
  if(!nextGdn || !nextGdn->built || !nextConv || !nextConv->done || !NextConvData() || !recurrent
   || nextGdn->parent!=nextConv || nextGdn->input!=&nextConvolution || nextConvolution.owner!=this || nextConvolution.state!=nextConv
   || nextConvolution.parent!=&nextProjections || nextConvolution.canonical!=projection->canonical || nextConvolution.ledger!=projection->ledger
   || nextConvolution.pages!=projection->pages || !(nextConvolution.identity==projection->identity) || nextConvolution.token!=projection->token
   || nextConvolution.position || projection->position || nextConvolution.bank!=projection->bank || nextConvolution.layer!=1 || nextGdn->layer!=1
   || nextConvolution.phase!=nextConv->epoch || nextGdn->epoch!=nextConv->epoch+1 || nextGdnClaimEpoch!=nextGdn->epoch
   || ((phase<37 || phase>41) && phase!=7 && phase!=8
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
    && !(nextOutput && phase>=42 && phase<=46)
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
    && !(nextResidual && phase>=47 && phase<=49)
#endif
    )
   || !recurrent->OwnsNextLayer(this,projection->identity,1,nextGdn->epoch)
   || nextConvolution.outputAddress!=nextInput.normalizedAddress+TT_Q38_CONV_OUTPUT
   || nextConvolution.residualAddress!=nextInput.residualAddress)return false;
  for(unsigned i=0;i<16;i++)if(nextConvolution.source[i]!=projection->cohort->authority.source[0][i])return false;
  return true;
 }
 bool NextGdnCanonical(unsigned w)const{
  if(!w || w>96 || !NextGdnData() || !PioneerQ38NextGdnLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  const auto& p=*projection;const auto& s=*nextGdn;
  if(!s.jobs[w-1].Matches(*p.canonical,*recurrent,w-1,p.bank,p.identity,p.cohort->authority.source[w],s.epoch)
   || !s.jobs[w-1].LearnedPagesPresent(p.ledger,p.pages)
   || !s.gated[w-1].Matches(*p.canonical,s.jobs[w-1].Descriptor(),s.jobs[w-1].StateDescriptor())
   || !s.gated[w-1].LearnedPagePresent(p.ledger,p.pages))return false;
  const auto* d=s.jobs[w-1].Descriptor();return d[4]+TT_Q38_CONV_OUTPUT==nextConvolution.outputAddress;
 }
 static uint32_t NextGdnWord(unsigned w,uint32_t a){
  if(w && w<=96){
   if(a==0x6000)return PioneerQ38GdnRendezvousImageWord(0);
   if(a>=0x64000 && a-0x64000<TT_Q38_GDN_RENDEZVOUS_TEXT_BYTES)return PioneerQ38GdnRendezvousImageWord(1+(a-0x64000)/4);
  }
  return NextConvWord(w,a); // full immutable original mixed tail remains tracked
 }
 template<class IO> bool NextGdnImage(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a) && (!io.ProjectionGraphMemoryReadChecked(w,a,v) || v!=NextGdnWord(w,a)))return false;
  }return true;
 }
 template<class IO> bool NextGdnStatus(IO& io)const{
  return io.RuntimeStatusRead(2)==TT_Q38_GDN_RENDEZVOUS_MARKER && io.RuntimeStatusRead(0)==0x6400c && io.RuntimeStatusRead(1)==64;
 }
 template<class IO> bool NextGdnTerminal(IO& io){
  if(!NextGdnStatus(io))return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
  return true;
 }
 template<class IO> bool NextGdnCurrent(IO& io,unsigned w){
  if(!NextGdnCanonical(w) || !nextGdn->image[w] || !NextGdnImage(io,w))return false;
  auto& s=*nextGdn;const auto* gd=s.gated[w-1].Descriptor();
  if(s.staged[w]){
   const uint32_t* tables[]={s.jobs[w-1].Descriptor(),s.jobs[w-1].StateDescriptor(),gd};
   const unsigned addresses[]={TT_Q38_GDN_INPUT_DESCRIPTOR,TT_Q38_RGDN_DESCRIPTOR,TT_Q38_GDN_GATED_DESCRIPTOR};
   for(unsigned t=0;t<3;t++)for(unsigned i=0;i<64;i++){
    uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,addresses[t]+i*4,v) || v!=tables[t][i])return false;
   }
   if(s.receipt[w] || !projection->running[w]){
    for(unsigned i=0;i<32;i++)if(io.Q38PanelRead(TT_Q38_GDN_GATED_READY+i*4)!=(s.receipt[w]?PioneerQ38GdnRendezvousReadyWord(gd,i%16,i>=16):0U))return false;
    for(unsigned i=0;i<16;i++)if(io.Q38PanelRead(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4)!=(s.receipt[w]?s.clock[w-1][i]:0U))return false;
   }
  }
  if(s.receipt[w]){
   if(projection->running[w]){if(!NextGdnTerminal(io))return false;}
   else if(resets[w]!=0x47800 || !NextGdnStatus(io))return false; // never sample held live PC
   const unsigned addresses[]={TT_Q38_GDN_INPUT_SUMMARY,TT_Q38_RGDN_SUMMARY,TT_Q38_GDN_GATED_SUMMARY};
   for(unsigned t=0;t<3;t++)for(unsigned i=0;i<32;i++)if(io.Q38PanelRead(addresses[t]+i*4)!=s.accepted[w-1][t*32+i])return false;
  }
  return true;
 }
 template<class IO> bool NextGdnReceipt(IO& io,unsigned w){
  if(!NextGdnTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !NextGdnCurrent(io,w))return false;
  auto& s=*nextGdn;const auto* d=s.jobs[w-1].Descriptor();const auto* sd=s.jobs[w-1].StateDescriptor();const auto* gd=s.gated[w-1].Descriptor();
  uint32_t input[32],state[32],gated[32],after[8],clock[16];
  for(unsigned i=0;i<32;i++){input[i]=io.Q38PanelRead(TT_Q38_GDN_INPUT_SUMMARY+i*4);state[i]=io.Q38PanelRead(TT_Q38_RGDN_SUMMARY+i*4);gated[i]=io.Q38PanelRead(TT_Q38_GDN_GATED_SUMMARY+i*4);}
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  if(!PioneerQ38GdnInputSummary(input,d,s.before[w-1],input+16) || !PioneerQ38ResidentGdnSummary(state,sd,state+8,state+16)
   || !PioneerQ38GdnRendezvousSummary(gated,gd,d,sd,gated+8,after))return false;
  for(unsigned i=0;i<8;i++)if(input[16+i]!=state[8+i] || state[16+i]!=gated[8+i])return false;
  for(unsigned i=0;i<16;i++){
   clock[i]=io.Q38PanelRead(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4);
   if(io.Q38PanelRead(TT_Q38_GDN_GATED_READY+i*4)!=PioneerQ38GdnRendezvousReadyWord(gd,i,false))return false;
   if(io.Q38PanelRead(TT_Q38_GDN_GATED_PEER_READY+i*4)!=PioneerQ38GdnRendezvousReadyWord(gd,i,true))return false;
  }
  if(!PioneerQ38GdnRendezvousClockReceipt(clock,gd,gated[5]))return false;
  const uint32_t last[]={0x8ea00,0,gd[9],gd[4]+gd[43],0,gd[8],0,0x212,256,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<32;i++){s.accepted[w-1][i]=input[i];s.accepted[w-1][32+i]=state[i];s.accepted[w-1][64+i]=gated[i];}
  for(unsigned i=0;i<16;i++)s.clock[w-1][i]=clock[i];
  for(unsigned i=0;i<8;i++)counts[w][i]=after[i];
  for(unsigned i=0;i<10;i++)command[w][i]=last[i];
  s.polls[w-1]=gated[5];s.receipt[w]=verified[w]=true;return Guard(io,w) && NextGdnTerminal(io);
 }
 bool MatchesNextAttention(const PioneerQ38NextGdnProducer& v)const{
  if(&v!=&nextAttention || v.owner!=this || !nextGdn || !nextGdn->done || !complete || closed || Unsafe() || phase!=41 || !NextGdnData()
   || !MatchesNativeOwner(*projection,*history) || !recurrent->NextLayerPublished(this,projection->identity,1,nextGdn->epoch)
   || v.state!=nextGdn || v.parent!=&nextConvolution || v.canonical!=projection->canonical || v.ledger!=projection->ledger || v.pages!=projection->pages
   || !(v.identity==projection->identity) || v.token!=projection->token || v.position || v.bank!=projection->bank || v.layer!=1 || v.phase!=nextGdn->epoch
   || v.inputAddress!=nextInput.normalizedAddress+TT_Q38_GDN_GATED_OUTPUT || v.residualAddress!=nextInput.residualAddress)return false;
  for(unsigned i=0;i<16;i++)if(v.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800 || !verified[w]
   || (w && w<=96 && (!nextGdn->image[w] || !nextGdn->staged[w] || !nextGdn->receipt[w])))return false;
  return !projection->cohort->rootRunning;
 }
 bool MatchesNextAttentionCohort(const PioneerQ38NextGdnProducer& v,const PioneerQ38DispatchLease& c)const{return MatchesNextAttention(v) && projection->cohort==&c;}
public:
 bool NextGdnCompleted()const{return nextAttention.Current();}
 const PioneerQ38NextGdnProducer* NextAttentionInput()const{return nextAttention.Current()?&nextAttention:nullptr;}
 template<class IO> bool RunNextGdn(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(nextGdn || nextAttention.owner || !nextConvolution.Current() || !nextConv || nextConv->epoch>=0x7ffffffe || !projection || !history || !recurrent
   || !MatchesNativeOwner(*projection,*history) || !MatchesRecurrentNativeOwner(*recurrent) || !recurrent->layerNextPublished
   || r.nextGdnInstalled || r.nextGdnArmed || r.nextGdnReleased || r.nextGdnCompleted || r.nextGdnHeld || r.nextGdnPublished)return false;
  auto& p=*projection;if(!PioneerQ38NextGdnLearned(*p.canonical,p.ledger,p.pages))return false;
  auto* s=new(std::nothrow) PioneerQ38NextGdnState;if(!s)return false;
  s->parent=nextConv;s->input=&nextConvolution;s->layer=1;s->epoch=nextConv->epoch+1;
  for(unsigned w=1;w<=96;w++)if(!s->jobs[w-1].Build(*p.canonical,*recurrent,w-1,p.bank,p.identity,p.cohort->authority.source[w],s->epoch)
   || !s->jobs[w-1].LearnedPagesPresent(p.ledger,p.pages) || !s->gated[w-1].Build(*p.canonical,s->jobs[w-1].Descriptor(),s->jobs[w-1].StateDescriptor())
   || !s->gated[w-1].LearnedPagePresent(p.ledger,p.pages)){delete s;return false;}
  s->built=true;nextGdn=s;complete=false;phase=37;r.phase=37; // consume genuine native60 BEFORE IO
  if(!recurrent->ClaimNextLayer(this,p.identity,1,s->epoch))return Fail(io,r);
  nextGdnClaimEpoch=s->epoch;
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
  for(unsigned w=1;w<=96;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<TT_Q38_GDN_RENDEZVOUS_IMAGE_WORDS;i++){r.word=i;if(!Write(io,w,PioneerQ38GdnRendezvousImageAddress(i),PioneerQ38GdnRendezvousImageWord(i)))return Fail(io,r);}
   if(!NextGdnImage(io,w))return Fail(io,r);
   s->image[w]=true;const uint32_t* tables[]={s->jobs[w-1].Descriptor(),s->jobs[w-1].StateDescriptor(),s->gated[w-1].Descriptor()};
   const unsigned addresses[]={TT_Q38_GDN_INPUT_DESCRIPTOR,TT_Q38_RGDN_DESCRIPTOR,TT_Q38_GDN_GATED_DESCRIPTOR};
   for(unsigned t=0;t<3;t++)for(unsigned i=0;i<64;i++){r.word=i;if(!Write(io,w,addresses[t]+i*4,tables[t][i]))return Fail(io,r);}
   const unsigned summaries[]={TT_Q38_GDN_INPUT_SUMMARY,TT_Q38_RGDN_SUMMARY,TT_Q38_GDN_GATED_SUMMARY};
   for(unsigned t=0;t<3;t++)for(unsigned i=0;i<32;i++)if(!Write(io,w,summaries[t]+i*4,0))return Fail(io,r);
   for(unsigned i=0;i<32;i++)if(!Write(io,w,TT_Q38_GDN_GATED_READY+i*4,0))return Fail(io,r);
   for(unsigned i=0;i<16;i++)if(!Write(io,w,TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4,0))return Fail(io,r);
   for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
   for(unsigned i=0;i<8;i++)s->before[w-1][i]=counts[w][i];
   s->staged[w]=true;if(!Guard(io,w) || !NextGdnCurrent(io,w))return Fail(io,r);
   r.nextGdnInstalled++;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
  phase=38;r.phase=38;
  for(unsigned w=1;w<=96;w++){r.worker=w;if(!Guard(io,w) || !NextGdnCurrent(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);r.nextGdnArmed++;}
  PioneerQ38HostDeadline deadline;if(!deadline.Begin(io.Q38CohortNow()))return Fail(io,r);
  for(unsigned w=1;w<=96;w++){
   r.worker=w;if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   if(!Guard(io,w) || !NextGdnCurrent(io,w))return Fail(io,r);
   if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;resets[w]=0x46800;io.RuntimeResetWrite(resets[w]);
   if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   if(!Guard(io,w) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   r.nextGdnReleased++;
  }
  phase=39;r.phase=39;bool done[96]={};unsigned finished=0;
  for(unsigned attempt=0;attempt<65536 && finished<96;attempt++){
   if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   for(unsigned w=1;w<=96;w++)if(!done[w-1]){
    r.worker=w;if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
    if(!Guard(io,w) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
    uint32_t marker=io.RuntimeStatusRead(2);
    if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
    if(marker && marker!=TT_Q38_GDN_RENDEZVOUS_MARKER)return Fail(io,r);
    if(marker){
     if(!NextGdnReceipt(io,w) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
     done[w-1]=true;finished++;r.nextGdnCompleted++;
    }else{
     io.Pause();if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
     uint32_t pc=io.RuntimePcRead();if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
     if(pc<0x64000 || pc>=0x64000+TT_Q38_GDN_RENDEZVOUS_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);
    }
   }io.Pause();if(!deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
  }
  if(finished!=96 || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
  phase=40;r.phase=40;
  for(unsigned w=0;w<120;w++){
   if(!deadline.Observe(io.Q38CohortNow()) || !Guard(io,w) || !Current(io,w)
    || ((!w || w>96) && (p.running[w] || resets[w]!=0x47800)) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
  }
  for(unsigned w=1;w<=96;w++){
   r.worker=w;if(!deadline.Observe(io.Q38CohortNow()) || !Guard(io,w) || !NextGdnTerminal(io)
    || !NextGdnCurrent(io,w) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();
   if(!deadline.Observe(io.Q38CohortNow()) || !Guard(io,w) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();
   if(!deadline.Observe(io.Q38CohortNow()) || !Guard(io,w) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;p.resets[w]=resets[w];r.nextGdnHeld++;
  }
  for(unsigned w=0;w<120;w++)if(!deadline.Observe(io.Q38CohortNow()) || p.running[w] || resets[w]!=0x47800
   || !Guard(io,w) || !Current(io,w) || !deadline.Observe(io.Q38CohortNow()))return Fail(io,r);
  if(!recurrent->PublishNextLayer(this,p.identity,1,s->epoch))return Fail(io,r);
  s->done=complete=true;phase=41;r.phase=41;auto& v=nextAttention;
  v.owner=this;v.state=s;v.parent=&nextConvolution;v.canonical=p.canonical;v.ledger=p.ledger;v.pages=p.pages;v.identity=p.identity;
  v.token=p.token;v.position=p.position;v.bank=p.bank;v.layer=1;v.phase=s->epoch;v.inputAddress=nextInput.normalizedAddress+TT_Q38_GDN_GATED_OUTPUT;v.residualAddress=nextInput.residualAddress;
  for(unsigned i=0;i<16;i++)v.source[i]=p.cohort->authority.source[0][i];
  if(!MatchesNextAttention(v))return Fail(io,r);
  r.nextGdnPublished=1;return true;
 }
private:
