/* SPDX-License-Identifier: MIT */
// Included INSIDE SAME actual decoder;claims genuine native58 input.
#if !defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || PIONEER_Q38_NEXT_PROJECTION_NATIVE != 1
#error Exact separate native59 next-layer projection role required
#endif
 friend class PioneerQ38NextProjectionProducer;
 PioneerQ38NextProjectionState* nextProjection=nullptr;
 PioneerQ38NextProjectionProducer nextProjections;
 static unsigned NextProjectionRole(unsigned slot){return slot==0?10:slot==1?11:slot==2?5:slot==3?6:14;}
 bool NextProjectionData()const{
  if(!nextProjection || !nextProjection->built || !nextRms || !nextRms->done || !nextRms->receipt || !NextRmsData()
   || nextProjection->parent!=nextRms || nextProjection->input!=&nextInput || nextInput.owner!=this || nextInput.state!=nextRms
   || nextInput.parent!=&ffnHidden || nextInput.canonical!=projection->canonical || nextInput.ledger!=projection->ledger
   || nextInput.pages!=projection->pages || !(nextInput.identity==projection->identity) || nextInput.token!=projection->token
   || nextInput.position!=projection->position || nextInput.bank!=projection->bank || nextInput.layer!=1 || nextProjection->layer!=1
   || nextInput.phase!=nextRms->epoch || nextProjection->epoch!=nextInput.phase+1
   || (phase!=27 && phase!=28 && phase!=29 && phase!=30 && phase!=31 && phase!=7 && phase!=8
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
    && !(nextConv && phase>=32 && phase<=36)
#endif
    )
   || nextProjection->slot>=4 || !nextProjection->graph)return false;
  const auto* m=nextProjection->graph->Manifest();
  if(!m || !PioneerQ38NextProjectionGraphFrame(m) || m[42]!=1 || m[43]!=NextProjectionRole(nextProjection->slot)
   || m[6]!=nextProjection->epoch+nextProjection->slot*2+(nextProjection->stopping?2U:0U)
   || m[4]!=nextInput.normalizedAddress || m[4]+0x18000!=nextInput.residualAddress || m[3]!=projection->bank
   || m[40]!=projection->position || m[41] || m[46]!=(nextProjection->stopping?0U:nextInput.phase) || m[47])return false;
  for(unsigned i=0;i<16;i++)if(nextInput.source[i]!=projection->cohort->authority.source[0][i] || m[16+i]!=nextInput.source[i])return false;
  return true; // private immutable canonical plans checked BEFORE EACH launch
 }
 bool NextProjectionCanonical()const{
  if(!NextProjectionData())return false;
  auto& s=*nextProjection;auto& p=*projection;
  for(unsigned i=0;i<4;i++)if(!s.jobs[i].Matches(*p.canonical,1,NextProjectionRole(i),p.position) || !s.jobs[i].LearnedPagesPresent(p.ledger,p.pages))return false;
  return s.graph->Matches(*p.canonical,s.jobs[s.slot],1,NextProjectionRole(s.slot),p.position,p.bank,p.identity,
   p.cohort->authority.source[0],s.sources,s.sequence,s.epoch+s.slot*2+(s.stopping?2U:0U),s.stopping?0:s.input->phase,s.stopping);
 }
 bool NextProjectionFrame(PioneerQ38NextProjectionState& s){
  auto* graph=new(std::nothrow) PioneerQ38NextProjectionGraphPlan;if(!graph)return false;
  auto& p=*projection;
  if(!graph->Build(*p.canonical,s.jobs[s.slot],s.layer,NextProjectionRole(s.slot),p.position,p.bank,p.identity,
   p.cohort->authority.source[0],s.sources,s.sequence,s.epoch+s.slot*2+(s.stopping?2U:0U),s.stopping?0:s.input->phase,s.stopping)){delete graph;return false;}
  delete s.graph;s.graph=graph;return true;
 }
 static uint32_t NextProjectionWord(unsigned w,uint32_t a){return PioneerQ38NextProjectionGraphProgramAddress(w,a)?PioneerQ38NextProjectionGraphProgramWord(w,a):0;}
 static bool NextProjectionExecutable(unsigned w,uint32_t a){
  if(w>=120 || (a&3))return false;
  if(a==0x6000)return true;
  if(!w)return (a>=0x64000 && a-0x64000<sizeof(PioneerQ38NextProjectionGraphRootPrefix))
   || (a>=0x68000 && a-0x68000<sizeof(PioneerQ38NextProjectionGraphRootGraph)) || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38NextProjectionGraphRootMain));
  return (a>=0x64000 && a-0x64000<sizeof(PioneerQ38ProjectionGraphPeerHelper))
   || (a>=0x70b00 && a-0x70b00<sizeof(PioneerQ38ProjectionGraphPeerAffine)) || (a>=0x72000 && a-0x72000<sizeof(PioneerQ38ProjectionGraphPeerMain));
 }
 template<class IO> bool NextProjectionImage(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a) && (!io.ProjectionGraphMemoryReadChecked(w,a,v) || v!=NextProjectionWord(w,a)))return false;
  }return true;
 }
 template<class IO> bool NextProjectionRootTerminal(IO& io){
  auto& s=*nextProjection;
  if(io.RuntimeStatusRead(2)!=TT_Q38_NEXT_PROJECTION_GRAPH_MARKER || io.RuntimeStatusRead(0)!=0x7200c
   || io.RuntimeStatusRead(1)!=(s.stopping?119U:s.jobs[s.slot].Outputs()))return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38RootTerminalPc(io.RuntimePcRead()))return false;}return true;
 }
 template<class IO> bool NextProjectionRootData(IO& io){
  const auto* m=nextProjection->graph->Manifest();
  for(unsigned i=0;i<TT_Q38_NEXT_PROJECTION_GRAPH_WORDS;i++){
   uint32_t a,b;if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST+i*4,a) || a!=m[i]
    || !io.ProjectionGraphMemoryReadChecked(0,TT_Q38_NEXT_PROJECTION_GRAPH_SHADOW+i*4,b) || b!=m[i])return false;
  }
  for(unsigned i=0;i<119*16;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_NEXT_PROJECTION_GRAPH_SOURCES+i*4,v) || v!=nextProjection->graph->Sources()[i])return false;}
  return true;
 }
 template<class IO> int NextProjectionPeer(IO& io,unsigned w,bool stop){
  unsigned marker=io.Q38PanelRead(0x7414c),seq=nextProjection->sequence[w-1];
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER){if(!seq && !stop && (marker==0 || marker==0xdeadbeef))return 0;return -1;}
  unsigned requested=io.Q38PanelRead(0x74140),finished=io.Q38PanelRead(0x74144),op=io.Q38PanelRead(0x74150);
  if(io.Q38PanelRead(0x74148) || finished>requested || requested>seq || finished>seq || (op && op!=(stop?2U:1U)))return -1;
  for(unsigned i=5;i<16;i++)if(io.Q38PanelRead(0x74140+i*4))return -1;
  bool terminal=true;for(unsigned i=0;i<2;i++){
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!NextProjectionExecutable(w,pc) || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):(pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH);
  }return requested==seq && finished==seq && op==(stop?2U:0U) && terminal?1:0;
 }
 template<class IO> bool NextProjectionPeerFrame(IO& io,unsigned w){
  for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74040+i*4)!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:i==2?PioneerQ38ProjectionGraphOwner::Route(w):i==3?projection->cohort->authority.source[w][0]:0))return false;
  return io.Q38PanelRead(0x74100)==nextProjection->sequence[w-1];
 }
 template<class IO> bool NextProjectionCurrent(IO& io,unsigned w){
  if(!NextProjectionData() || !NextProjectionImage(io,w))return false;
  if(!nextProjection->bootPrepared)return !projection->running[w] && resets[w]==0x47800;
  if(w)return NextProjectionPeer(io,w,nextProjection->stopped)==1 && NextProjectionPeerFrame(io,w);
  if(!nextProjection->rootProof)return !projection->running[0] && resets[0]==0x47800;
  if(!NextProjectionRootTerminal(io) || !NextProjectionRootData(io))return false;
  for(unsigned i=0;i<64;i++)if(io.Q38PanelRead(TT_Q38_NEXT_PROJECTION_GRAPH_STATS+i*4)!=nextProjection->accepted[i])return false;
  return true;
 }
 template<class IO> bool NextProjectionWait(IO& io,bool stop,PioneerQ38ResidentDecoderResult& r){
  bool done[119]={};unsigned finished=0;uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){r.worker=w;if(!Guard(io,w))return false;int value=NextProjectionPeer(io,w,stop);if(value<0)return false;
    if(value==1){if(!NextProjectionImage(io,w) || !NextProjectionPeerFrame(io,w))return false;verified[w]=true;if(!Guard(io,w))return false;
     done[w-1]=true;finished++;if(stop)r.nextProjectionStopped++;else if(phase==27)r.nextProjectionBooted++;}}
   io.Pause();
  }return finished==119;
 }
 template<class IO> bool NextProjectionProof(IO& io){
  if(!Guard(io,0) || !NextProjectionRootTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !NextProjectionImage(io,0) || !NextProjectionRootData(io))return false;
  auto& s=*nextProjection;const auto* m=s.graph->Manifest();uint32_t stats[64],after[8];
  for(unsigned i=0;i<64;i++)stats[i]=io.Q38PanelRead(TT_Q38_NEXT_PROJECTION_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  if(!PioneerQ38NextProjectionGraphReceipt(stats,m,s.before,after))return false;
  for(unsigned w=0;w<119;w++)if(io.Q38PanelRead(TT_Q38_NEXT_PROJECTION_GRAPH_SEQUENCE+w*4)!=PioneerQ38NextProjectionGraphNextPeer(m,w))return false;
  unsigned lastPeer=s.stopping?119:s.jobs[s.slot].Count();
  const uint32_t last[]={0xffb20228,0,PioneerQ38ProjectionGraphOwner::Route(lastPeer),0x75008,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<64;i++)s.accepted[i]=stats[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  verified[0]=s.rootProof=true;return Guard(io,0);
 }
 void NextProjectionAdvance(){
  auto& s=*nextProjection;const auto* m=s.graph->Manifest();
  for(unsigned w=1;w<120;w++){
   s.sequence[w-1]=PioneerQ38NextProjectionGraphNextPeer(m,w-1);if(s.stopping)continue;
   unsigned reads=0,flits=0,writes=0,writeFlits=0;
   if(w<=20){reads=1;flits=16;writes=2;writeFlits=10;
    const uint32_t last[]={0x90e00,0,PioneerQ38ProjectionGraphOwner::Route(w),m[4]+TT_Q38_PROJECTION_LINES_OFFSET+(w-1)*128,0,m[8],0,0x212,128,0};
    for(unsigned i=0;i<10;i++)command[w][i]=last[i];}
   if(w<=s.jobs[s.slot].Count()){
    const auto& part=*s.jobs[s.slot].Part(w-1);reads+=part.groups*160+2;flits+=part.groups*160*9+170;
    const uint32_t last[]={uint32_t(projection->canonical->Base+part.offset+(part.groups*160-1)*576),0,PioneerQ38ProjectionRoute(part.bank),0x90000,0,PioneerQ38ProjectionGraphOwner::Route(w),0,0,576,0};
    for(unsigned i=0;i<10;i++)command[w][i]=last[i];
   }
   for(unsigned i=0;i<8;i++)counts[w][i]+=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  }
 }
 template<class IO> bool NextProjectionExecute(IO& io,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;auto& s=*nextProjection;r.worker=0;
  if(!NextProjectionCanonical() || !Guard(io,0) || !NextProjectionImage(io,0)
   || !OutputStage(io,TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST,s.graph->Manifest(),TT_Q38_NEXT_PROJECTION_GRAPH_WORDS)
   || !OutputStage(io,TT_Q38_NEXT_PROJECTION_GRAPH_SOURCES,s.graph->Sources(),119*16))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  for(unsigned i=0;i<8;i++)s.before[i]=counts[0][i];
  s.rootProof=false;verified[0]=false;
  if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x87ffff;p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;
  io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned w=1;w<120;w++)verified[w]=false;
  resets[0]=0x46800;r.nextProjectionRuns++;io.RuntimeResetWrite(resets[0]);bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   unsigned marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_NEXT_PROJECTION_GRAPH_MARKER)return Fail(io,r);
   if(marker){if(!NextProjectionProof(io))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!NextProjectionExecutable(0,pc) || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!done)return Fail(io,r);
  NextProjectionAdvance();
  if(!NextProjectionWait(io,s.stopping,r) || !Guard(io,0) || !NextProjectionRootTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x47800;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;p.resets[0]=resets[0];return true;
 }
 bool MatchesNextProjections(const PioneerQ38NextProjectionProducer& value)const{
  if(&value!=&nextProjections || value.owner!=this || !nextProjection || !nextProjection->stopped || !nextProjection->rootProof || !nextProjection->bootPrepared || nextProjection->produced!=15
   || !complete || closed || Unsafe() || phase!=31 || !NextProjectionData() || !MatchesNativeOwner(*projection,*history)
   || value.state!=nextProjection || value.parent!=&nextInput || value.canonical!=projection->canonical || value.ledger!=projection->ledger
   || value.pages!=projection->pages || !(value.identity==projection->identity) || value.token!=projection->token || value.position!=projection->position
   || value.bank!=projection->bank || value.layer!=1 || value.phase!=nextProjection->graph->Manifest()[13]
   || value.resultAddress!=nextInput.normalizedAddress+TT_Q38_PROJECTION_RESULT_BASE || value.residualAddress!=nextInput.residualAddress)return false;
  for(unsigned i=0;i<16;i++)if(value.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800 || !verified[w] || !nextProjection->image[w])return false;
  return !projection->cohort->rootRunning;
 }
 bool MatchesNextProjectionCohort(const PioneerQ38NextProjectionProducer& value,const PioneerQ38DispatchLease& cohort)const{
  return MatchesNextProjections(value) && projection && projection->cohort==&cohort;
 }
public:
 bool NextProjectionsCompleted()const{return nextProjections.Current();}
 const PioneerQ38NextProjectionProducer* NextProjectionInput()const{return nextProjections.Current()?&nextProjections:nullptr;}
 template<class IO> bool RunNextProjections(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(nextProjection || nextProjections.owner || !nextInput.Current() || !nextRms || nextInput.phase>=0x7ffffff0
   || !projection || !history || !MatchesNativeOwner(*projection,*history) || r.nextProjectionInstalled || r.nextProjectionBooted
   || r.nextProjectionRuns || r.nextProjectionStopped || r.nextProjectionPublished)return false;
  auto* s=new(std::nothrow) PioneerQ38NextProjectionState;if(!s)return false;
  auto& p=*projection;s->parent=nextRms;s->input=&nextInput;s->epoch=nextInput.phase+1;s->layer=nextInput.layer;
  for(unsigned slot=0;slot<4;slot++)if(s->layer!=1 || !s->jobs[slot].Build(*p.canonical,s->layer,NextProjectionRole(slot),p.position)
   || !s->jobs[slot].LearnedPagesPresent(p.ledger,p.pages)){delete s;return false;}
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)s->sources[w][i]=p.cohort->authority.source[w+1][i];
  if(!NextProjectionFrame(*s)){delete s;return false;}s->built=true;
  nextProjection=s;phase=27;r.phase=27;complete=false; // actual private input expires BEFORE IO
  for(unsigned w=0;w<120;w++){r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);}
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i);if(!Write(io,w,a,NextProjectionWord(w,a)))return Fail(io,r);}
   if(!NextProjectionImage(io,w))return Fail(io,r);
   s->image[w]=true;r.nextProjectionInstalled++;
  }
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !NextProjectionCurrent(io,w))return Fail(io,r);
   const uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,PioneerQ38ProjectionGraphOwner::Route(w),p.cohort->authority.source[w][0],0,0,0,0};
   for(unsigned i=0;i<96;i++)if(!Write(io,w,0x74000+i*4,i>=16 && i<24?boot[i-16]:i>=80?0xdeadbeef:0))return Fail(io,r);
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?PioneerQ38ProjectionGraphOwner::Route(w):0;io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!Guard(io,w))return Fail(io,r);
  }
  s->bootPrepared=true;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);}
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x46800;verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;io.RuntimeResetWrite(resets[w]);if(!Guard(io,w))return Fail(io,r);}
  if(!NextProjectionWait(io,false,r))return Fail(io,r);
  phase=28;r.phase=28;
  for(unsigned slot=0;slot<4;slot++){
   if(slot){s->slot=slot;s->rootProof=false;if(!NextProjectionFrame(*s))return Fail(io,r);}
   if(!NextProjectionExecute(io,r))return false;
   s->produced|=1U<<slot;
  }
  s->stopping=true;s->rootProof=false;if(!NextProjectionFrame(*s))return Fail(io,r);phase=29;r.phase=29;
  if(!NextProjectionExecute(io,r))return false;
  s->stopped=true;phase=30;r.phase=30;
  // ALL119 independent STOPs already proven BEFORE first peer hold.
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !NextProjectionCurrent(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;p.resets[w]=resets[w];
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !NextProjectionCurrent(io,w))return Fail(io,r);
  complete=true;phase=31;r.phase=31;auto& value=nextProjections;
  value.owner=this;value.state=s;value.parent=&nextInput;value.canonical=p.canonical;value.ledger=p.ledger;value.pages=p.pages;value.identity=p.identity;
  value.token=p.token;value.position=p.position;value.bank=p.bank;value.layer=s->layer;value.phase=s->graph->Manifest()[13];
  value.resultAddress=nextInput.normalizedAddress+TT_Q38_PROJECTION_RESULT_BASE;value.residualAddress=nextInput.residualAddress;
  for(unsigned i=0;i<16;i++)value.source[i]=p.cohort->authority.source[0][i];
  if(!MatchesNextProjections(value))return Fail(io,r);
  r.nextProjectionPublished=1;return true;
 }
private:
