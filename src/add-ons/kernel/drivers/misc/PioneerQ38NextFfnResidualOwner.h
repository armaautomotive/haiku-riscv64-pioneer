/* SPDX-License-Identifier: MIT */
// Included INSIDE SAME decoder after genuine native63. Layer1 normalized
// and raw residual stay in GDDR;no new numeric host staging or ready facade.
#if !defined(PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE) || PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE != 1
#error Exact separate native65 SAME native63 FFN+residual required
#endif
 friend class PioneerQ38NextFfnHiddenProducer;
 PioneerQ38NextFfnResidualState* nextFfn=nullptr;
 PioneerQ38NextFfnHiddenProducer nextFfnHidden;
 unsigned nextFfnClaimEpoch=0,nextFfnPublishedEpoch=0;
 bool NextFfnRetainedPhase()const{return nextFfn && (phase==50 || phase==51 || phase==52 || phase==53 || phase==55);}
 template<class IO> bool NextFfnAlive(IO& io)const{
  if(!nextFfn || nextFfn->uncertain)return false;
  if(nextFfn->timing && !nextFfn->deadline.Observe(io.Q38CohortNow())){nextFfn->uncertain=true;return false;}
  return true;
 }
 template<class IO> bool NextFfnGuard(IO& io,unsigned w){return NextFfnAlive(io) && Guard(io,w) && NextFfnAlive(io);}
 template<class IO> bool NextFfnMemory(IO& io,unsigned w,uint32_t a,uint32_t& v){
  return NextFfnAlive(io) && io.ProjectionGraphMemoryReadChecked(w,a,v) && NextFfnAlive(io);
 }
 template<class IO> bool NextFfnWrite(IO& io,unsigned w,uint32_t a,uint32_t v){
  if(!NextFfnAlive(io) || !PioneerQ38ProjectionGraphSnapshotContains(w,a) || !io.ProjectionGraphMemoryWrite(w,a,v) || !NextFfnAlive(io))return false;
  uint32_t actual;return NextFfnMemory(io,w,a,actual) && actual==v;
 }
 template<class IO> uint32_t NextFfnPanel(IO& io,uint32_t a){
  if(!NextFfnAlive(io))return 0;
  uint32_t v=io.Q38PanelRead(a);return NextFfnAlive(io)?v:0;
 }
 template<class IO> uint32_t NextFfnStatus(IO& io,unsigned a){
  if(!NextFfnAlive(io))return 0;
  uint32_t v=io.RuntimeStatusRead(a);return NextFfnAlive(io)?v:0;
 }
 template<class IO> uint32_t NextFfnPc(IO& io){
  if(!NextFfnAlive(io))return 0;
  uint32_t v=io.RuntimePcRead();return NextFfnAlive(io)?v:0;
 }
 template<class IO> uint32_t NextFfnNoc(IO& io,unsigned a){
  if(!NextFfnAlive(io))return 0;
  uint32_t v=io.ResidentNocRead(a);return NextFfnAlive(io)?v:0;
 }
 template<class IO> bool NextFfnStatusWrite(IO& io,unsigned a,uint32_t v){return NextFfnAlive(io) && (io.RuntimeStatusWrite(a,v),NextFfnAlive(io));}
 template<class IO> bool NextFfnNocWrite(IO& io,unsigned a,uint32_t v){return NextFfnAlive(io) && (io.ResidentNocWrite(a,v),NextFfnAlive(io));}
 template<class IO> bool NextFfnReset(IO& io,uint32_t v){return NextFfnAlive(io) && (io.RuntimeResetWrite(v),NextFfnAlive(io));}
 template<class IO> bool NextFfnPause(IO& io){return NextFfnAlive(io) && (io.Pause(),NextFfnAlive(io));}
 template<class IO> bool NextFfnQuiet(IO& io){return NextFfnAlive(io) && PioneerQ38GraphCsrIdle(io) && NextFfnAlive(io);}
 template<class IO> bool NextFfnStage(IO& io,uint32_t address,const uint32_t* words,unsigned count){
  for(unsigned i=0;i<count;i++)if(!NextFfnWrite(io,0,address+i*4,words[i]))return false;
  return NextFfnAlive(io);
 }
 bool NextFfnData()const{
  if(!nextFfn || !nextFfn->built || !projection || !nextResidual || !nextResidual->done || !nextResidual->receipt || !NextResidualData()
   || nextFfn->parent!=nextResidual || nextFfn->input!=&nextFfnInput || nextFfnInput.owner!=this || nextFfnInput.state!=nextResidual
   || nextFfnInput.canonical!=projection->canonical || nextFfnInput.ledger!=projection->ledger || nextFfnInput.pages!=projection->pages
   || !(nextFfnInput.identity==projection->identity) || nextFfnInput.token!=projection->token || nextFfnInput.position!=projection->position
   || nextFfnInput.bank!=projection->bank || nextFfnInput.phase!=nextResidual->epoch || nextFfn->epoch!=nextResidual->epoch+1
   || (!NextFfnRetainedPhase() && phase!=7 && phase!=8))return false;
  if(nextFfn->uncertain || nextFfnClaimEpoch!=nextFfn->epoch || nextFfnClaimEpoch!=nextResidualPublishedEpoch+1
   || !nextResidual->done || !nextResidual->held || !nextResidual->receipt || nextResidual->timing
   || nextFfnInput.layer!=1 || nextFfnInput.phase!=nextResidualPublishedEpoch)return false;
  const auto* m=nextFfn->manifest;
  if(!PioneerQ38FfnResidualGraphFrame(m) || m[6]!=nextFfn->epoch+(nextFfn->stopping?5U:0U)
   || m[4]!=nextFfnInput.normalizedAddress || m[4]+0x18000!=nextFfnInput.residualAddress || m[3]!=projection->bank
   || m[40]!=projection->position || m[41] || m[42])return false;
  const uint64_t id[]={projection->identity.session,projection->identity.sequence,projection->identity.modelEpoch,projection->identity.driverGeneration};
  for(unsigned i=0;i<4;i++)if(m[32+i*2]!=uint32_t(id[i]) || m[33+i*2]!=uint32_t(id[i]>>32))return false;
  for(unsigned i=0;i<16;i++)if(m[16+i]!=nextFfnInput.source[i] || m[16+i]!=projection->cohort->authority.source[0][i])return false;
  // Private jobs/manifest/source storage is never exposed or writable by
  // callers. Full canonical roles/pages and shape checked at construction
  // and BEFORE EACH launch,not an O(all-pages) CPU scan per codeword. SAME
  // native call lock/source/ledger/host/clock scope remains mandatory.
  return true;
 }
 bool NextFfnCanonical()const{
  if(!NextFfnData() || !nextFfn->jobs.Matches(*projection->canonical,1,projection->position)
   || !PioneerQ38NextFfnResidualLearned(*projection->canonical,projection->ledger,projection->pages)
   || !nextFfn->jobs.LearnedPagesPresent(projection->ledger,projection->pages)
   || !PioneerQ38FfnResidualGraphManifest(nextFfn->manifest,nextFfn->sources))return false;
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)if(nextFfn->sources[w*16+i]!=projection->cohort->authority.source[w+1][i])return false;
  return true;
 }
 bool BuildNextFfn(PioneerQ38NextFfnResidualState& s){
  auto& p=*projection;const auto* workspace=p.canonical->Workspace(p.bank);
  if(!workspace || workspace->bytes!=67108864 || workspace->allocated!=67108864 || p.position
   || nextFfnInput.phase>=0x7ffffff8 || !s.jobs.Build(*p.canonical,1,p.position)
   || !s.jobs.LearnedPagesPresent(p.ledger,p.pages))return false;
  s.input=&nextFfnInput;s.parent=nextResidual;s.epoch=nextFfnInput.phase+1;
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
  if(!PioneerQ38FfnResidualGraphManifest(m,s.sources) || m[4]!=nextFfnInput.normalizedAddress || m[4]+0x18000!=nextFfnInput.residualAddress)return false;
  s.built=true;return true; // DATA derived from actual private producer;not claimed yet
 }
 static uint32_t NextFfnWord(unsigned w,uint32_t address){
  // Fill old larger GDN peer-prefix tail with ZERO within SAME saved lease.
  return PioneerQ38FfnGraphProgramAddress(w,address)?PioneerQ38FfnResidualProgramWord(w,address):0;
 }
 template<class IO> bool NextFfnImage(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a)
    && (!NextFfnMemory(io,w,a,v) || v!=NextFfnWord(w,a)))return false;
  }
  return NextFfnAlive(io);
 }
 template<class IO> int NextFfnPeer(IO& io,unsigned w,bool stop){
  unsigned marker=NextFfnPanel(io,0x7414c),seq=nextFfn->sequence[w-1];
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER){if(!seq && !stop && (marker==0 || marker==0xdeadbeef))return 0;return -1;}
  unsigned requested=NextFfnPanel(io,0x74140),finished=NextFfnPanel(io,0x74144),op=NextFfnPanel(io,0x74150);
  if(NextFfnPanel(io,0x74148) || finished>requested || requested>seq || finished>seq || (op && op!=(stop?2U:1U)))return -1;
  for(unsigned i=5;i<16;i++)if(NextFfnPanel(io,0x74140+i*4))return -1;
  if(!projection->running[w])return stop && nextFfn->stopped && nextFfn->stopReceipt[w] && nextFfn->held[w]
   && resets[w]==0x47800 && requested==seq && finished==seq && op==2 && NextFfnAlive(io)?1:-1;
  bool terminal=true;
  for(unsigned i=0;i<2;i++){NextFfnPause(io);uint32_t pc=NextFfnPc(io);
   if(!PioneerQ38FfnResidualExecutableAddress(w,pc) || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK
    || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):(pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH);
  }
  return requested==seq && finished==seq && op==(stop?2U:0U) && terminal?1:0;
 }
 template<class IO> bool NextFfnPeerFrame(IO& io,unsigned w){
  for(unsigned i=0;i<8;i++)if(NextFfnPanel(io,0x74040+i*4)!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:
   i==2?PioneerQ38ProjectionGraphOwner::Route(w):i==3?projection->cohort->authority.source[w][0]:0))return false;
  if(nextFfn->stopped)for(unsigned i=0;i<16;i++)if(NextFfnPanel(io,0x74000+i*4)!=(i==10?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==11?w:i==12?nextFfn->sequence[w-1]:i==13?2U:0U))return false;
  return NextFfnPanel(io,0x74100)==nextFfn->sequence[w-1] && NextFfnAlive(io);
 }
 template<class IO> bool NextFfnRootTerminal(IO& io){
  if(NextFfnStatus(io,2)!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER || NextFfnStatus(io,0)!=0x7200c || NextFfnStatus(io,1)!=(nextFfn->stopping?119U:5120U))return false;
  // Live terminal PCs were proved BEFORE the physical hold. While held,
  // verify that retained proof plus actual reset/source/counter/image/L1
  // receipts;never reinterpret a held core's live PC as executing code.
  if(!projection->running[0])return resets[0]==0x47800 && nextFfn->rootHeld && nextFfn->held[0] && nextFfn->rootProof && NextFfnAlive(io);
  for(unsigned i=0;i<2;i++){NextFfnPause(io);if(!PioneerQ38RootTerminalPc(NextFfnPc(io)))return false;}return true;
 }
 template<class IO> bool NextFfnRootData(IO& io){
  for(unsigned i=0;i<TT_Q38_FFN_RESIDUAL_GRAPH_WORDS;i++){
   uint32_t a,b;if(!NextFfnMemory(io,0,TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+i*4,a) || a!=nextFfn->manifest[i]
    || !NextFfnMemory(io,0,TT_Q38_FFN_RESIDUAL_GRAPH_SHADOW+i*4,b) || b!=nextFfn->manifest[i])return false;
  }
  for(unsigned i=0;i<119*16;i++){uint32_t v;if(!NextFfnMemory(io,0,0x7d800+i*4,v) || v!=nextFfn->sources[i])return false;}
  return NextFfnAlive(io);
 }
 template<class IO> bool NextFfnCurrent(IO& io,unsigned w){
  if(!NextFfnData() || !NextFfnImage(io,w))return false;
  if(!nextFfn->bootPrepared)return !projection->running[w] && resets[w]==0x47800;
  if(w)return NextFfnPeer(io,w,nextFfn->stopped)==1 && NextFfnPeerFrame(io,w);
  if(!nextFfn->rootProof)return !projection->running[0] && resets[0]==0x47800;
  if(!NextFfnRootTerminal(io) || !NextFfnRootData(io))return false;
  for(unsigned i=0;i<64;i++)if(NextFfnPanel(io,TT_Q38_FFN_RESIDUAL_GRAPH_STATS+i*4)!=nextFfn->accepted[i])return false;
  return NextFfnAlive(io);
 }
 template<class IO> bool NextFfnWaitPeers(IO& io,bool stop,PioneerQ38ResidentDecoderResult& r){
  bool done[119]={};unsigned finished=0;uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){r.worker=w;if(!NextFfnGuard(io,w))return false;int value=NextFfnPeer(io,w,stop);if(value<0)return false;
    if(value==1){if(!NextFfnImage(io,w) || !NextFfnPeerFrame(io,w))return false;verified[w]=true;if(!NextFfnGuard(io,w))return false;
     done[w-1]=true;finished++;if(stop){nextFfn->stopReceipt[w]=true;r.nextFfnStopped++;}else if(phase==50)r.nextFfnBooted++;}}
   NextFfnPause(io);
  }
  return finished==119 && NextFfnAlive(io);
 }
 template<class IO> bool NextFfnProof(IO& io){
  if(!NextFfnGuard(io,0) || !NextFfnRootTerminal(io) || !NextFfnQuiet(io) || !NextFfnImage(io,0) || !NextFfnRootData(io))return false;
  uint32_t stats[64],after[8];for(unsigned i=0;i<64;i++)stats[i]=NextFfnPanel(io,TT_Q38_FFN_RESIDUAL_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++)after[i]=NextFfnNoc(io,PioneerQ38PanelCounter(i));
  const auto* m=nextFfn->manifest;if(!PioneerQ38FfnResidualGraphReceipt(stats,m,nextFfn->before,after))return false;
  for(unsigned w=0;w<119;w++)if(NextFfnPanel(io,TT_Q38_FFN_RESIDUAL_GRAPH_SEQUENCE+w*4)!=PioneerQ38FfnResidualGraphNextPeer(m,w))return false;
  uint32_t last[10]={};
  if(nextFfn->stopping){last[0]=0xffb20228;last[2]=PioneerQ38ProjectionGraphOwner::Route(119);last[3]=0x75008;last[5]=129;last[8]=4;}
  else{last[0]=0x9d000;last[2]=129;last[3]=m[4]+16384;last[5]=m[8];last[7]=0x212;last[8]=4096;}
  for(unsigned i=0;i<10;i++)if(NextFfnNoc(io,i*4)!=last[i])return false;
  for(unsigned i=0;i<64;i++)nextFfn->accepted[i]=stats[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  verified[0]=nextFfn->rootProof=true;return NextFfnGuard(io,0);
 }
 void NextFfnAdvance(){
  const auto* m=nextFfn->manifest;
  for(unsigned w=1;w<120;w++){
   nextFfn->sequence[w-1]=PioneerQ38FfnResidualGraphNextPeer(m,w-1);if(nextFfn->stopping)continue;
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
 template<class IO> bool NextFfnExecute(IO& io,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;r.worker=0;
  if(!NextFfnCanonical() || !NextFfnGuard(io,0) || !NextFfnImage(io,0)
   || !NextFfnStage(io,TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST,nextFfn->manifest,TT_Q38_FFN_RESIDUAL_GRAPH_WORDS)
   || !NextFfnStage(io,0x7d800,nextFfn->sources,119*16))return Fail(io,r);
  for(unsigned i=0;i<3;i++){NextFfnStatusWrite(io,i,0);if(NextFfnStatus(io,i))return Fail(io,r);}
  for(unsigned i=0;i<8;i++)nextFfn->before[i]=counts[0][i];
  nextFfn->rootProof=false;nextFfn->rootHeld=false;nextFfn->held[0]=false;verified[0]=false;
  if(!NextFfnGuard(io,0))return Fail(io,r);
  resets[0]=0x87ffff;p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;
  NextFfnReset(io,resets[0]);NextFfnPause(io);if(!NextFfnGuard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned w=1;w<120;w++)verified[w]=false;
  resets[0]=0x46800;r.nextFfnRootRuns++;NextFfnReset(io,resets[0]);bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!NextFfnGuard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   unsigned marker=NextFfnStatus(io,2);if(marker && marker!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER)return Fail(io,r);
   if(marker){if(!NextFfnProof(io))return Fail(io,r);done=true;break;}
   NextFfnPause(io);uint32_t pc=NextFfnPc(io);if(!PioneerQ38FfnResidualExecutableAddress(0,pc) || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!done)return Fail(io,r);
  NextFfnAdvance();
  if(!NextFfnWaitPeers(io,nextFfn->stopping,r) || !NextFfnGuard(io,0) || !NextFfnRootTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;NextFfnReset(io,resets[0]);NextFfnPause(io);if(!NextFfnGuard(io,0))return Fail(io,r);
  resets[0]=0x47800;NextFfnReset(io,resets[0]);NextFfnPause(io);if(!NextFfnGuard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;p.resets[0]=resets[0];
  nextFfn->rootHeld=nextFfn->held[0]=true;return NextFfnAlive(io);
 }
 bool MatchesNextFfnHidden(const PioneerQ38NextFfnHiddenProducer& value)const{
  if(&value!=&nextFfnHidden || value.owner!=this || !nextFfn || !nextFfn->done || !nextFfn->stopped || !nextFfn->rootProof || !complete || closed || Unsafe()
   || phase!=55 || !NextFfnData() || !MatchesNativeOwner(*projection,*history) || value.parent!=&nextFfnInput || value.state!=nextFfn
   || value.canonical!=projection->canonical || value.ledger!=projection->ledger || value.pages!=projection->pages
   || !(value.identity==projection->identity) || value.token!=projection->token || value.position!=projection->position
   || nextFfnPublishedEpoch!=nextFfnClaimEpoch || nextFfnPublishedEpoch!=nextFfn->epoch
   || value.bank!=projection->bank || value.phase!=nextFfn->epoch+5 || value.nextLayer!=2 || value.hiddenAddress!=nextFfn->manifest[4])return false;
  for(unsigned i=0;i<16;i++)if(value.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800
   || !verified[w] || !nextFfn->image[w] || !nextFfn->held[w] || (w && !nextFfn->stopReceipt[w]))return false;
  return nextFfn->rootHeld && !projection->cohort->rootRunning;
 }
 bool PublishNextFfnHidden(){
  if(nextFfnHidden.owner || !nextFfn || !nextFfn->done || !nextFfn->stopped || !projection || phase!=55 || !complete)return false;
  auto& p=*projection;nextFfnHidden.owner=this;nextFfnHidden.canonical=p.canonical;nextFfnHidden.parent=&nextFfnInput;nextFfnHidden.state=nextFfn;
  nextFfnHidden.ledger=p.ledger;nextFfnHidden.pages=p.pages;nextFfnHidden.identity=p.identity;nextFfnHidden.token=p.token;nextFfnHidden.position=p.position;
  nextFfnHidden.bank=p.bank;nextFfnHidden.phase=nextFfn->epoch+5;nextFfnHidden.nextLayer=2;nextFfnHidden.hiddenAddress=nextFfn->manifest[4];
  for(unsigned i=0;i<16;i++)nextFfnHidden.source[i]=p.cohort->authority.source[0][i];
  return MatchesNextFfnHidden(nextFfnHidden);
 }
 bool MatchesNextFfnCohort(const PioneerQ38NextFfnHiddenProducer& value,const PioneerQ38DispatchLease& cohort)const{
  return MatchesNextFfnHidden(value) && projection && projection->cohort==&cohort;
 }
public:
 bool NextFfnCompleted()const{return nextFfnHidden.Current();}
 const PioneerQ38NextFfnHiddenProducer* NextFfnHiddenInput()const{return nextFfnHidden.Current()?&nextFfnHidden:nullptr;}
 template<class IO> bool RunNextFfn(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(nextFfn || nextFfnHidden.owner || nextFfnClaimEpoch || nextFfnPublishedEpoch || !nextFfnInput.Current() || !projection || !history || !MatchesNativeOwner(*projection,*history)
   || r.nextFfnInstalled || r.nextFfnBooted || r.nextFfnRootRuns || r.nextFfnStopped || r.nextFfnPublished)return false;
  if(!PioneerQ38NextFfnResidualLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  auto* state=new(std::nothrow) PioneerQ38NextFfnResidualState;if(!state)return false;
  if(!BuildNextFfn(*state)){delete state;return false;}
  // Claim actual normalized AND raw residual lineages BEFORE FIRST IO. Old
  // completion query expires;SAME original snapshot/clock/host owners stay.
  nextFfn=state;nextFfnClaimEpoch=state->epoch;phase=50;r.phase=50;complete=false;auto& p=*projection;
  if(!NextFfnCanonical())return Fail(io,r);
  for(unsigned w=0;w<120;w++){r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !NextFfnGuard(io,w) || !Current(io,w))return Fail(io,r);}
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!NextFfnGuard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
    r.word=i;uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i);if(!NextFfnWrite(io,w,a,NextFfnWord(w,a)))return Fail(io,r);
   }
   if(!NextFfnImage(io,w))return Fail(io,r);
   state->image[w]=true;r.nextFfnInstalled++;
  }
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!NextFfnGuard(io,w) || !NextFfnCurrent(io,w))return Fail(io,r);
   const uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,PioneerQ38ProjectionGraphOwner::Route(w),p.cohort->authority.source[w][0],0,0,0,0};
   for(unsigned i=0;i<96;i++)if(!NextFfnWrite(io,w,0x74000+i*4,i>=16 && i<24?boot[i-16]:i>=80?0xdeadbeef:0))return Fail(io,r);
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?PioneerQ38ProjectionGraphOwner::Route(w):0;
    NextFfnNocWrite(io,i*4,command[w][i]);if(NextFfnNoc(io,i*4)!=command[w][i])return Fail(io,r);}
   if(!NextFfnGuard(io,w))return Fail(io,r);
  }
  state->bootPrepared=true;
  // One interval starts BEFORE first peer arm/release and is NEVER renewed
  // across boot,FFN,STOP,all physical holds or final publication.
  if(!state->deadline.Begin(io.Q38CohortNow()))return Fail(io,r);
  state->timing=true;if(!NextFfnAlive(io))return Fail(io,r);
  for(unsigned w=1;w<120;w++){r.worker=w;if(!NextFfnGuard(io,w))return Fail(io,r);resets[w]=0x87ffff;NextFfnReset(io,resets[w]);NextFfnPause(io);if(!NextFfnGuard(io,w))return Fail(io,r);}
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!NextFfnGuard(io,w))return Fail(io,r);resets[w]=0x46800;verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;
   NextFfnReset(io,resets[w]);if(!NextFfnGuard(io,w))return Fail(io,r);
  }
  if(!NextFfnWaitPeers(io,false,r))return Fail(io,r);
  phase=51;r.phase=51;if(!NextFfnExecute(io,r))return false;state->done=true;
  auto* m=state->manifest;state->stopping=true;m[5]=2;m[6]+=5;m[13]=m[6]+1;m[14]=0;
  for(unsigned w=0;w<119;w++)m[64+w]=state->sequence[w];
  phase=52;r.phase=52;if(!NextFfnExecute(io,r))return false;
  // ALL119 independent STOPs BEFORE FIRST physical peer hold.
  state->stopped=true;phase=53;r.phase=53;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!NextFfnGuard(io,w) || !NextFfnCurrent(io,w))return Fail(io,r);
   resets[w]=0x87ffff;NextFfnReset(io,resets[w]);NextFfnPause(io);if(!NextFfnGuard(io,w))return Fail(io,r);
   resets[w]=0x47800;NextFfnReset(io,resets[w]);NextFfnPause(io);if(!NextFfnGuard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;p.resets[w]=resets[w];state->held[w]=true;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !NextFfnGuard(io,w) || !NextFfnCurrent(io,w))return Fail(io,r);
  complete=true;phase=55;r.phase=55;nextFfnPublishedEpoch=state->epoch;
  if(!NextFfnAlive(io) || !PublishNextFfnHidden() || !NextFfnAlive(io))return Fail(io,r);
  state->timing=false;r.nextFfnPublished=1;return true;
 }
private:
