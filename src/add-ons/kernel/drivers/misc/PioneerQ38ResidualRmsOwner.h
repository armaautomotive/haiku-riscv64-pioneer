/* SPDX-License-Identifier: MIT */
// Included INSIDE SAME actual resident decoder. No parallel/metadata owner.
#if !defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE) || PIONEER_Q38_RESIDUAL_RMS_NATIVE != 1
#error Separate exact native56 residual+post-attention RMS owner required
#endif
 friend class PioneerQ38PostAttentionProducer;
 PioneerQ38ResidualRmsState* residual=nullptr;
 PioneerQ38PostAttentionProducer postAttention;
 bool ResidualData()const{
  if(!residual || !output || !projection || !history || !OutputData() || !output->done || !output->stopped
   || !output->bootPrepared || !output->rootProof || residual->parent!=output || residual->raw!=projection->input
   || !projection->input || !projection->input->Completed() || projection->position
   || (phase!=17 && phase!=18 && phase!=7 && phase!=8
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
    && !(ffn && (phase==19 || phase==20 || phase==21 || phase==22 || phase==24
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
     || (nextRms && (phase==25 || phase==26))
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
     || (nextProjection && phase>=27 && phase<=31)
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
     || (nextConv && phase>=32 && phase<=36)
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
     || (nextGdn && phase>=37 && phase<=41)
#endif
     ))
#endif
    ) || residual->epoch!=output->bootstrap.manifest[6]+1
   || !residual->plan.Matches(*projection->canonical,projection->token,projection->bank,projection->identity,
    projection->cohort->authority.source[0],residual->epoch)
   || !residual->plan.LearnedPagesPresent(projection->ledger,projection->pages))return false;
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
  if(!ffn) // claimed successor owns physical images/lifetimes,not old held-only receipt
#endif
  for(unsigned w=1;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running
   || resets[w]!=0x47800 || !output->image[w] || !verified[w])return false;
  return true; // raw lineage also bound by MatchesNativeOwner -> actual completed initial RMS
 }
 template<class IO> bool ResidualImage(IO& io){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(0);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(0,i),v,expected=i<=10240?PioneerQ38ResidualRmsProgramWord(i):PioneerQ38GdnOutputProgramWord(0,a);
   if(!io.ProjectionGraphMemoryReadChecked(0,a,v) || v!=expected)return false;
  }
  return true; // old ROOT main remains exact;only existing40KiB prefix replaced
 }
 template<class IO> bool ResidualTerminal(IO& io){
  if(io.RuntimeStatusRead(2)!=TT_Q38_RESIDUAL_RMS_MARKER || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=5120)return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
  return true;
 }
 template<class IO> bool ResidualCurrent(IO& io){
  if(!ResidualData() || !residual->image || !ResidualImage(io))return false;
  if(residual->staged){
   const auto* d=residual->plan.Descriptor();
   for(unsigned i=0;i<64;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,v) || v!=d[i])return false;}
  }
  if(residual->receipt){
   if(!ResidualTerminal(io))return false;
   for(unsigned i=0;i<32;i++)if(io.Q38PanelRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4)!=residual->accepted[i])return false;
  }
  return true;
 }
 template<class IO> bool ResidualReceipt(IO& io){
  if(!ResidualTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !ResidualCurrent(io))return false;
  uint32_t summary[32],after[8];
  for(unsigned i=0;i<32;i++)summary[i]=io.Q38PanelRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  const auto* d=residual->plan.Descriptor();
  if(!PioneerQ38ResidualRmsSummary(summary,d,residual->before,after))return false;
  const uint32_t last[]={0xa4000,0,129,d[4]+16384,0,d[8],0,0x212,4096,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<32;i++)residual->accepted[i]=summary[i];
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  residual->receipt=true;verified[0]=true;return Guard(io,0) && ResidualCurrent(io);
 }
 bool MatchesPostAttention(const PioneerQ38PostAttentionProducer& value)const{
  if(&value!=&postAttention || value.owner!=this || !complete || phase!=18 || closed || Unsafe() || !ResidualData()
   || !residual->done || !residual->receipt || !residual->staged || !residual->image || value.state!=residual
   || value.canonical!=projection->canonical || value.raw!=projection->input || value.ledger!=projection->ledger
   || value.pages!=projection->pages || !(value.identity==projection->identity) || value.token!=projection->token
   || value.position!=projection->position || value.bank!=projection->bank || value.phase!=residual->epoch
   || !MatchesNativeOwner(*projection,*history))return false;
  const auto* d=residual->plan.Descriptor();
  if(value.normalizedAddress!=d[4] || value.residualAddress!=d[4]+TT_Q38_RMS_BACKUP)return false;
  for(unsigned i=0;i<16;i++)if(value.source[i]!=projection->cohort->authority.source[0][i])return false;
  return !projection->running[0] && !projection->cohort->rootRunning && !projection->cohort->worker[0].image.running
   && resets[0]==0x47800 && verified[0];
 }
 bool PublishPostAttention(){
  if(postAttention.owner || !residual || !residual->done || !projection || phase!=18 || !complete)return false;
  const auto& p=*projection;const auto* d=residual->plan.Descriptor();if(!d)return false;
  postAttention.owner=this;postAttention.canonical=p.canonical;postAttention.raw=p.input;postAttention.state=residual;
  postAttention.ledger=p.ledger;postAttention.pages=p.pages;postAttention.identity=p.identity;postAttention.token=p.token;
  postAttention.position=p.position;postAttention.bank=p.bank;postAttention.phase=residual->epoch;
  postAttention.normalizedAddress=d[4];postAttention.residualAddress=d[4]+TT_Q38_RMS_BACKUP;
  for(unsigned i=0;i<16;i++)postAttention.source[i]=p.cohort->authority.source[0][i];
  return MatchesPostAttention(postAttention);
 }
 bool MatchesPostAttentionCohort(const PioneerQ38PostAttentionProducer& value,const PioneerQ38DispatchLease& cohort)const{
  return MatchesPostAttention(value) && projection && projection->cohort==&cohort;
 }
public:
 bool PostAttentionCompleted()const{return postAttention.Current();}
 const PioneerQ38PostAttentionProducer* PostAttentionInput()const{return postAttention.Current()?&postAttention:nullptr;}
 template<class IO> bool RunResidualRms(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(residual || postAttention.owner || !GdnOutputCompleted() || !projection || !history || !projection->input
   || !projection->input->Completed() || !MatchesNativeOwner(*projection,*history) || !OutputData()
   || output->bootstrap.manifest[6]>=0x7ffffffe || r.residualInstalled || r.residualRuns || r.residualHeld || r.residualPublished)return false;
  auto* state=new(std::nothrow) PioneerQ38ResidualRmsState;if(!state)return false;
  auto& p=*projection;state->parent=output;state->raw=p.input;state->epoch=output->bootstrap.manifest[6]+1;
  if(!state->plan.Build(*p.canonical,p.token,p.bank,p.identity,p.cohort->authority.source[0],state->epoch)
   || !state->plan.LearnedPagesPresent(p.ledger,p.pages)){delete state;return false;}
  // Claim BOTH lineages BEFORE first IO;old output-completion query expires.
  residual=state;phase=17;r.phase=17;complete=false;
  for(unsigned w=0;w<120;w++){r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !OutputCurrent(io,w))return Fail(io,r);}
  r.worker=0;if(!Guard(io,0))return Fail(io,r);
  for(unsigned i=0;i<=10240;i++){
   r.word=i;if((i%256==0 && !Guard(io,0)) || !Write(io,0,PioneerQ38ProjectionGraphCodeAddress(0,i),PioneerQ38ResidualRmsProgramWord(i)))return Fail(io,r);
   r.residualInstalled++;
  }
  state->image=true;if(!Guard(io,0) || !ResidualImage(io))return Fail(io,r);
  const auto* d=state->plan.Descriptor();
  for(unsigned i=0;i<64;i++)if(!Write(io,0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4,d[i]))return Fail(io,r);
  for(unsigned i=0;i<32;i++)if(!Write(io,0,TT_Q38_FFN_RESIDENT_SUMMARY+i*4,0))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  state->staged=true;for(unsigned i=0;i<8;i++)state->before[i]=counts[0][i];
  if(!Guard(io,0) || !ResidualCurrent(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  verified[0]=false;p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=true;
  resets[0]=0x46800;r.residualRuns++;io.RuntimeResetWrite(resets[0]);bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   unsigned marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_RESIDUAL_RMS_MARKER)return Fail(io,r);
   if(marker){if(!ResidualReceipt(io))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();
   if(pc!=0x6000 && (pc<0x64000 || pc>=0x64000+sizeof(PioneerQ38ResidualRmsCode) || pc==0x6403c || pc==0x64058))return Fail(io,r);
  }
  if(!done || !Guard(io,0) || !ResidualTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x47800;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  p.running[0]=p.cohort->rootRunning=p.cohort->worker[0].image.running=false;p.resets[0]=resets[0];r.residualHeld=1;
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
  state->done=true;complete=true;phase=18;r.phase=18;
  if(!PublishPostAttention())return Fail(io,r);
  r.residualPublished=1;return true;
 }
private:
