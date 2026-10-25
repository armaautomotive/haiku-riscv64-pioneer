/* SPDX-License-Identifier: MIT */
// Included INSIDE the SAME real resident decoder. Not a parallel owner,
// synthetic RMS epoch,public control,or standalone caller-ready facade.
#if !defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE != 1
#error Exact separate native54 output execution role required
#endif
 PioneerQ38GdnOutputExecutionState* output=nullptr; // retained on UNKNOWN
 bool OutputData()const{
  if(!output || !projection || !history || phase==12 || !gdnComplete || !gatedPublished || !GdnLayerCompleted()
   || output->jobs.producer!=&gatedOutput
   || output->bootstrap.input!=&output->jobs || output->bootstrap.cohort!=projection->cohort
   || gatedOutput.owner!=this || gatedOutput.canonical!=projection->canonical || gatedOutput.ledger!=projection->ledger
   || gatedOutput.pages!=projection->pages || !(gatedOutput.identity==projection->identity)
   || gatedOutput.token!=projection->token || gatedOutput.position!=projection->position || gatedOutput.bank!=projection->bank
   || gatedOutput.phase!=gdnPhase
   || !output->jobs.jobs.Matches(*projection->canonical,0,13,projection->position)
   || !output->jobs.jobs.LearnedPagesPresent(projection->ledger,projection->pages))return false;
  const auto* workspace=projection->canonical->Workspace(projection->bank);
  if(!workspace || workspace->bytes!=67108864 || workspace->allocated!=67108864
   || gatedOutput.inputAddress!=projection->canonical->Base+workspace->offset+TT_Q38_GDN_GATED_OUTPUT)return false;
  for(unsigned i=0;i<16;i++)if(gatedOutput.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=1;w<=96;w++)if(!gatedReceipted[w])return false;
  const auto* m=output->bootstrap.manifest;
  if(!PioneerQ38GdnOutputGraphManifest(m,output->bootstrap.sources))return false;
  uint32_t header[64];output->bootstrap.Header(header);
  if(output->stopping){header[5]=2;header[6]+=2;header[13]=header[6]+1;header[14]=0;header[41]=header[6];header[46]=header[47]=0;}
  for(unsigned i=0;i<64;i++)if(m[i]!=header[i])return false;
  for(unsigned w=0;w<119;w++){
   if(m[64+w]!=(output->stopping?(w<24?2U:1U):0U))return false;
   for(unsigned i=0;i<16;i++)if(output->bootstrap.sources[w*16+i]!=projection->cohort->authority.source[w+1][i])return false;
   const auto* p=output->jobs.jobs.Part(w);const auto* r=m+192+w*8;if(!p)return false;
   if(r[0]!=projection->canonical->Base+p->offset || r[1]!=p->bank || r[2]!=p->groups || r[3]!=p->firstRow
    || r[4]!=PioneerQ38ProjectionRoute(p->bank) || r[5] || r[6] || r[7])return false;
  }
  return true; // OWN live lineage,never old producer.Current after claim
 }
 template<class IO> bool OutputImage(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a)
    && (!io.ProjectionGraphMemoryReadChecked(w,a,v) || v!=PioneerQ38GdnOutputProgramWord(w,a)))return false;
  }
  return true;
 }
 template<class IO> bool OutputRootTerminal(IO& io){
  if(io.RuntimeStatusRead(2)!=TT_Q38_GDN_OUTPUT_GRAPH_MARKER || io.RuntimeStatusRead(0)!=0x7200c
   || io.RuntimeStatusRead(1)!=(output->stopping?119U:5120U))return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38RootTerminalPc(io.RuntimePcRead()))return false;}return true;
 }
 template<class IO> bool OutputRootData(IO& io){
  const auto* m=output->bootstrap.manifest;
  for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++){
   uint32_t a,b;if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+i*4,a) || a!=m[i]
    || !io.ProjectionGraphMemoryReadChecked(0,TT_Q38_PROJECTION_GRAPH_SHADOW+i*4,b) || b!=m[i])return false;
  }
  for(unsigned i=0;i<119*16;i++){
   uint32_t a,b,v=output->bootstrap.sources[i];
   if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_GDN_OUTPUT_GRAPH_SOURCES+i*4,a) || a!=v
    || !io.ProjectionGraphMemoryReadChecked(0,TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW+i*4,b) || b!=v)return false;
  }
  for(unsigned i=0;i<384;i++){
   uint32_t a,b,v=output->bootstrap.sources[i];
   if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_GDN_OUTPUT_GATHER_SOURCES+i*4,a) || a!=v
    || !io.ProjectionGraphMemoryReadChecked(0,TT_Q38_GDN_OUTPUT_GATHER_SHADOW+i*4,b) || b!=v)return false;
  }
  return true;
 }
 template<class IO> int OutputPeer(IO& io,unsigned w,bool stop){
  unsigned marker=io.Q38PanelRead(0x7414c),seq=output->sequence[w-1];
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER){
   if(!seq && !stop && (marker==0 || marker==0xdeadbeef))return 0;
   return -1;
  }
  unsigned requested=io.Q38PanelRead(0x74140),finished=io.Q38PanelRead(0x74144),op=io.Q38PanelRead(0x74150);
  if(io.Q38PanelRead(0x74148) || finished>requested || requested>seq || finished>seq || (op && op!=(stop?2U:1U)))return -1;
  for(unsigned i=5;i<16;i++)if(io.Q38PanelRead(0x74140+i*4))return -1;
  bool terminal=true;
  for(unsigned i=0;i<2;i++){
   io.Pause();uint32_t pc=io.RuntimePcRead();
   if(!PioneerQ38GdnOutputExecutable(w,pc) || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK
    || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):(pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH);
  }
  return requested==seq && finished==seq && op==(stop?2U:0U) && terminal?1:0;
 }
 template<class IO> bool OutputPeerFrame(IO& io,unsigned w){
  for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74040+i*4)!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:
   i==2?PioneerQ38ProjectionGraphOwner::Route(w):i==3?projection->cohort->authority.source[w][0]:0))return false;
  return io.Q38PanelRead(0x74100)==output->sequence[w-1];
 }
 template<class IO> bool OutputCurrent(IO& io,unsigned w){
  if(!OutputData() || !OutputImage(io,w))return false;
  if(!output->bootPrepared)return !projection->running[w] && resets[w]==0x47800;
  if(w)return OutputPeer(io,w,output->stopped)==1 && OutputPeerFrame(io,w);
  if(!output->rootProof)return !projection->running[0] && resets[0]==0x47800;
  if(!OutputRootTerminal(io) || !OutputRootData(io))return false;
  for(unsigned i=0;i<64;i++)if(io.Q38PanelRead(TT_Q38_PROJECTION_GRAPH_STATS+i*4)!=output->accepted[i])return false;
  return true;
 }
 template<class IO> bool OutputWaitPeers(IO& io,bool stop,PioneerQ38ResidentDecoderResult& r){
  bool done[119]={};unsigned finished=0;uint64_t began=io.Q38CohortNow();
  if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){
    r.worker=w;if(!Guard(io,w))return false;int value=OutputPeer(io,w,stop);if(value<0)return false;
    if(value==1){
     if(!OutputImage(io,w) || !OutputPeerFrame(io,w))return false;
     verified[w]=true;if(!Guard(io,w))return false;done[w-1]=true;finished++;
     if(stop)r.outputStopped++;else if(phase==13)r.outputBooted++;
    }
   }
   io.Pause();
  }
  return finished==119;
 }
 template<class IO> bool OutputStage(IO& io,uint32_t address,const uint32_t* data,unsigned words){
  for(unsigned i=0;i<words;i++)if(!Write(io,0,address+i*4,data[i]))return false;
  return true;
 }
 template<class IO> bool OutputProof(IO& io){
  if(!Guard(io,0) || !OutputRootTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !OutputImage(io,0) || !OutputRootData(io))return false;
  uint32_t stats[64],after[8];for(unsigned i=0;i<64;i++)stats[i]=io.Q38PanelRead(TT_Q38_PROJECTION_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  const auto* m=output->bootstrap.manifest;
  if(!PioneerQ38GdnOutputGraphReceipt(stats,m,output->before,after))return false;
  for(unsigned w=0;w<119;w++)if(io.Q38PanelRead(TT_Q38_PROJECTION_GRAPH_SEQUENCE+w*4)!=PioneerQ38ProjectionGraphNextPeer(m,w))return false;
  const uint32_t last[]={0xffb20228,0,PioneerQ38ProjectionGraphOwner::Route(119),0x75008,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<64;i++)output->accepted[i]=stats[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  verified[0]=output->rootProof=true;return Guard(io,0);
 }
 void OutputAdvance(){
  const auto* m=output->bootstrap.manifest;const auto& p=*projection;
  for(unsigned w=1;w<120;w++){
   output->sequence[w-1]=PioneerQ38ProjectionGraphNextPeer(m,w-1);if(output->stopping)continue;
   const auto& part=*output->jobs.jobs.Part(w-1);unsigned reads=part.groups*192+2,flits=part.groups*192*9+204,writes=0,writeFlits=0;
   if(w<=24){reads++;flits+=16;writes=2;writeFlits=10;}
   const uint32_t last[]={uint32_t(p.canonical->Base+part.offset+(part.groups*192-1)*576),0,
    PioneerQ38ProjectionRoute(part.bank),0x90000,0,PioneerQ38ProjectionGraphOwner::Route(w),0,0,576,0};
   for(unsigned i=0;i<10;i++)command[w][i]=last[i];
   for(unsigned i=0;i<8;i++)counts[w][i]+=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  }
 }
 template<class IO> bool OutputExecute(IO& io,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;auto* m=output->bootstrap.manifest;r.worker=0;
  if(!OutputData() || !Guard(io,0) || !OutputImage(io,0)
   || !OutputStage(io,TT_Q38_PROJECTION_GRAPH_MANIFEST,m,TT_Q38_PROJECTION_GRAPH_WORDS)
   || !OutputStage(io,TT_Q38_GDN_OUTPUT_GRAPH_SOURCES,output->bootstrap.sources,119*16))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  for(unsigned i=0;i<8;i++)output->before[i]=counts[0][i];
  output->rootProof=false;verified[0]=false;
  if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x87ffff;p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;
  io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  // Deadline starts BEFORE first release,not after a potentially slow MMIO.
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned w=1;w<120;w++)verified[w]=false;
  resets[0]=0x46800;r.outputRootRuns++;io.RuntimeResetWrite(resets[0]);bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();
   if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   unsigned marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_GDN_OUTPUT_GRAPH_MARKER)return Fail(io,r);
   if(marker){if(!OutputProof(io))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();
   if(!PioneerQ38GdnOutputExecutable(0,pc) || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!done)return Fail(io,r);
  OutputAdvance();
  if(!OutputWaitPeers(io,output->stopping,r) || !Guard(io,0) || !OutputRootTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x47800;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;return true;
 }
public:
 ~PioneerQ38ResidentDecoderOwner(){
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
  delete nextFfn;
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
  delete nextResidual;
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
  delete nextOutput;delete nextGdn;
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
  delete nextConv;
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
  delete nextProjection;
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
  delete nextRms;
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
  delete ffn;
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
  delete residual;
#endif
  delete output;
 } // UNKNOWN retains owner;normal Close already clears blocks
 bool GdnOutputCompleted()const{return output && output->done && output->stopped && complete && phase==16 && !Unsafe() && !closed;}
 template<class IO> bool RunGdnOutput(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(output || !gatedOutput.Current() || gatedOutput.phase>=0x7ffffffb || phase!=12 || !complete || !projection || !history || r.outputInstalled
   || r.outputBooted || r.outputRootRuns || r.outputStopped || r.outputPublished)return false;
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
  if(!gdnTimedReadbackDone || r.gdnValidationWorkers!=96 || r.gdnValidationWords!=829632)return false;
#endif
  auto* state=new(std::nothrow) PioneerQ38GdnOutputExecutionState;
  if(!state)return false;
  if(!state->jobs.Build(gatedOutput) || !state->bootstrap.Build(state->jobs,*projection->cohort)){delete state;return false;}
  // Claim BEFORE first IO/mutation. Old held-only producer/plan queries
  // expire NOW;the real SAME decoder's Scope/Guard owns every later step.
  output=state;phase=13;r.phase=13;complete=false;auto& p=*projection;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Scope(io) || !Guard(io,w) || !Current(io,w))return Fail(io,r);
  }
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
    r.word=i;uint32_t address=PioneerQ38ProjectionGraphCodeAddress(w,i);
    if(!Write(io,w,address,PioneerQ38GdnOutputProgramWord(w,address)))return Fail(io,r);
   }
   if(!OutputImage(io,w))return Fail(io,r);
   state->image[w]=true;r.outputInstalled++;
  }
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !OutputCurrent(io,w))return Fail(io,r);
   const uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,PioneerQ38ProjectionGraphOwner::Route(w),p.cohort->authority.source[w][0],0,0,0,0};
   for(unsigned i=0;i<96;i++)if(!Write(io,w,0x74000+i*4,i>=16 && i<24?boot[i-16]:i>=80?0xdeadbeef:0))return Fail(io,r);
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?PioneerQ38ProjectionGraphOwner::Route(w):0;
    io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!Guard(io,w))return Fail(io,r);
  }
  state->bootPrepared=true;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
  }
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x46800;verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;
   io.RuntimeResetWrite(resets[w]);if(!Guard(io,w))return Fail(io,r);
  }
  if(!OutputWaitPeers(io,false,r))return Fail(io,r);
  phase=14;r.phase=14;if(!OutputExecute(io,r))return false;state->done=true;
  // Known-clean ROOT held/ALL119 WAIT. STOP only,never discard a history.
  auto* m=state->bootstrap.manifest;state->stopping=true;m[5]=2;m[6]+=2;m[13]=m[6]+1;m[14]=0;m[41]=m[6];m[46]=m[47]=0;
  for(unsigned w=0;w<119;w++)m[64+w]=state->sequence[w];
  phase=15;r.phase=15;if(!OutputExecute(io,r))return false;
  // ALL119 independent terminals proven above BEFORE FIRST peer hold.
  state->stopped=true;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !OutputCurrent(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;p.resets[w]=resets[w];
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !OutputCurrent(io,w))return Fail(io,r);
  complete=true;phase=16;r.phase=16;r.outputPublished=1;return true;
 }
private:
