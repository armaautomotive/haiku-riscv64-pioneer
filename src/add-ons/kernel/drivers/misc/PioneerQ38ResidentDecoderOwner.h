/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_DECODER_OWNER_H
#define PIONEER_Q38_RESIDENT_DECODER_OWNER_H
#if !defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || PIONEER_Q38_RESIDENT_DECODER_OWNER != 1 || !defined(PIONEER_Q38_DECODER_RUNTIME) || PIONEER_Q38_DECODER_RUNTIME != 1 || !defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || PIONEER_Q38_PROJECTION_RESULT_STASH != 1
#error Exact private retained QPS1/decoder runtime owner role required
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Resident decoder is OFFLINE arithmetic integration,not tag42 DEBUG/CPU-reference/Work promotion
#endif
#include "PioneerQ38ProjectionGraphOwner.h"
#include "PioneerQ38ConvBatchImage.h"
#include "PioneerQ38ConvBatchPlan.h"
// HEAP ONLY. Real first-token/layer0 retained QPS1 -> QCB1 arithmetic.
// Private proof consumption,STOP/ALL119 terminals/ALL120holds precede
// replacement code. The SAME full original snapshot survives both images.
// Native adapter/role still REQUIRED;IO scope must bind exact context,
// global call/cookie/team/thread/clock/model/ALL host owners. No selector.
// This does NOT commit next history or implement subsequent GDN/layers.
struct PioneerQ38ResidentDecoderResult {
 uint32_t version=1,phase=0,worker=0,word=0,unsafe=0,installed=0,armed=0,released=0,completed=0,held=0,closed=0;
 PioneerQ38ProjectionGraphResult stop;
 PioneerQ38ProjectionGraphMemoryResult memory;
};
class PioneerQ38ResidentDecoderOwner {
 PioneerQ38ProjectionGraphOwner* projection=nullptr;PioneerQ38ConvHistoryBanks* history=nullptr;
 PioneerQ38ConvBatchPlan jobs[120];
 uint32_t command[120][10]={},counts[120][8]={},resets[120]={};
 bool verified[120]={};unsigned phase=0,operationPhase=0;
 bool started=false,unsafe=false,closed=false,consumed=false,complete=false;
 template<class IO> bool Fail(IO& io,PioneerQ38ResidentDecoderResult& r){
  unsafe=true;complete=false;r.unsafe=1;
  if(history)history->Fault();
  if(projection)projection->Fail(io,r.stop);
  return false; // MEMORY ONLY;ALL owners/snapshot/maps retained,NO later IO
 }
 template<class IO> bool Scope(IO& io)const{
  return projection && history && MatchesNativeOwner(*projection,*history)
   && projection->Scope(io) && io.ResidentDecoderScope(*this,*projection,*history);
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  auto& p=*projection;
  if(w>=120 || !Scope(io) || !io.SelectRuntimeCore(w) || !io.Q38PanelMappingStable() || !io.ProjectionGraphMemoryMappingStable(w)
   || io.ResetRead()!=resets[w] || io.ResetRead()!=resets[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=p.cohort->authority.source[w][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=p.proof->source[i])return false;
  for(unsigned port=0;port<2;port++)if(!PioneerQ38GraphCsrPciQuiet(io,port))return false;
  // This NEW owner,not the old Image/Guard,tracks traffic on ALL120 cores.
  // Unknown in-flight controls are not accepted as a new baseline.
  if(p.running[w] && !verified[w])return true;
  if(!PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counts[w][i])return false;
  return true;
 }
 template<class IO> bool Image(IO& io,unsigned w){
  for(unsigned i=0;i<TT_Q38_CONV_BATCH_IMAGE_WORDS;i++){
   uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38ConvBatchImageAddress(i),v) || v!=PioneerQ38ConvBatchImageWord(i))return false;
  }
  return true;
 }
 template<class IO> bool Current(IO& io,unsigned w){
  auto& p=*projection;
  if(!jobs[w].Matches(*p.canonical,0,0,p.bank,w,p.identity,p.cohort->authority.source[w],operationPhase)
   || !jobs[w].LearnedPagesPresent(p.ledger,p.pages) || !Image(io,w))return false;
  const auto* table=jobs[w].Records();
  for(unsigned i=0;i<384;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_CONV_BATCH_TABLE+i*4,v) || v!=table[i])return false;}
  return true; // exact recipe/code/table,including inactive slots,BEFORE release
 }
 template<class IO> bool Write(IO& io,unsigned w,uint32_t a,uint32_t v){
  uint32_t actual;return PioneerQ38ProjectionGraphSnapshotContains(w,a)
   && io.ProjectionGraphMemoryWrite(w,a,v) && io.ProjectionGraphMemoryReadChecked(w,a,actual) && actual==v;
 }
 bool HistorySpan(const uint32_t* d)const{
  for(unsigned i=0;i<history->Regions;i++){
   const auto& r=history->regions[i];
   if(r.layer!=d[5] || d[7]<r.first || d[7]-r.first>r.channels || 16>r.channels-(d[7]-r.first))continue;
   unsigned relative=(d[7]-r.first)*12,first=relative/4096,pages=((relative&4095)+192+4095)/4096;
   PioneerQ38ConvHistoryExtent e;
   return history->InitializeSpan(projection->identity,i,first,pages,e) && e.bank==d[14]
    && e.address+(relative&4095)==d[15] && e.route==d[41];
  }
  return false;
 }
 bool Build(){
  auto& p=*projection;
  for(unsigned w=0;w<120;w++){
   jobs[w].~PioneerQ38ConvBatchPlan();new(&jobs[w]) PioneerQ38ConvBatchPlan;
   if(!jobs[w].Build(*p.canonical,0,0,p.bank,w,p.identity,p.cohort->authority.source[w],operationPhase)
    || !jobs[w].LearnedPagesPresent(p.ledger,p.pages))return false;
   const auto* table=jobs[w].Records();
   for(unsigned j=0;j<jobs[w].Count();j++)if(!HistorySpan(table+j*64))return false;
   for(unsigned i=0;i<TT_Q38_CONV_BATCH_IMAGE_WORDS;i++)
    if(!PioneerQ38ProjectionGraphSnapshotContains(w,PioneerQ38ConvBatchImageAddress(i)))return false;
  }
  return true; // DATA+real reserved initial-zero spans,not execution proof
 }
 template<class IO> bool Terminal(IO& io,unsigned w){
  if(io.RuntimeStatusRead(2)!=TT_Q38_CONV_BATCH_MARKER || io.RuntimeStatusRead(0)!=0x6400c
   || io.RuntimeStatusRead(1)!=jobs[w].Count()*16)return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38ConvBatchTerminalPc(io.RuntimePcRead()))return false;}
  return true;
 }
 template<class IO> bool Receipt(IO& io,unsigned w){
  if(!Terminal(io,w) || !PioneerQ38GraphCsrIdle(io) || !Current(io,w))return false;
  const auto* table=jobs[w].Records();uint32_t s[32],after[8];
  for(unsigned i=0;i<384;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_CONV_BATCH_TABLE+i*4,v) || v!=table[i])return false;}
  for(unsigned i=0;i<32;i++)s[i]=io.Q38PanelRead(TT_Q38_FFN_RESIDENT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  if(!PioneerQ38ConvBatchSummary(s,table,counts[w],after))return false;
  const auto* last=table+(jobs[w].Count()-1)*64;
  const uint32_t end[]={0x81000,0,last[9],last[4]+last[43],0,last[8],0,0x212,64,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=end[i])return false;
  // Advance ONLY after independent terminal/source/ALL table/receipt/code/
  // command/counter proofs. Never baseline whatever a running core reports.
  for(unsigned i=0;i<8;i++)counts[w][i]=after[i];
  for(unsigned i=0;i<10;i++)command[w][i]=end[i];
  verified[w]=true;return Guard(io,w) && Terminal(io,w);
 }
public:
 PioneerQ38ResidentDecoderOwner()=default;
 PioneerQ38ResidentDecoderOwner(const PioneerQ38ResidentDecoderOwner&)=delete;
 PioneerQ38ResidentDecoderOwner& operator=(const PioneerQ38ResidentDecoderOwner&)=delete;
 bool Unsafe()const{return unsafe || (projection && projection->Unsafe()) || (history && history->Unsafe());}
 bool Closed()const{return closed && !Unsafe();}
 bool Claimed()const{return started;}
 bool MatchesNativeOwner(const PioneerQ38ProjectionGraphOwner& p,const PioneerQ38ConvHistoryBanks& h)const{
  return started && !unsafe && !closed && consumed && projection==&p && history==&h && p.decoderOwner==this && h.decoderOwner==this
   && p.proof && p.banks && p.cohort && p.memory && p.graph && p.canonical && p.input && p.ledger
   && h.banks==p.banks && h.baseline==p.proof && h.canonical==p.canonical && h.reservationOwner==p.graph
   && h.identity==p.identity && h.published && !h.initializationLease && h.Matches(p.identity)
   && p.MatchesNativeOwner(*p.proof,*p.banks,*p.cohort,*p.memory,*p.graph,*p.canonical,*p.input,p.identity,p.ledger,p.pages);
 } // CPU relation ONLY;actual IO scope and physical guards remain mandatory
 template<class IO> bool RunOnce(IO& io,PioneerQ38ProjectionGraphOwner& p,PioneerQ38ConvHistoryBanks& h,PioneerQ38ResidentDecoderResult& r){
  r={};
  if(started || unsafe || closed || p.decoderOwner || h.decoderOwner || !p.Ready() || !p.output || p.produced!=15 || p.running[0]
   || p.position || !p.graph || !p.canonical || !p.state || !p.memory || !p.proof || !p.banks || !p.cohort || p.nonce>=0x7fffffed
   || p.graph->ConvHistory(p.identity)!=&h || !h.published || h.initializationLease
   || !h.MatchesInitializationOwner(*p.banks,*p.proof,p.identity) || !p.memory->launched || p.memory->closed || p.memory->unsafe)return false;
  const unsigned roles[]={10U,11U,5U,6U};
  for(unsigned role:roles)if(!p.MatchesCompleted(*p.graph,*p.canonical,p.identity,p.token,0,p.bank,role))return false;
  projection=&p;history=&h;operationPhase=p.nonce+2;
  if(!Build()){projection=nullptr;history=nullptr;operationPhase=0;return false;} // NO claim or IO
  if(!p.Manifest(p.role,true)){projection=nullptr;history=nullptr;operationPhase=0;return false;}
  // Consume the ACTUAL private proof before first STOP/possible mutation.
  // Old Run/Close/MatchesCompleted become permanently unavailable NOW.
  started=consumed=true;p.decoderOwner=this;h.decoderOwner=this;p.produced=0;p.output=false;phase=1;r.phase=1;
  if(!Scope(io) || !p.Execute(io,true,r.stop))return Fail(io,r);
  if(p.nonce+1!=operationPhase)return Fail(io,r);
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Scope(io) || !p.Guard(io,w) || !p.Image(io,w) || !p.PeerFrame(io,w) || p.Peer(io,w,true)!=1)return Fail(io,r);
   p.resets[w]=0x87ffff;io.RuntimeResetWrite(p.resets[w]);io.Pause();if(!p.Guard(io,w))return Fail(io,r);
   p.resets[w]=0x47800;io.RuntimeResetWrite(p.resets[w]);io.Pause();if(!p.Guard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;
  }
  // ALL120 old images/physical holds are re-proved BEFORE ANY code write.
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(p.running[w] || p.resets[w]!=0x47800 || !Scope(io) || !p.Guard(io,w) || !p.Image(io,w))return Fail(io,r);
   resets[w]=0x47800;verified[w]=true;
   for(unsigned i=0;i<10;i++)command[w][i]=p.command[w][i];
   for(unsigned i=0;i<8;i++)counts[w][i]=p.counts[w][i];
  }
  phase=2;r.phase=2;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<TT_Q38_CONV_BATCH_IMAGE_WORDS;i++){r.word=i;if(!Write(io,w,PioneerQ38ConvBatchImageAddress(i),PioneerQ38ConvBatchImageWord(i)))return Fail(io,r);}
   const auto* table=jobs[w].Records();
   for(unsigned i=0;i<384;i++){r.word=i;if(!Write(io,w,TT_Q38_CONV_BATCH_TABLE+i*4,table[i]))return Fail(io,r);}
   for(unsigned i=0;i<32;i++)if(!Write(io,w,TT_Q38_FFN_RESIDENT_SUMMARY+i*4,0))return Fail(io,r);
   for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
   if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   r.installed++;
  }
  // Complete SECOND pass before FIRST reset/start of the replacement image.
  for(unsigned w=0;w<120;w++)if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
  phase=3;r.phase=3;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);r.armed++;
  }
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;
   if(!w)p.cohort->rootRunning=true;
   resets[w]=0x46800;io.RuntimeResetWrite(resets[w]);if(!Guard(io,w))return Fail(io,r);r.released++;
  }
  phase=4;r.phase=4;bool done[120]={};unsigned finished=0;uint64_t began=io.Q38CohortNow();
  if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned attempt=0;attempt<65536 && finished<120;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   for(unsigned w=0;w<120;w++)if(!done[w]){
    r.worker=w;if(!Guard(io,w))return Fail(io,r);uint32_t marker=io.RuntimeStatusRead(2);
    if(marker && marker!=TT_Q38_CONV_BATCH_MARKER)return Fail(io,r);
    if(marker){if(!Guard(io,w) || !Receipt(io,w))return Fail(io,r);done[w]=true;finished++;r.completed++;}
    else{io.Pause();uint32_t pc=io.RuntimePcRead();if(pc<0x64000 || pc>=0x64000+TT_Q38_CONV_BATCH_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);}
   }
   io.Pause();
  }
  if(finished!=120)return Fail(io,r);
  // ALL120 independent terminals/receipts BEFORE FIRST post-compute hold.
  phase=5;r.phase=5;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Terminal(io,w) || !Current(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;if(!w)p.cohort->rootRunning=false;p.resets[w]=resets[w];r.held++;
  }
  for(unsigned w=0;w<120;w++)if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
  complete=true;phase=6;r.phase=6;return true; // private outputs stay ON CARD,NOT committed next history or GDN/full inference
 }
 template<class IO> bool Close(IO& io,PioneerQ38ResidentDecoderResult& r){
  if(!started || unsafe || closed || !complete || phase!=6 || !projection || !history)return false;
  auto& p=*projection;phase=7;r.phase=7;r.memory={};
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(w);i++){
    r.word=i;uint32_t a=PioneerQ38ProjectionGraphSnapshotAddress(w,i),v=p.memory->saved[PioneerQ38ProjectionGraphSnapshotBase(w)+i];
    if(!Write(io,w,a,v))return Fail(io,r);
    r.memory.restoredWords++;
   }
   for(unsigned i=0;i<10;i++){command[w][i]=p.memory->command[w][i];io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);p.command[w][i]=command[w][i];}
   for(unsigned i=0;i<8;i++)p.counts[w][i]=counts[w][i];
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  phase=8;r.phase=8;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !p.memory->VerifyOriginal(io,w,r.memory) || !Guard(io,w))return Fail(io,r);}
  if(r.memory.restoredWords!=8134072 || r.memory.verifiedOriginalWords!=8134072 || !Guard(io,0)
   || !p.state->CloseExecution(p.identity,&p,p.input))return Fail(io,r);
  p.closed=p.memory->closed=true;p.memory->installed=false;p.ready=false;closed=true;complete=false;phase=9;r.phase=9;r.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38ResidentDecoderOwner)>327680 && sizeof(PioneerQ38ResidentDecoderOwner)<524288,"actual decoder plans/lifetime HEAP ONLY");
#endif
