/* SPDX-License-Identifier: MIT */
// Included INSIDE the SAME real resident decoder. Not a parallel owner,
// synthetic RMS epoch,public control,or standalone caller-ready facade.
#if !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE != 1
#error Exact separate native62 next-layer output execution role required
#endif
 friend class PioneerQ38NextGdnOutputProducer;
 PioneerQ38NextGdnOutputState* nextOutput=nullptr;PioneerQ38NextGdnOutputProducer nextAttentionResult;
 bool nextOutputClaimed=false;unsigned nextOutputPublishedEpoch=0; // actual closed-owner lineage,never caller readiness
 template<class IO> bool NextOutputAlive(IO& io)const{return nextOutput && (!nextOutput->active || nextOutput->active->Observe(io.Q38CohortNow()));}
 template<class IO> bool NextOutputGuard(IO& io,unsigned w){return NextOutputAlive(io) && Guard(io,w) && NextOutputAlive(io);}
 template<class IO> bool NextOutputPanel(IO& io,uint32_t a,uint32_t& v){if(!NextOutputAlive(io))return false;v=io.Q38PanelRead(a);return NextOutputAlive(io);}
 template<class IO> bool NextOutputStatus(IO& io,unsigned a,uint32_t& v){if(!NextOutputAlive(io))return false;v=io.RuntimeStatusRead(a);return NextOutputAlive(io);}
 template<class IO> bool NextOutputPc(IO& io,uint32_t& v){if(!NextOutputAlive(io))return false;v=io.RuntimePcRead();return NextOutputAlive(io);}
 template<class IO> bool NextOutputPause(IO& io){if(!NextOutputAlive(io))return false;io.Pause();return NextOutputAlive(io);}
 template<class IO> bool NextOutputReset(IO& io,uint32_t v){if(!NextOutputAlive(io))return false;io.RuntimeResetWrite(v);return NextOutputAlive(io);}
 template<class IO> bool NextOutputMemory(IO& io,unsigned w,uint32_t a,uint32_t& v){return NextOutputAlive(io) && io.ProjectionGraphMemoryReadChecked(w,a,v) && NextOutputAlive(io);}
 template<class IO> bool NextOutputNoc(IO& io,unsigned a,uint32_t& v){if(!NextOutputAlive(io))return false;v=io.ResidentNocRead(a);return NextOutputAlive(io);}

 bool NextOutputData()const{
  if(!nextOutput || !projection || !history || !nextGdn || !nextGdn->done || !NextGdnData()
   || nextOutput->parent!=nextGdn || nextOutput->input!=&nextAttention || nextOutput->jobs.producer!=&nextAttention
   || nextOutput->bootstrap.input!=&nextOutput->jobs || nextOutput->bootstrap.cohort!=projection->cohort
   || nextAttention.owner!=this || nextAttention.state!=nextGdn || nextAttention.parent!=&nextConvolution
   || nextAttention.canonical!=projection->canonical || nextAttention.ledger!=projection->ledger || nextAttention.pages!=projection->pages
   || !(nextAttention.identity==projection->identity) || nextAttention.token!=projection->token || nextAttention.position || projection->position
   || nextAttention.bank!=projection->bank || nextAttention.layer!=1 || nextAttention.phase!=nextGdn->epoch
   || ((phase<42 || phase>46) && phase!=7 && phase!=8)
   || !nextOutput->jobs.jobs.Matches(*projection->canonical,1,13,0)
   || !nextOutput->jobs.jobs.LearnedPagesPresent(projection->ledger,projection->pages)
   || !PioneerQ38NextGdnOutputLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  const auto* workspace=projection->canonical->Workspace(projection->bank);
  if(!workspace || workspace->bytes!=67108864 || workspace->allocated!=67108864
   || nextAttention.inputAddress!=projection->canonical->Base+workspace->offset+TT_Q38_GDN_GATED_OUTPUT
   || nextAttention.residualAddress!=nextInput.residualAddress || nextAttention.residualAddress!=projection->canonical->Base+workspace->offset+0x18000)return false;
  for(unsigned i=0;i<16;i++)if(nextAttention.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=1;w<=96;w++)if(!nextGdn->receipt[w] || !nextGdn->staged[w] || !nextGdn->image[w])return false;
  const auto* m=nextOutput->bootstrap.manifest;
  if(!PioneerQ38NextGdnOutputGraphManifest(m,nextOutput->bootstrap.sources))return false;
  uint32_t header[64];nextOutput->bootstrap.Header(header);
  if(nextOutput->stopping){header[5]=2;header[6]+=2;header[13]=header[6]+1;header[14]=0;header[41]=header[6];header[46]=header[47]=0;}
  for(unsigned i=0;i<64;i++)if(m[i]!=header[i])return false;
  for(unsigned w=0;w<119;w++){
   if(m[64+w]!=(nextOutput->stopping?(w<24?2U:1U):0U))return false;
   for(unsigned i=0;i<16;i++)if(nextOutput->bootstrap.sources[w*16+i]!=projection->cohort->authority.source[w+1][i])return false;
   const auto* p=nextOutput->jobs.jobs.Part(w);const auto* r=m+192+w*8;if(!p)return false;
   if(r[0]!=projection->canonical->Base+p->offset || r[1]!=p->bank || r[2]!=p->groups || r[3]!=p->firstRow
    || r[4]!=PioneerQ38ProjectionRoute(p->bank) || r[5] || r[6] || r[7])return false;
  }
  return true; // immutable SAME native61 lineage,NOT expired producer.Current()
 }
 template<class IO> bool NextOutputImage(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a)
    && (!NextOutputMemory(io,w,a,v) || v!=PioneerQ38NextGdnOutputProgramWord(w,a)))return false;
  }
  return true;
 }
 template<class IO> bool NextOutputRootTerminal(IO& io){
  uint32_t marker,entry,count;
  if(!NextOutputStatus(io,2,marker) || marker!=TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER || !NextOutputStatus(io,0,entry) || entry!=0x7200c
   || !NextOutputStatus(io,1,count) || count!=(nextOutput->stopping?119U:5120U))return false;
  // Independently proven terminal/hold remains valid while held. Do not
  // sample a held core's live PC as though it were executing the image.
  if(!projection->running[0])return resets[0]==0x47800 && nextOutput->rootHeld && nextOutput->held[0] && nextOutput->rootProof;
  for(unsigned i=0;i<2;i++){uint32_t pc;if(!NextOutputPause(io) || !NextOutputPc(io,pc) || !PioneerQ38RootTerminalPc(pc))return false;}
  return NextOutputAlive(io);
 }
 template<class IO> bool NextOutputRootData(IO& io){
  const auto* m=nextOutput->bootstrap.manifest;
  for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++){
   uint32_t a,b;if(!NextOutputMemory(io,0,TT_Q38_PROJECTION_GRAPH_MANIFEST+i*4,a) || a!=m[i]
    || !NextOutputMemory(io,0,TT_Q38_PROJECTION_GRAPH_SHADOW+i*4,b) || b!=m[i])return false;
  }
  for(unsigned i=0;i<119*16;i++){
   uint32_t a,b,v=nextOutput->bootstrap.sources[i];
   if(!NextOutputMemory(io,0,TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCES+i*4,a) || a!=v
    || !NextOutputMemory(io,0,TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCE_SHADOW+i*4,b) || b!=v)return false;
  }
  for(unsigned i=0;i<384;i++){
   uint32_t a,b,v=nextOutput->bootstrap.sources[i];
   if(!NextOutputMemory(io,0,TT_Q38_NEXT_GDN_OUTPUT_GATHER_SOURCES+i*4,a) || a!=v
    || !NextOutputMemory(io,0,TT_Q38_NEXT_GDN_OUTPUT_GATHER_SHADOW+i*4,b) || b!=v)return false;
  }
  return true;
 }
 template<class IO> int NextOutputPeer(IO& io,unsigned w,bool stop){
  uint32_t marker,requested,finished,op,error,v;unsigned seq=nextOutput->sequence[w-1];
  if(!NextOutputPanel(io,0x7414c,marker))return -1;
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER){if(!seq && !stop && (marker==0 || marker==0xdeadbeef))return 0;return -1;}
  if(!NextOutputPanel(io,0x74140,requested) || !NextOutputPanel(io,0x74144,finished) || !NextOutputPanel(io,0x74150,op)
   || !NextOutputPanel(io,0x74148,error) || error || finished>requested || requested>seq || finished>seq || (op && op!=(stop?2U:1U)))return -1;
  for(unsigned i=5;i<16;i++)if(!NextOutputPanel(io,0x74140+i*4,v) || v)return -1;
  if(!projection->running[w]){
   return stop && nextOutput->stopped && nextOutput->stopReceipt[w] && nextOutput->held[w] && resets[w]==0x47800
    && requested==seq && finished==seq && op==2 ? 1 : -1;
  }
  bool terminal=true;
  for(unsigned i=0;i<2;i++){
   uint32_t pc;if(!NextOutputPause(io) || !NextOutputPc(io,pc) || !PioneerQ38NextGdnOutputExecutable(w,pc)
    || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):(pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH);
  }
  return requested==seq && finished==seq && op==(stop?2U:0U) && terminal?1:0;
 }
 template<class IO> bool NextOutputPeerFrame(IO& io,unsigned w){
  uint32_t v;for(unsigned i=0;i<8;i++)if(!NextOutputPanel(io,0x74040+i*4,v) || v!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:
   i==2?PioneerQ38ProjectionGraphOwner::Route(w):i==3?projection->cohort->authority.source[w][0]:0))return false;
  return NextOutputPanel(io,0x74100,v) && v==nextOutput->sequence[w-1];
 }
 template<class IO> bool NextOutputCurrent(IO& io,unsigned w){
  if(!NextOutputAlive(io) || !NextOutputData() || !NextOutputImage(io,w))return false;
  if(!nextOutput->bootPrepared)return !projection->running[w] && resets[w]==0x47800;
  if(w)return NextOutputPeer(io,w,nextOutput->stopped)==1 && NextOutputPeerFrame(io,w);
  if(!nextOutput->rootProof)return !projection->running[0] && resets[0]==0x47800;
  if(!NextOutputRootTerminal(io) || !NextOutputRootData(io))return false;
  for(unsigned i=0;i<64;i++){uint32_t v;if(!NextOutputPanel(io,TT_Q38_PROJECTION_GRAPH_STATS+i*4,v) || v!=nextOutput->accepted[i])return false;}
  return NextOutputAlive(io);
 }
 template<class IO> bool NextOutputWaitPeers(IO& io,bool stop,PioneerQ38ResidentDecoderResult& r){
  if(!nextOutput->active || !NextOutputAlive(io))return false;
  bool done[119]={};unsigned finished=0;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   if(!NextOutputAlive(io))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){
    r.worker=w;if(!NextOutputGuard(io,w))return false;
    int value=NextOutputPeer(io,w,stop);if(value<0)return false;
    if(value==1){
     if(!NextOutputImage(io,w) || !NextOutputPeerFrame(io,w))return false;
     verified[w]=true;if(!NextOutputGuard(io,w))return false;
     done[w-1]=true;finished++;if(stop){nextOutput->stopReceipt[w]=true;r.nextOutputStopped++;}
     else if(phase==42)r.nextOutputBooted++;
    }
   }
   if(!NextOutputPause(io))return false;
  }
  return finished==119 && NextOutputAlive(io);
 }
 template<class IO> bool NextOutputStage(IO& io,uint32_t address,const uint32_t* data,unsigned words){
  for(unsigned i=0;i<words;i++)if(!NextOutputAlive(io) || !Write(io,0,address+i*4,data[i]) || !NextOutputAlive(io))return false;
  return true;
 }
 template<class IO> bool NextOutputProof(IO& io){
  if(!NextOutputGuard(io,0) || !NextOutputRootTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !NextOutputAlive(io)
   || !NextOutputImage(io,0) || !NextOutputRootData(io))return false;
  uint32_t stats[64],after[8],v;
  for(unsigned i=0;i<64;i++)if(!NextOutputPanel(io,TT_Q38_PROJECTION_GRAPH_STATS+i*4,stats[i]))return false;
  for(unsigned i=0;i<8;i++)if(!NextOutputNoc(io,PioneerQ38PanelCounter(i),after[i]))return false;
  const auto* m=nextOutput->bootstrap.manifest;
  if(!PioneerQ38NextGdnOutputGraphReceipt(stats,m,nextOutput->before,after))return false;
  for(unsigned w=0;w<119;w++)if(!NextOutputPanel(io,TT_Q38_PROJECTION_GRAPH_SEQUENCE+w*4,v) || v!=PioneerQ38ProjectionGraphNextPeer(m,w))return false;
  const uint32_t last[]={0xffb20228,0,PioneerQ38ProjectionGraphOwner::Route(119),0x75008,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++)if(!NextOutputNoc(io,i*4,v) || v!=last[i])return false;
  for(unsigned i=0;i<64;i++)nextOutput->accepted[i]=stats[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  verified[0]=nextOutput->rootProof=true;return NextOutputGuard(io,0);
 }
 void NextOutputAdvance(){
  const auto* m=nextOutput->bootstrap.manifest;const auto& p=*projection;
  for(unsigned w=1;w<120;w++){
   nextOutput->sequence[w-1]=PioneerQ38ProjectionGraphNextPeer(m,w-1);if(nextOutput->stopping)continue;
   const auto& part=*nextOutput->jobs.jobs.Part(w-1);unsigned reads=part.groups*192+2,flits=part.groups*192*9+204,writes=0,writeFlits=0;
   if(w<=24){reads++;flits+=16;writes=2;writeFlits=10;}
   const uint32_t last[]={uint32_t(p.canonical->Base+part.offset+(part.groups*192-1)*576),0,
    PioneerQ38ProjectionRoute(part.bank),0x90000,0,PioneerQ38ProjectionGraphOwner::Route(w),0,0,576,0};
   for(unsigned i=0;i<10;i++)command[w][i]=last[i];
   for(unsigned i=0;i<8;i++)counts[w][i]+=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  }
 }
 template<class IO> bool NextOutputExecute(IO& io,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;auto& s=*nextOutput;auto* m=s.bootstrap.manifest;r.worker=0;
  if(s.active || !NextOutputData() || !NextOutputGuard(io,0) || !NextOutputImage(io,0)
   || !NextOutputStage(io,TT_Q38_PROJECTION_GRAPH_MANIFEST,m,TT_Q38_PROJECTION_GRAPH_WORDS)
   || !NextOutputStage(io,TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCES,s.bootstrap.sources,119*16))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);uint32_t v;if(!NextOutputStatus(io,i,v) || v)return Fail(io,r);}
  for(unsigned i=0;i<8;i++)s.before[i]=counts[0][i];
  s.rootProof=s.rootHeld=s.held[0]=false;verified[0]=false;
  if(!NextOutputGuard(io,0))return Fail(io,r);
  resets[0]=0x87ffff;p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;
  if(!NextOutputReset(io,resets[0]) || !NextOutputPause(io) || !NextOutputGuard(io,0))return Fail(io,r);
  s.active=s.stopping?&s.stopDeadline:&s.runDeadline;
  if(!s.active->Begin(io.Q38CohortNow()) || !NextOutputAlive(io))return Fail(io,r);
  for(unsigned w=1;w<120;w++)verified[w]=false;
  resets[0]=0x46800;
  if(!NextOutputReset(io,resets[0]) || !NextOutputGuard(io,0))return Fail(io,r);
  r.nextOutputRootRuns++;bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!NextOutputGuard(io,0))return Fail(io,r);
   uint32_t marker;if(!NextOutputStatus(io,2,marker) || (marker && marker!=TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER))return Fail(io,r);
   if(marker){if(!NextOutputProof(io))return Fail(io,r);done=true;break;}
   uint32_t pc;if(!NextOutputPause(io) || !NextOutputPc(io,pc) || !PioneerQ38NextGdnOutputExecutable(0,pc)
    || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!done || !NextOutputAlive(io))return Fail(io,r);
  NextOutputAdvance();
  if(!NextOutputWaitPeers(io,s.stopping,r) || !NextOutputGuard(io,0) || !NextOutputRootTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;if(!NextOutputReset(io,resets[0]) || !NextOutputPause(io) || !NextOutputGuard(io,0))return Fail(io,r);
  resets[0]=0x47800;if(!NextOutputReset(io,resets[0]) || !NextOutputPause(io) || !NextOutputGuard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;p.resets[0]=resets[0];
  s.rootHeld=s.held[0]=true;
  if(!NextOutputAlive(io))return Fail(io,r);
  if(!s.stopping)s.active=nullptr; // one-shot RUN interval ended;STOP has its OWN nonrenewable deadline
  return true;
 }
 bool MatchesNextOutput(const PioneerQ38NextGdnOutputProducer& v)const{
  if(&v!=&nextAttentionResult || v.owner!=this || !nextOutput || !nextOutput->done || !nextOutput->stopped || !nextOutput->rootProof
   || !nextOutput->rootHeld || !complete || closed || Unsafe() || phase!=46 || !NextOutputData()
   || !nextOutputClaimed || !nextOutputPublishedEpoch || nextOutputPublishedEpoch!=nextGdnClaimEpoch+3
   || !MatchesNativeOwner(*projection,*history) || v.state!=nextOutput || v.parent!=&nextAttention
   || v.canonical!=projection->canonical || v.ledger!=projection->ledger || v.pages!=projection->pages || !(v.identity==projection->identity)
   || v.token!=projection->token || v.position || v.bank!=projection->bank || v.layer!=1 || v.phase!=nextOutputPublishedEpoch
   || v.resultAddress!=nextInput.normalizedAddress+TT_Q38_NEXT_GDN_OUTPUT_RESULT || v.residualAddress!=nextInput.residualAddress)return false;
  for(unsigned i=0;i<16;i++)if(v.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800 || !verified[w]
   || !nextOutput->image[w] || !nextOutput->held[w] || (w && !nextOutput->stopReceipt[w]))return false;
  return !projection->cohort->rootRunning;
 }
 bool MatchesNextOutputCohort(const PioneerQ38NextGdnOutputProducer& v,const PioneerQ38DispatchLease& c)const{return MatchesNextOutput(v) && projection->cohort==&c;}
public:
 bool NextGdnOutputCompleted()const{return nextAttentionResult.Current();}
 const PioneerQ38NextGdnOutputProducer* NextPostAttentionInput()const{return nextAttentionResult.Current()?&nextAttentionResult:nullptr;}
 template<class IO> bool RunNextGdnOutput(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(nextOutput || nextAttentionResult.owner || !nextAttention.Current() || nextAttention.phase>=0x7ffffffb || phase!=41 || !complete
   || !projection || !history || !nextGdn || !MatchesNativeOwner(*projection,*history) || r.nextOutputInstalled
   || r.nextOutputBooted || r.nextOutputRootRuns || r.nextOutputStopped || r.nextOutputHeld || r.nextOutputPublished
   || !PioneerQ38NextGdnOutputLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  auto* state=new(std::nothrow) PioneerQ38NextGdnOutputState;
  if(!state)return false;
  if(!state->jobs.Build(nextAttention) || !state->bootstrap.Build(state->jobs,*projection->cohort)){delete state;return false;}
  // Claim BEFORE first IO/mutation. Old held-only producer/plan queries
  // expire NOW;the real SAME decoder's Scope/Guard owns every later step.
  state->parent=nextGdn;state->input=&nextAttention;nextOutput=state;nextOutputClaimed=true;phase=42;r.phase=42;complete=false;auto& p=*projection;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Scope(io) || !NextOutputGuard(io,w) || !Current(io,w))return Fail(io,r);
  }
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!NextOutputGuard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
    r.word=i;uint32_t address=PioneerQ38ProjectionGraphCodeAddress(w,i);
    if(!Write(io,w,address,PioneerQ38NextGdnOutputProgramWord(w,address)))return Fail(io,r);
   }
   if(!NextOutputImage(io,w))return Fail(io,r);
   state->image[w]=true;r.nextOutputInstalled++;
  }
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!NextOutputGuard(io,w) || !NextOutputCurrent(io,w))return Fail(io,r);
   const uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,PioneerQ38ProjectionGraphOwner::Route(w),p.cohort->authority.source[w][0],0,0,0,0};
   for(unsigned i=0;i<96;i++)if(!Write(io,w,0x74000+i*4,i>=16 && i<24?boot[i-16]:i>=80?0xdeadbeef:0))return Fail(io,r);
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?PioneerQ38ProjectionGraphOwner::Route(w):0;
    io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!NextOutputGuard(io,w))return Fail(io,r);
  }
  state->bootPrepared=true;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!NextOutputGuard(io,w))return Fail(io,r);resets[w]=0x87ffff;if(!NextOutputReset(io,resets[w]) || !NextOutputPause(io) || !NextOutputGuard(io,w))return Fail(io,r);
  }
  state->active=&state->bootDeadline;
  if(!state->bootDeadline.Begin(io.Q38CohortNow()) || !NextOutputAlive(io))return Fail(io,r);
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!NextOutputGuard(io,w))return Fail(io,r);resets[w]=0x46800;verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;
   if(!NextOutputReset(io,resets[w]) || !NextOutputGuard(io,w))return Fail(io,r);
  }
  if(!NextOutputWaitPeers(io,false,r) || !NextOutputAlive(io))return Fail(io,r);
  state->active=nullptr;
  phase=43;r.phase=43;if(!NextOutputExecute(io,r))return false;state->done=true;
  // Known-clean ROOT held/ALL119 WAIT. STOP only,never discard a history.
  auto* m=state->bootstrap.manifest;state->stopping=true;m[5]=2;m[6]+=2;m[13]=m[6]+1;m[14]=0;m[41]=m[6];m[46]=m[47]=0;
  for(unsigned w=0;w<119;w++)m[64+w]=state->sequence[w];
  phase=44;r.phase=44;if(!NextOutputExecute(io,r))return false;
  // ALL119 independent terminals proven above BEFORE FIRST peer hold.
  state->stopped=true;phase=45;r.phase=45;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!NextOutputGuard(io,w) || !NextOutputCurrent(io,w))return Fail(io,r);
   resets[w]=0x87ffff;if(!NextOutputReset(io,resets[w]) || !NextOutputPause(io) || !NextOutputGuard(io,w))return Fail(io,r);
   resets[w]=0x47800;if(!NextOutputReset(io,resets[w]) || !NextOutputPause(io) || !NextOutputGuard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;p.resets[w]=resets[w];state->held[w]=true;r.nextOutputHeld++;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !NextOutputGuard(io,w) || !NextOutputCurrent(io,w))return Fail(io,r);
  if(!NextOutputAlive(io))return Fail(io,r);
  complete=true;phase=46;r.phase=46;nextOutputPublishedEpoch=state->bootstrap.manifest[13];auto& v=nextAttentionResult;
  v.owner=this;v.state=state;v.parent=&nextAttention;v.canonical=p.canonical;v.ledger=p.ledger;v.pages=p.pages;v.identity=p.identity;
  v.token=p.token;v.position=p.position;v.bank=p.bank;v.layer=1;v.phase=nextOutputPublishedEpoch;
  v.resultAddress=nextInput.normalizedAddress+TT_Q38_NEXT_GDN_OUTPUT_RESULT;v.residualAddress=nextInput.residualAddress;
  for(unsigned i=0;i<16;i++)v.source[i]=p.cohort->authority.source[0][i];
  if(!MatchesNextOutput(v) || !NextOutputAlive(io))return Fail(io,r);
  state->active=nullptr;r.nextOutputPublished=1;return true;
 }
private:
