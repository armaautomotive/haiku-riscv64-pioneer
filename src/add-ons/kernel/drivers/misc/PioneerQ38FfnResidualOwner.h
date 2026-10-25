/* SPDX-License-Identifier: MIT */
// Included INSIDE SAME actual decoder. No parallel FfnMutableBanks/code/
// clock/host owner,synthetic RMS epoch or caller ready-vector grant.
#if !defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || PIONEER_Q38_FFN_RESIDUAL_NATIVE != 1
#error Exact separate native57 SAME decoder FFN+residual execution required
#endif
 friend class PioneerQ38FfnHiddenProducer;
 PioneerQ38FfnResidualState* ffn=nullptr;
 PioneerQ38FfnHiddenProducer ffnHidden;
 bool FfnData()const{
  if(!ffn || !ffn->built || !projection || !residual || !residual->done || !residual->receipt || !ResidualData()
   || ffn->parent!=residual || ffn->input!=&postAttention || postAttention.owner!=this || postAttention.state!=residual
   || postAttention.canonical!=projection->canonical || postAttention.ledger!=projection->ledger || postAttention.pages!=projection->pages
   || !(postAttention.identity==projection->identity) || postAttention.token!=projection->token || postAttention.position!=projection->position
   || postAttention.bank!=projection->bank || postAttention.phase!=residual->epoch || ffn->epoch!=residual->epoch+1
   || (phase!=19 && phase!=20 && phase!=21 && phase!=22 && phase!=24 && phase!=7 && phase!=8
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
    && !(nextRms && (phase==25 || phase==26))
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
    && !(nextProjection && phase>=27 && phase<=31)
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
    && !(nextConv && phase>=32 && phase<=36)
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
    && !(nextGdn && phase>=37 && phase<=41)
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
    && !(nextOutput && phase>=42 && phase<=46)
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
    && !(nextResidual && phase>=47 && phase<=49)
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
    && !NextFfnRetainedPhase()
#endif
    ))return false;
  const auto* m=ffn->manifest;
  if(!PioneerQ38FfnResidualGraphFrame(m) || m[6]!=ffn->epoch+(ffn->stopping?5U:0U)
   || m[4]!=postAttention.normalizedAddress || m[4]+0x18000!=postAttention.residualAddress || m[3]!=projection->bank
   || m[40]!=projection->position || m[41] || m[42])return false;
  const uint64_t id[]={projection->identity.session,projection->identity.sequence,projection->identity.modelEpoch,projection->identity.driverGeneration};
  for(unsigned i=0;i<4;i++)if(m[32+i*2]!=uint32_t(id[i]) || m[33+i*2]!=uint32_t(id[i]>>32))return false;
  for(unsigned i=0;i<16;i++)if(m[16+i]!=postAttention.source[i] || m[16+i]!=projection->cohort->authority.source[0][i])return false;
  // Private jobs/manifest/source storage is never exposed or writable by
  // callers. Full canonical roles/pages and shape checked at construction
  // and BEFORE EACH launch,not an O(all-pages) CPU scan per codeword. SAME
  // native call lock/source/ledger/host/clock scope remains mandatory.
  return true;
 }
 bool FfnCanonical()const{
  if(!FfnData() || !ffn->jobs.Matches(*projection->canonical,0,projection->position)
   || !ffn->jobs.LearnedPagesPresent(projection->ledger,projection->pages)
   || !PioneerQ38FfnResidualGraphManifest(ffn->manifest,ffn->sources))return false;
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)if(ffn->sources[w*16+i]!=projection->cohort->authority.source[w+1][i])return false;
  return true;
 }
 bool BuildFfn(PioneerQ38FfnResidualState& s){
  auto& p=*projection;const auto* workspace=p.canonical->Workspace(p.bank);
  if(!workspace || workspace->bytes!=67108864 || workspace->allocated!=67108864 || p.position
   || postAttention.phase>=0x7ffffff8 || !s.jobs.Build(*p.canonical,0,p.position)
   || !s.jobs.LearnedPagesPresent(p.ledger,p.pages))return false;
  s.input=&postAttention;s.parent=residual;s.epoch=postAttention.phase+1;
  auto* m=s.manifest;m[0]=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER;m[1]=3;m[2]=67108864;m[3]=p.bank;m[4]=p.canonical->Base+workspace->offset;
  m[5]=1;m[6]=s.epoch;m[7]=3;m[8]=PioneerQ38SpanRoute(p.bank);m[9]=129;m[10]=TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+256;
  m[11]=4352;m[12]=TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+768;m[13]=s.epoch+5;m[14]=1;
  for(unsigned i=0;i<16;i++)m[16+i]=p.cohort->authority.source[0][i];
  const uint64_t id[]={p.identity.session,p.identity.sequence,p.identity.modelEpoch,p.identity.driverGeneration};
  for(unsigned i=0;i<4;i++){m[32+i*2]=uint32_t(id[i]);m[33+i*2]=uint32_t(id[i]>>32);}
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)s.sources[w*16+i]=p.cohort->authority.source[w+1][i];
  for(unsigned role=0;role<3;role++){
   const auto* job=s.jobs.Job(role);if(!job || job->peers!=119 || job->type!=12)return false;
   auto* r=m+PioneerQ38FfnResidualGraphRoleOffset(role);r[0]=role;r[1]=12;r[2]=119;r[3]=role==2?17408:5120;r[4]=role==2?5120:17408;
   for(unsigned w=0;w<119;w++){auto* part=r+16+w*8;part[0]=uint32_t(p.canonical->Base+job->part[w].offset);
    part[1]=job->part[w].bank;part[2]=job->part[w].groups;part[4]=PioneerQ38SpanRoute(part[1]);part[5]=s.sources[w*16];
    r[16+119*8+w]=job->first[w];}
  }
  if(!PioneerQ38FfnResidualGraphManifest(m,s.sources) || m[4]!=postAttention.normalizedAddress || m[4]+0x18000!=postAttention.residualAddress)return false;
  s.built=true;return true; // DATA derived from actual private producer;not claimed yet
 }
 static uint32_t FfnWord(unsigned w,uint32_t address){
  // Fill old larger GDN peer-prefix tail with ZERO within SAME saved lease.
  return PioneerQ38FfnGraphProgramAddress(w,address)?PioneerQ38FfnResidualProgramWord(w,address):0;
 }
 template<class IO> bool FfnImage(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a)
    && (!io.ProjectionGraphMemoryReadChecked(w,a,v) || v!=FfnWord(w,a)))return false;
  }
  return true;
 }
 template<class IO> int FfnPeer(IO& io,unsigned w,bool stop){
  unsigned marker=io.Q38PanelRead(0x7414c),seq=ffn->sequence[w-1];
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER){if(!seq && !stop && (marker==0 || marker==0xdeadbeef))return 0;return -1;}
  unsigned requested=io.Q38PanelRead(0x74140),finished=io.Q38PanelRead(0x74144),op=io.Q38PanelRead(0x74150);
  if(io.Q38PanelRead(0x74148) || finished>requested || requested>seq || finished>seq || (op && op!=(stop?2U:1U)))return -1;
  for(unsigned i=5;i<16;i++)if(io.Q38PanelRead(0x74140+i*4))return -1;
  bool terminal=true;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();
   if(!PioneerQ38FfnResidualExecutableAddress(w,pc) || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK
    || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):(pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH);
  }
  return requested==seq && finished==seq && op==(stop?2U:0U) && terminal?1:0;
 }
 template<class IO> bool FfnPeerFrame(IO& io,unsigned w){
  for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74040+i*4)!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:
   i==2?PioneerQ38ProjectionGraphOwner::Route(w):i==3?projection->cohort->authority.source[w][0]:0))return false;
  if(ffn->stopped)for(unsigned i=0;i<16;i++)if(io.Q38PanelRead(0x74000+i*4)!=(i==10?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==11?w:i==12?ffn->sequence[w-1]:i==13?2U:0U))return false;
  return io.Q38PanelRead(0x74100)==ffn->sequence[w-1];
 }
 template<class IO> bool FfnRootTerminal(IO& io){
  if(io.RuntimeStatusRead(2)!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER || io.RuntimeStatusRead(0)!=0x7200c || io.RuntimeStatusRead(1)!=(ffn->stopping?119U:5120U))return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38RootTerminalPc(io.RuntimePcRead()))return false;}return true;
 }
 template<class IO> bool FfnRootData(IO& io){
  for(unsigned i=0;i<TT_Q38_FFN_RESIDUAL_GRAPH_WORDS;i++){
   uint32_t a,b;if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+i*4,a) || a!=ffn->manifest[i]
    || !io.ProjectionGraphMemoryReadChecked(0,TT_Q38_FFN_RESIDUAL_GRAPH_SHADOW+i*4,b) || b!=ffn->manifest[i])return false;
  }
  for(unsigned i=0;i<119*16;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(0,0x7d800+i*4,v) || v!=ffn->sources[i])return false;}
  return true;
 }
 template<class IO> bool FfnCurrent(IO& io,unsigned w){
  if(!FfnData() || !FfnImage(io,w))return false;
  if(!ffn->bootPrepared)return !projection->running[w] && resets[w]==0x47800;
  if(w)return FfnPeer(io,w,ffn->stopped)==1 && FfnPeerFrame(io,w);
  if(!ffn->rootProof)return !projection->running[0] && resets[0]==0x47800;
  if(!FfnRootTerminal(io) || !FfnRootData(io))return false;
  for(unsigned i=0;i<64;i++)if(io.Q38PanelRead(TT_Q38_FFN_RESIDUAL_GRAPH_STATS+i*4)!=ffn->accepted[i])return false;
  return true;
 }
 template<class IO> bool FfnWaitPeers(IO& io,bool stop,PioneerQ38ResidentDecoderResult& r){
  bool done[119]={};unsigned finished=0;uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){r.worker=w;if(!Guard(io,w))return false;int value=FfnPeer(io,w,stop);if(value<0)return false;
    if(value==1){if(!FfnImage(io,w) || !FfnPeerFrame(io,w))return false;verified[w]=true;if(!Guard(io,w))return false;
     done[w-1]=true;finished++;if(stop)r.ffnStopped++;else if(phase==19)r.ffnBooted++;}}
   io.Pause();
  }
  return finished==119;
 }
 template<class IO> bool FfnProof(IO& io){
  if(!Guard(io,0) || !FfnRootTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !FfnImage(io,0) || !FfnRootData(io))return false;
  uint32_t stats[64],after[8];for(unsigned i=0;i<64;i++)stats[i]=io.Q38PanelRead(TT_Q38_FFN_RESIDUAL_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  const auto* m=ffn->manifest;if(!PioneerQ38FfnResidualGraphReceipt(stats,m,ffn->before,after))return false;
  for(unsigned w=0;w<119;w++)if(io.Q38PanelRead(TT_Q38_FFN_RESIDUAL_GRAPH_SEQUENCE+w*4)!=PioneerQ38FfnResidualGraphNextPeer(m,w))return false;
  uint32_t last[10]={};
  if(ffn->stopping){last[0]=0xffb20228;last[2]=PioneerQ38ProjectionGraphOwner::Route(119);last[3]=0x75008;last[5]=129;last[8]=4;}
  else{last[0]=0x9d000;last[2]=129;last[3]=m[4]+16384;last[5]=m[8];last[7]=0x212;last[8]=4096;}
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<64;i++)ffn->accepted[i]=stats[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  verified[0]=ffn->rootProof=true;return Guard(io,0);
 }
 void FfnAdvance(){
  const auto* m=ffn->manifest;
  for(unsigned w=1;w<120;w++){
   ffn->sequence[w-1]=PioneerQ38FfnResidualGraphNextPeer(m,w-1);if(ffn->stopping)continue;
   unsigned reads=0,flits=0,writes=0,writeFlits=0;
   if(w<=20){reads++;flits+=16;writes+=2;writeFlits+=9;}
   if(w<=68){reads+=2;flits+=32;writes+=2;writeFlits+=9;}
   for(unsigned role=0;role<3;role++){
    const auto* r=m+PioneerQ38FfnResidualGraphRoleOffset(role);const auto* part=r+16+(w-1)*8;unsigned k=r[3],groups=part[2],blocks=k/256;
    reads+=groups*blocks+(k*2+16383)/16384+1;flits+=groups*blocks*36+(k*2+((blocks*4+63)&~63U))/64;
    if(role==2){const uint32_t last[]={part[0]+(groups*blocks-1)*2304,0,part[4],0x90000,0,PioneerQ38ProjectionGraphOwner::Route(w),0,0,2304,0};
     for(unsigned i=0;i<10;i++)command[w][i]=last[i];}
   }
   for(unsigned i=0;i<8;i++)counts[w][i]+=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  }
 }
 template<class IO> bool FfnExecute(IO& io,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;r.worker=0;
  if(!FfnCanonical() || !Guard(io,0) || !FfnImage(io,0)
   || !OutputStage(io,TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST,ffn->manifest,TT_Q38_FFN_RESIDUAL_GRAPH_WORDS)
   || !OutputStage(io,0x7d800,ffn->sources,119*16))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  for(unsigned i=0;i<8;i++)ffn->before[i]=counts[0][i];
  ffn->rootProof=false;verified[0]=false;
  if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x87ffff;p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;
  io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned w=1;w<120;w++)verified[w]=false;
  resets[0]=0x46800;r.ffnRootRuns++;io.RuntimeResetWrite(resets[0]);bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   unsigned marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER)return Fail(io,r);
   if(marker){if(!FfnProof(io))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!PioneerQ38FfnResidualExecutableAddress(0,pc) || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!done)return Fail(io,r);
  FfnAdvance();
  if(!FfnWaitPeers(io,ffn->stopping,r) || !Guard(io,0) || !FfnRootTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x47800;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;p.resets[0]=resets[0];return true;
 }
 bool MatchesFfnHidden(const PioneerQ38FfnHiddenProducer& value)const{
  if(&value!=&ffnHidden || value.owner!=this || !ffn || !ffn->done || !ffn->stopped || !ffn->rootProof || !complete || closed || Unsafe()
   || phase!=24 || !FfnData() || !MatchesNativeOwner(*projection,*history) || value.parent!=&postAttention || value.state!=ffn
   || value.canonical!=projection->canonical || value.ledger!=projection->ledger || value.pages!=projection->pages
   || !(value.identity==projection->identity) || value.token!=projection->token || value.position!=projection->position
   || value.bank!=projection->bank || value.phase!=ffn->epoch+5 || value.nextLayer!=1 || value.hiddenAddress!=ffn->manifest[4])return false;
  for(unsigned i=0;i<16;i++)if(value.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800 || !verified[w] || !ffn->image[w])return false;
  return !projection->cohort->rootRunning;
 }
 bool PublishFfnHidden(){
  if(ffnHidden.owner || !ffn || !ffn->done || !ffn->stopped || !projection || phase!=24 || !complete)return false;
  auto& p=*projection;ffnHidden.owner=this;ffnHidden.canonical=p.canonical;ffnHidden.parent=&postAttention;ffnHidden.state=ffn;
  ffnHidden.ledger=p.ledger;ffnHidden.pages=p.pages;ffnHidden.identity=p.identity;ffnHidden.token=p.token;ffnHidden.position=p.position;
  ffnHidden.bank=p.bank;ffnHidden.phase=ffn->epoch+5;ffnHidden.nextLayer=1;ffnHidden.hiddenAddress=ffn->manifest[4];
  for(unsigned i=0;i<16;i++)ffnHidden.source[i]=p.cohort->authority.source[0][i];
  return MatchesFfnHidden(ffnHidden);
 }
 bool MatchesFfnCohort(const PioneerQ38FfnHiddenProducer& value,const PioneerQ38DispatchLease& cohort)const{
  return MatchesFfnHidden(value) && projection && projection->cohort==&cohort;
 }
public:
 bool FfnCompleted()const{return ffnHidden.Current();}
 const PioneerQ38FfnHiddenProducer* FfnHiddenInput()const{return ffnHidden.Current()?&ffnHidden:nullptr;}
 template<class IO> bool RunFfn(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(ffn || ffnHidden.owner || !postAttention.Current() || !projection || !history || !MatchesNativeOwner(*projection,*history)
   || r.ffnInstalled || r.ffnBooted || r.ffnRootRuns || r.ffnStopped || r.ffnPublished)return false;
  auto* state=new(std::nothrow) PioneerQ38FfnResidualState;if(!state)return false;
  if(!BuildFfn(*state)){delete state;return false;}
  // Claim actual normalized AND residual lineages BEFORE FIRST IO. Old
  // completion query expires;SAME original snapshot/clock/host owners stay.
  ffn=state;phase=19;r.phase=19;complete=false;auto& p=*projection;
  if(!FfnCanonical())return Fail(io,r);
  for(unsigned w=0;w<120;w++){r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);}
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
    r.word=i;uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i);if(!Write(io,w,a,FfnWord(w,a)))return Fail(io,r);
   }
   if(!FfnImage(io,w))return Fail(io,r);
   state->image[w]=true;r.ffnInstalled++;
  }
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !FfnCurrent(io,w))return Fail(io,r);
   const uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,PioneerQ38ProjectionGraphOwner::Route(w),p.cohort->authority.source[w][0],0,0,0,0};
   for(unsigned i=0;i<96;i++)if(!Write(io,w,0x74000+i*4,i>=16 && i<24?boot[i-16]:i>=80?0xdeadbeef:0))return Fail(io,r);
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?PioneerQ38ProjectionGraphOwner::Route(w):0;
    io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!Guard(io,w))return Fail(io,r);
  }
  state->bootPrepared=true;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);}
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x46800;verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;
   io.RuntimeResetWrite(resets[w]);if(!Guard(io,w))return Fail(io,r);
  }
  if(!FfnWaitPeers(io,false,r))return Fail(io,r);
  phase=20;r.phase=20;if(!FfnExecute(io,r))return false;state->done=true;
  auto* m=state->manifest;state->stopping=true;m[5]=2;m[6]+=5;m[13]=m[6]+1;m[14]=0;
  for(unsigned w=0;w<119;w++)m[64+w]=state->sequence[w];
  phase=21;r.phase=21;if(!FfnExecute(io,r))return false;
  // ALL119 independent STOPs BEFORE FIRST physical peer hold.
  state->stopped=true;phase=22;r.phase=22;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !FfnCurrent(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;p.resets[w]=resets[w];
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !FfnCurrent(io,w))return Fail(io,r);
  complete=true;phase=24;r.phase=24;if(!PublishFfnHidden())return Fail(io,r);r.ffnPublished=1;return true;
 }
private:
