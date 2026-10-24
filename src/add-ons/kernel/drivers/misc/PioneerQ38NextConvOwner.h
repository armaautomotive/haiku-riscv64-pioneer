/* SPDX-License-Identifier: MIT */
// Included INSIDE SAME actual decoder;genuine native59 output only.
#if !defined(PIONEER_Q38_NEXT_CONV_NATIVE) || PIONEER_Q38_NEXT_CONV_NATIVE != 1
#error Exact separate native60 next convolution required
#endif
 friend class PioneerQ38NextConvProducer;
 PioneerQ38NextConvState* nextConv=nullptr;
 PioneerQ38NextConvProducer nextConvolution;
 bool NextConvData()const{
  if(!nextConv || !nextConv->built || !nextProjection || !nextProjection->stopped || !nextProjection->rootProof
   || nextProjection->produced!=15 || !NextProjectionData() || nextConv->parent!=nextProjection || nextConv->input!=&nextProjections
   || nextProjections.owner!=this || nextProjections.state!=nextProjection || nextProjections.parent!=&nextInput
   || nextProjections.canonical!=projection->canonical || nextProjections.ledger!=projection->ledger || nextProjections.pages!=projection->pages
   || !(nextProjections.identity==projection->identity) || nextProjections.token!=projection->token || nextProjections.position!=projection->position
   || nextProjections.bank!=projection->bank || nextProjections.layer!=1 || nextConv->layer!=1
   || nextProjections.phase!=nextProjection->graph->Manifest()[13] || nextConv->epoch!=nextProjections.phase+1
   || ((phase<32 || phase>36) && phase!=7 && phase!=8
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
    && !(nextGdn && phase>=37 && phase<=41)
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
    && !(nextOutput && phase>=42 && phase<=46)
#endif
    )
   || nextProjections.resultAddress!=nextInput.normalizedAddress+TT_Q38_PROJECTION_RESULT_BASE || nextProjections.residualAddress!=nextInput.residualAddress)return false;
  for(unsigned i=0;i<16;i++)if(nextProjections.source[i]!=projection->cohort->authority.source[0][i])return false;
  return true; // immutable private recipe storage checked per worker before launch
 }
 bool NextConvCanonical(unsigned w)const{
  if(w>=120 || !NextConvData() || !PioneerQ38NextConvLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  const auto& p=*projection;const auto& s=*nextConv;
  if(!s.jobs[w].Matches(*p.canonical,s.layer,p.position,p.bank,w,p.identity,p.cohort->authority.source[w],s.epoch)
   || !s.jobs[w].LearnedPagesPresent(p.ledger,p.pages))return false;
  const auto* t=s.jobs[w].Records();if(!t || t[4]+TT_Q38_PROJECTION_RESULT_BASE!=s.input->resultAddress)return false;
  for(unsigned j=0;j<s.jobs[w].Count();j++)if(!HistorySpan(t+j*64))return false;
  return true;
 }
 static uint32_t NextConvWord(unsigned w,uint32_t a){
  if(a==0x6000)return PioneerQ38ConvBatchImageWord(0);
  if(a>=0x64000 && a-0x64000<TT_Q38_CONV_BATCH_TEXT_BYTES)return PioneerQ38ConvBatchCodeWord((a-0x64000)/4);
  return NextProjectionWord(w,a); // retain/check the full old immutable tail
 }
 template<class IO> bool NextConvImage(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a) && (!io.ProjectionGraphMemoryReadChecked(w,a,v) || v!=NextConvWord(w,a)))return false;
  }return true;
 }
 template<class IO> bool NextConvTerminal(IO& io,unsigned w){
  if(io.RuntimeStatusRead(2)!=TT_Q38_CONV_BATCH_MARKER || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=nextConv->jobs[w].Count()*16)return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
  return true; // speculative fallthrough only AFTER fresh success/phase/count
 }
 template<class IO> bool NextConvCurrent(IO& io,unsigned w){
  if(!NextConvCanonical(w) || !nextConv->image[w] || !NextConvImage(io,w))return false;
  if(nextConv->staged[w])for(unsigned i=0;i<384;i++){
   uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_CONV_BATCH_TABLE+i*4,v) || v!=nextConv->jobs[w].Records()[i])return false;
  }
  if(nextConv->receipt[w]){
   if(projection->running[w] && !NextConvTerminal(io,w))return false;
   if(!projection->running[w] && (resets[w]!=0x47800 || io.RuntimeStatusRead(2)!=TT_Q38_CONV_BATCH_MARKER
    || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=nextConv->jobs[w].Count()*16))return false;
   for(unsigned i=0;i<32;i++)if(io.Q38PanelRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4)!=nextConv->accepted[w][i])return false;
  }
  return true;
 }
 template<class IO> bool NextConvReceipt(IO& io,unsigned w){
  if(!NextConvTerminal(io,w) || !PioneerQ38GraphCsrIdle(io) || !NextConvCurrent(io,w))return false;
  auto& s=*nextConv;const auto* t=s.jobs[w].Records();uint32_t summary[32],after[8];
  for(unsigned i=0;i<32;i++)summary[i]=io.Q38PanelRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  if(!PioneerQ38ConvBatchSummary(summary,t,s.before[w],after))return false;
  const auto* last=t+(s.jobs[w].Count()-1)*64;const uint32_t commandLast[]={0x81000,0,last[9],last[4]+last[43],0,last[8],0,0x212,64,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=commandLast[i])return false;
  for(unsigned i=0;i<32;i++)s.accepted[w][i]=summary[i];
  for(unsigned i=0;i<8;i++)counts[w][i]=after[i];
  for(unsigned i=0;i<10;i++)command[w][i]=commandLast[i];
  s.receipt[w]=verified[w]=true;return Guard(io,w) && NextConvTerminal(io,w);
 }
 bool MatchesNextConvolution(const PioneerQ38NextConvProducer& v)const{
  if(&v!=&nextConvolution || v.owner!=this || !nextConv || !nextConv->done || !complete || closed || Unsafe() || phase!=36
   || !NextConvData() || !MatchesNativeOwner(*projection,*history) || v.state!=nextConv || v.parent!=&nextProjections
   || v.canonical!=projection->canonical || v.ledger!=projection->ledger || v.pages!=projection->pages || !(v.identity==projection->identity)
   || v.token!=projection->token || v.position!=projection->position || v.bank!=projection->bank || v.layer!=1 || v.phase!=nextConv->epoch
   || v.outputAddress!=nextInput.normalizedAddress+TT_Q38_CONV_OUTPUT || v.nextHistoryAddress!=nextInput.normalizedAddress+TT_Q38_CONV_NEXT
   || v.residualAddress!=nextInput.residualAddress)return false;
  for(unsigned i=0;i<16;i++)if(v.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800 || !verified[w]
   || !nextConv->image[w] || !nextConv->staged[w] || !nextConv->receipt[w])return false;
  return !projection->cohort->rootRunning;
 }
 bool MatchesNextConvCohort(const PioneerQ38NextConvProducer& v,const PioneerQ38DispatchLease& c)const{
  return MatchesNextConvolution(v) && projection && projection->cohort==&c;
 }
public:
 bool NextConvCompleted()const{return nextConvolution.Current();}
 const PioneerQ38NextConvProducer* NextConvolutionInput()const{return nextConvolution.Current()?&nextConvolution:nullptr;}
 template<class IO> bool RunNextConv(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(nextConv || nextConvolution.owner || !nextProjections.Current() || !nextProjection || nextProjections.phase>=0x7ffffffe
   || !projection || !history || !MatchesNativeOwner(*projection,*history) || r.nextConvInstalled || r.nextConvArmed || r.nextConvReleased
   || r.nextConvCompleted || r.nextConvHeld || r.nextConvPublished)return false;
  if(!PioneerQ38NextConvLearned(*projection->canonical,projection->ledger,projection->pages))return false;
  auto* s=new(std::nothrow) PioneerQ38NextConvState;if(!s)return false;
  auto& p=*projection;s->parent=nextProjection;s->input=&nextProjections;s->layer=nextProjections.layer;s->epoch=nextProjections.phase+1;
  unsigned jobsTotal=0;
  for(unsigned w=0;w<120;w++){
   if(s->layer!=1 || !s->jobs[w].Build(*p.canonical,s->layer,p.position,p.bank,w,p.identity,p.cohort->authority.source[w],s->epoch)
    || !s->jobs[w].LearnedPagesPresent(p.ledger,p.pages)){delete s;return false;}
   const auto* t=s->jobs[w].Records();for(unsigned j=0;j<s->jobs[w].Count();j++)if(!HistorySpan(t+j*64)){delete s;return false;}
   jobsTotal+=s->jobs[w].Count();
  }
  if(jobsTotal!=640){delete s;return false;}s->built=true;
  nextConv=s;complete=false;phase=32;r.phase=32; // genuine old input expires BEFORE IO
  for(unsigned w=0;w<120;w++){r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);}
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<TT_Q38_CONV_BATCH_IMAGE_WORDS;i++){r.word=i;if(!Write(io,w,PioneerQ38ConvBatchImageAddress(i),PioneerQ38ConvBatchImageWord(i)))return Fail(io,r);}
   if(!NextConvImage(io,w))return Fail(io,r);
   s->image[w]=true;
   for(unsigned i=0;i<384;i++){r.word=i;if(!Write(io,w,TT_Q38_CONV_BATCH_TABLE+i*4,s->jobs[w].Records()[i]))return Fail(io,r);}
   for(unsigned i=0;i<32;i++)if(!Write(io,w,TT_Q38_FFN_RESIDENT_SUMMARY+i*4,0))return Fail(io,r);
   for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
   for(unsigned i=0;i<8;i++)s->before[w][i]=counts[w][i];
   s->staged[w]=true;if(!Guard(io,w) || !NextConvCurrent(io,w))return Fail(io,r);r.nextConvInstalled++;
  }
  for(unsigned w=0;w<120;w++)if(!Guard(io,w) || !NextConvCurrent(io,w))return Fail(io,r);
  phase=33;r.phase=33;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !NextConvCurrent(io,w))return Fail(io,r);resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);r.nextConvArmed++;
  }
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned w=0;w<120;w++){
   r.worker=w;uint64_t before=io.Q38CohortNow();if(before<began || before-began>=UINT64_C(30000000))return Fail(io,r);
   if(!Guard(io,w) || !NextConvCurrent(io,w))return Fail(io,r);
   uint64_t after=io.Q38CohortNow();if(after<before || after-began>=UINT64_C(30000000))return Fail(io,r);
   verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;if(!w)p.cohort->rootRunning=true;
   resets[w]=0x46800;io.RuntimeResetWrite(resets[w]);
   uint64_t issued=io.Q38CohortNow();if(issued<after || issued-began>=UINT64_C(30000000))return Fail(io,r);
   if(!Guard(io,w))return Fail(io,r);
   r.nextConvReleased++;
  }
  phase=34;r.phase=34;bool done[120]={};unsigned finished=0;
  for(unsigned attempt=0;attempt<65536 && finished<120;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   for(unsigned w=0;w<120;w++)if(!done[w]){
    r.worker=w;if(!Guard(io,w))return Fail(io,r);uint32_t marker=io.RuntimeStatusRead(2);
    if(marker && marker!=TT_Q38_CONV_BATCH_MARKER)return Fail(io,r);
    if(marker){if(!NextConvReceipt(io,w))return Fail(io,r);done[w]=true;finished++;r.nextConvCompleted++;}
    else{io.Pause();uint32_t pc=io.RuntimePcRead();if(pc<0x64000 || pc>=0x64000+TT_Q38_CONV_BATCH_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);}
   }
   io.Pause();
  }
  if(finished!=120)return Fail(io,r);
  phase=35;r.phase=35;
  // ALL120 receipts/terminals independently proved BEFORE first hold.
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !NextConvTerminal(io,w) || !NextConvCurrent(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;if(!w)p.cohort->rootRunning=false;p.resets[w]=resets[w];r.nextConvHeld++;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !NextConvCurrent(io,w))return Fail(io,r);
  s->done=complete=true;phase=36;r.phase=36;auto& v=nextConvolution;
  v.owner=this;v.state=s;v.parent=&nextProjections;v.canonical=p.canonical;v.ledger=p.ledger;v.pages=p.pages;v.identity=p.identity;
  v.token=p.token;v.position=p.position;v.bank=p.bank;v.layer=s->layer;v.phase=s->epoch;v.outputAddress=nextInput.normalizedAddress+TT_Q38_CONV_OUTPUT;
  v.nextHistoryAddress=nextInput.normalizedAddress+TT_Q38_CONV_NEXT;v.residualAddress=nextInput.residualAddress;
  for(unsigned i=0;i<16;i++)v.source[i]=p.cohort->authority.source[0][i];
  if(!MatchesNextConvolution(v))return Fail(io,r);
  r.nextConvPublished=1;return true;
 }
private:
