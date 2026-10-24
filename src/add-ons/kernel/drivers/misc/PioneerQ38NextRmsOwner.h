/* SPDX-License-Identifier: MIT */
// Included INSIDE SAME actual resident decoder,not an initial RMS facade.
#if !defined(PIONEER_Q38_NEXT_RMS_NATIVE) || PIONEER_Q38_NEXT_RMS_NATIVE != 1
#error Exact separate native58 next-layer RMS execution required
#endif
 friend class PioneerQ38NextRmsProducer;
 PioneerQ38NextRmsState* nextRms=nullptr;
 PioneerQ38NextRmsProducer nextInput;
 bool NextRmsData()const{
  if(!nextRms || !ffn || !ffn->done || !ffn->stopped || !ffn->rootProof || !FfnData()
   || nextRms->input!=&ffnHidden || nextRms->parent!=ffn || ffnHidden.owner!=this || ffnHidden.state!=ffn
   || ffnHidden.parent!=&postAttention || ffnHidden.canonical!=projection->canonical || ffnHidden.ledger!=projection->ledger
   || ffnHidden.pages!=projection->pages || !(ffnHidden.identity==projection->identity) || ffnHidden.token!=projection->token
   || ffnHidden.position!=projection->position || ffnHidden.bank!=projection->bank || ffnHidden.phase!=ffn->epoch+5
   || ffnHidden.nextLayer!=1 || nextRms->layer!=ffnHidden.nextLayer || ffnHidden.hiddenAddress!=ffn->manifest[4]
   || ffn->manifest[13]>=0x7ffffffe || nextRms->epoch!=ffn->manifest[13]+1
   || (phase!=25 && phase!=26 && phase!=7 && phase!=8
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
    && !(nextProjection && phase>=27 && phase<=31)
#endif
    )
   || !nextRms->plan.Matches(*projection->canonical,nextRms->layer,0,projection->position,projection->bank,
    projection->identity,projection->cohort->authority.source[0],nextRms->epoch)
   || !nextRms->plan.LearnedPagesPresent(projection->ledger,projection->pages))return false;
  const auto* d=nextRms->plan.Descriptor();if(!d || d[4]!=ffnHidden.hiddenAddress)return false;
  for(unsigned i=0;i<16;i++)if(ffnHidden.source[i]!=projection->cohort->authority.source[0][i])return false;
  return true; // historical private input lineage,NOT expired producer.Current()
 }
 template<class IO> bool NextRmsImage(IO& io){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(0);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(0,i),v;
   uint32_t expected=i<=10240?PioneerQ38LearnedRmsProgramWord(i):FfnWord(0,a);
   if(!io.ProjectionGraphMemoryReadChecked(0,a,v) || v!=expected)return false;
  }
  return true; // existing40KiB prefix replaced;old main remains exact
 }
 template<class IO> bool NextRmsTerminal(IO& io){
  if(io.RuntimeStatusRead(2)!=TT_Q38_RMS_MARKER || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=5120)return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
  return true;
 }
 template<class IO> bool NextRmsCurrent(IO& io){
  if(!NextRmsData() || !nextRms->image || !NextRmsImage(io))return false;
  if(nextRms->staged){const auto* d=nextRms->plan.Descriptor();
   for(unsigned i=0;i<64;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,v) || v!=d[i])return false;}
  }
  if(nextRms->receipt){
   if(!NextRmsTerminal(io))return false;
   for(unsigned i=0;i<32;i++)if(io.Q38PanelRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4)!=nextRms->accepted[i])return false;
  }
  return true;
 }
 template<class IO> bool NextRmsReceipt(IO& io){
  if(!NextRmsTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !NextRmsCurrent(io))return false;
  uint32_t summary[32],after[8];for(unsigned i=0;i<32;i++)summary[i]=io.Q38PanelRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  const auto* d=nextRms->plan.Descriptor();if(!PioneerQ38LearnedRmsSummary(summary,d,nextRms->before,after))return false;
  const uint32_t last[]={0x94000,0,129,d[4]+16384,0,d[8],0,0x212,4096,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<32;i++)nextRms->accepted[i]=summary[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  nextRms->receipt=verified[0]=true;return Guard(io,0) && NextRmsCurrent(io);
 }
 bool MatchesNextInput(const PioneerQ38NextRmsProducer& value)const{
  if(&value!=&nextInput || value.owner!=this || !nextRms || !nextRms->done || !nextRms->receipt || !nextRms->image || !nextRms->staged
   || !complete || phase!=26 || closed || Unsafe() || !NextRmsData() || !MatchesNativeOwner(*projection,*history)
   || value.parent!=&ffnHidden || value.state!=nextRms || value.canonical!=projection->canonical || value.ledger!=projection->ledger
   || value.pages!=projection->pages || !(value.identity==projection->identity) || value.token!=projection->token
   || value.position!=projection->position || value.bank!=projection->bank || value.layer!=nextRms->layer || value.phase!=nextRms->epoch)return false;
  const auto* d=nextRms->plan.Descriptor();
  if(value.normalizedAddress!=d[4] || value.residualAddress!=d[4]+TT_Q38_RMS_BACKUP)return false;
  for(unsigned i=0;i<16;i++)if(value.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800 || !verified[w])return false;
  return !projection->cohort->rootRunning;
 }
 bool MatchesNextCohort(const PioneerQ38NextRmsProducer& value,const PioneerQ38DispatchLease& cohort)const{
  return MatchesNextInput(value) && projection && projection->cohort==&cohort;
 }
 bool PublishNextInput(){
  if(nextInput.owner || !nextRms || !nextRms->done || !projection || phase!=26 || !complete)return false;
  auto& p=*projection;const auto* d=nextRms->plan.Descriptor();if(!d)return false;
  nextInput.owner=this;nextInput.canonical=p.canonical;nextInput.parent=&ffnHidden;nextInput.state=nextRms;
  nextInput.ledger=p.ledger;nextInput.pages=p.pages;nextInput.identity=p.identity;nextInput.token=p.token;nextInput.position=p.position;
  nextInput.bank=p.bank;nextInput.layer=nextRms->layer;nextInput.phase=nextRms->epoch;
  nextInput.normalizedAddress=d[4];nextInput.residualAddress=d[4]+TT_Q38_RMS_BACKUP;
  for(unsigned i=0;i<16;i++)nextInput.source[i]=p.cohort->authority.source[0][i];
  return MatchesNextInput(nextInput);
 }
public:
 bool NextRmsCompleted()const{return nextInput.Current();}
 const PioneerQ38NextRmsProducer* NextLayerInput()const{return nextInput.Current()?&nextInput:nullptr;}
 template<class IO> bool RunNextRms(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(nextRms || nextInput.owner || !ffnHidden.Current() || !ffn || !ffn->stopped || !ffn->rootProof
   || ffn->manifest[13]>=0x7ffffffe || !projection || !history || !MatchesNativeOwner(*projection,*history)
   || r.nextRmsInstalled || r.nextRmsRuns || r.nextRmsHeld || r.nextRmsPublished)return false;
  auto* s=new(std::nothrow) PioneerQ38NextRmsState;if(!s)return false;
  auto& p=*projection;s->input=&ffnHidden;s->parent=ffn;s->layer=ffnHidden.nextLayer;s->epoch=ffn->manifest[13]+1;
  if(s->layer!=1 || !s->plan.Build(*p.canonical,s->layer,0,p.position,p.bank,p.identity,p.cohort->authority.source[0],s->epoch)
   || !s->plan.LearnedPagesPresent(p.ledger,p.pages)){delete s;return false;}
  // Claim genuine hidden BEFORE first IO;its old completion expires NOW.
  nextRms=s;phase=25;r.phase=25;complete=false;
  for(unsigned w=0;w<120;w++){r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !FfnCurrent(io,w))return Fail(io,r);}
  r.worker=0;
  for(unsigned i=0;i<=10240;i++){
   r.word=i;if((i%256==0 && !Guard(io,0)) || !Write(io,0,PioneerQ38ProjectionGraphCodeAddress(0,i),PioneerQ38LearnedRmsProgramWord(i)))return Fail(io,r);
   r.nextRmsInstalled++;
  }
  s->image=true;if(!Guard(io,0) || !NextRmsImage(io))return Fail(io,r);
  const auto* d=s->plan.Descriptor();
  for(unsigned i=0;i<64;i++)if(!Write(io,0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,d[i]))return Fail(io,r);
  for(unsigned i=0;i<32;i++)if(!Write(io,0,TT_Q38_FFN_RESIDENT_SUMMARY+i*4,0))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  s->staged=true;for(unsigned i=0;i<8;i++)s->before[i]=counts[0][i];
  if(!Guard(io,0) || !NextRmsCurrent(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  verified[0]=false;p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;
  resets[0]=0x46800;r.nextRmsRuns++;io.RuntimeResetWrite(resets[0]);bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   unsigned marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_RMS_MARKER)return Fail(io,r);
   if(marker){if(!NextRmsReceipt(io))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();
   if(pc!=0x6000 && (pc<0x64000 || pc>=0x64000+sizeof(PioneerQ38LearnedRmsCode) || pc==0x6403c || pc==0x64058))return Fail(io,r);
  }
  if(!done || !Guard(io,0) || !NextRmsTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x47800;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;p.resets[0]=resets[0];r.nextRmsHeld=1;
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
  s->done=complete=true;phase=26;r.phase=26;
  if(!PublishNextInput())return Fail(io,r);
  r.nextRmsPublished=1;return true;
 }
private:
