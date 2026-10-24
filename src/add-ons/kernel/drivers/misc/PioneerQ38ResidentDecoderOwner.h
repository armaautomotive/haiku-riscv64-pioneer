/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_DECODER_OWNER_H
#define PIONEER_Q38_RESIDENT_DECODER_OWNER_H
#if !defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || PIONEER_Q38_RESIDENT_DECODER_OWNER != 1 || !defined(PIONEER_Q38_DECODER_RUNTIME) || PIONEER_Q38_DECODER_RUNTIME != 1 || !defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || PIONEER_Q38_PROJECTION_RESULT_STASH != 1
#error Exact private retained QPS1/decoder runtime owner role required
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Resident decoder is OFFLINE arithmetic integration,not tag42 DEBUG/CPU-reference/Work promotion
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
#if PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || PIONEER_Q38_RESIDENT_DECODER_NATIVE != 1
#error Fixed retained-convolution DEBUG readback requires its separate exact native role
#endif
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
#if PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_INPUT_OWNER) || PIONEER_Q38_GDN_INPUT_OWNER != 1 || !defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || PIONEER_Q38_RESIDENT_DECODER_NATIVE != 1 || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Exact separate GDN-input DEBUG requires SAME native owner,not old convolution DEBUG
#endif
#endif
#include "PioneerQ38ProjectionGraphOwner.h"
#include "PioneerQ38ConvBatchImage.h"
#include "PioneerQ38ConvBatchPlan.h"
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
#if PIONEER_Q38_GDN_INPUT_OWNER != 1 || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error Exact separate resident-input owner role,not old fixed convolution DEBUG
#endif
#include "PioneerQ38GdnInputImage.h"
#include "PioneerQ38GdnInputPlan.h"
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
#if PIONEER_Q38_GDN_RECURRENT_NATIVE != 1 || !defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || PIONEER_Q38_GDN_STATE_INIT_NATIVE != 1 || !defined(PIONEER_Q38_GDN_INPUT_OWNER) || PIONEER_Q38_GDN_INPUT_OWNER != 1 || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
#error Exact SAME native initialized-GDN recurrent owner,not input DEBUG
#endif
#include "PioneerQ38GdnRecurrentImage.h"
#include "PioneerQ38GdnRecurrentPlan.h"
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
#if PIONEER_Q38_GDN_GATED_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || PIONEER_Q38_GDN_RECURRENT_NATIVE != 1 || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE)
#error Separate exact fused QGH1 SAME owner,not old recurrent DEBUG promotion
#endif
#include "PioneerQ38GdnGatedImage.h"
#include "PioneerQ38GdnGatedPlan.h"
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
#if PIONEER_Q38_GDN_RENDEZVOUS_NATIVE != 1 || !defined(PIONEER_Q38_GDN_GATED_NATIVE) || PIONEER_Q38_GDN_GATED_NATIVE != 1
#error Exact separate timed rendezvous requires SAME native gated owner
#endif
#include "PioneerQ38GdnRendezvousImage.h"
#include "PioneerQ38GdnRendezvousPlan.h"
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
#include "PioneerQ38GdnOutputProducer.h"
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
#if PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || PIONEER_Q38_GDN_RENDEZVOUS_NATIVE != 1
#error Exact separate timed-rendezvous DEBUG52 requires SAME native51
#endif
#include "PioneerQ38GdnRendezvousValidationProtocol.h"
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
#if PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || PIONEER_Q38_GDN_RECURRENT_NATIVE != 1 || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
#error Separate exact recurrent DEBUG requires SAME initialized-state owner,never old input DEBUG
#endif
#include "PioneerQ38GdnRecurrentValidationProtocol.h"
#endif
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
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 uint32_t gdnInstalled=0,gdnArmed=0,gdnReleased=0,gdnCompleted=0,gdnHeld=0;
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 uint32_t gdnLayerPublished=0; // result DATA ONLY;cannot mint the private state-owner publication
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
 uint32_t gdnGatedPublished=0; // result DATA ONLY,never a producer capability
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
 uint32_t gdnValidationWorkers=0,gdnValidationWords=0;
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 uint32_t validationPages=0,validationWords=0;
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
 uint32_t gdnReadbackWorkers=0,gdnReadbackWords=0;
#endif
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
 uint32_t observedStatus[3]={},observedPc[2]={}; // captured EXISTING reads only,never post-failure diagnostics
 uint32_t observedWorker=UINT32_MAX;
#endif
};
class PioneerQ38ResidentDecoderOwner {
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
 friend class PioneerQ38GdnOutputProducer;
 PioneerQ38GdnOutputProducer gatedOutput;
 bool MatchesGatedOutput(const PioneerQ38GdnOutputProducer& value)const{
  if(&value!=&gatedOutput || value.owner!=this || !GdnGatedCompleted() || !complete || phase!=12 || !projection || !history
   || !MatchesNativeOwner(*projection,*history) || value.canonical!=projection->canonical || value.ledger!=projection->ledger
   || value.pages!=projection->pages || !(value.identity==projection->identity) || value.token!=projection->token
   || value.position!=projection->position || value.bank!=projection->bank || value.phase!=gdnPhase)return false;
  const auto* workspace=value.canonical->Workspace(value.bank);
  if(!workspace || workspace->bytes!=67108864 || workspace->allocated!=67108864
   || value.inputAddress!=value.canonical->Base+workspace->offset+TT_Q38_GDN_GATED_OUTPUT)return false;
  for(unsigned i=0;i<16;i++)if(value.source[i]!=projection->cohort->authority.source[0][i])return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || projection->cohort->worker[w].image.running || resets[w]!=0x47800
   || (w>0 && w<=96 && (!gdnImage[w] || !gatedReceipted[w] || !verified[w])))return false;
  return true;
 }
 bool PublishGatedOutput(){
  if(gatedOutput.owner || !projection || !GdnGatedCompleted() || !complete || phase!=12)return false;
  auto& p=*projection;const auto* workspace=p.canonical->Workspace(p.bank);if(!workspace)return false;
  gatedOutput.owner=this;gatedOutput.canonical=p.canonical;gatedOutput.ledger=p.ledger;gatedOutput.pages=p.pages;
  gatedOutput.identity=p.identity;gatedOutput.token=p.token;gatedOutput.position=p.position;gatedOutput.bank=p.bank;gatedOutput.phase=gdnPhase;
  gatedOutput.inputAddress=p.canonical->Base+workspace->offset+TT_Q38_GDN_GATED_OUTPUT;
  for(unsigned i=0;i<16;i++)gatedOutput.source[i]=p.cohort->authority.source[0][i];
  return MatchesGatedOutput(gatedOutput);
 }
#endif
 PioneerQ38ProjectionGraphOwner* projection=nullptr;PioneerQ38ConvHistoryBanks* history=nullptr;
 PioneerQ38ConvBatchPlan jobs[120];
 uint32_t command[120][10]={},counts[120][8]={},resets[120]={};
 bool verified[120]={};unsigned phase=0,operationPhase=0;
 bool started=false,unsafe=false,closed=false,consumed=false,complete=false;
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
 PioneerQ38GdnRendezvousPlan gatedJobs[96];bool gatedReceipted[120]={},gatedPublished=false;uint32_t gatedPolls[120]={},gatedClock[120][4]={};
#elif defined(PIONEER_Q38_GDN_GATED_NATIVE)
 PioneerQ38GdnGatedPlan gatedJobs[96];bool gatedReceipted[120]={},gatedPublished=false;
#endif
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 PioneerQ38GdnRecurrentPlan gdnJobs[96];PioneerQ38GdnBanks* recurrent=nullptr;bool gdnImage[120]={};
#else
 PioneerQ38GdnInputPlan gdnJobs[96];bool gdnImage[120]={};
#endif
 bool gdnAttempted=false,gdnComplete=false;unsigned gdnPhase=0;
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 bool readbackTraffic=false,readbackDone=false;
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
 bool gdnReadbackDone=false;
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
 bool gdnRecurrentReadbackDone=false;
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
 bool gdnTimedReadbackDone=false;
#endif
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
 uint32_t observedStatus[3]={},observedPc[2]={};
 uint32_t observedWorker=UINT32_MAX;
#endif
 template<class IO> bool Fail(IO& io,PioneerQ38ResidentDecoderResult& r){
  unsafe=true;complete=false;r.unsafe=1;
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
  for(unsigned i=0;i<3;i++)r.observedStatus[i]=observedStatus[i];
  for(unsigned i=0;i<2;i++)r.observedPc[i]=observedPc[i];
  r.observedWorker=observedWorker;
#endif
  if(history)history->Fault();
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
  if(recurrent)recurrent->Fault();
#endif
  if(projection)projection->Fail(io,r.stop);
  return false; // MEMORY ONLY;ALL owners/snapshot/maps retained,NO later IO
 }
 template<class IO> bool Scope(IO& io)const{
  return projection && history && MatchesNativeOwner(*projection,*history)
   && projection->Scope(io) && io.ResidentDecoderScope(*this,*projection,*history)
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
   && (!gdnAttempted || (recurrent && io.ResidentGdnRecurrentScope(*this,*projection,*recurrent)))
#endif
   ;
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
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
  // DEBUG transport does NOT pretend that a physically held ROOT runs.
  // Only this owner's one pending fixed read may change its counters.
  if(!w && readbackTraffic)return !p.running[0] && resets[0]==0x47800;
#endif
  if(p.running[w] && !verified[w])return true;
  if(!PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counts[w][i])return false;
  return true;
 }
 template<class IO> bool Image(IO& io,unsigned w){
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
  if(gdnImage[w]){
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   for(unsigned i=0;i<TT_Q38_GDN_RENDEZVOUS_IMAGE_WORDS;i++){
    uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38GdnRendezvousImageAddress(i),v) || v!=PioneerQ38GdnRendezvousImageWord(i))return false;
   }
#elif defined(PIONEER_Q38_GDN_GATED_NATIVE)
   for(unsigned i=0;i<TT_Q38_GDN_GATED_IMAGE_WORDS;i++){
    uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38GdnGatedImageAddress(i),v) || v!=PioneerQ38GdnGatedImageWord(i))return false;
   }
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
   for(unsigned i=0;i<TT_Q38_GDN_RECURRENT_IMAGE_WORDS;i++){
    uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38GdnRecurrentImageAddress(i),v) || v!=PioneerQ38GdnRecurrentImageWord(i))return false;
   }
#else
   for(unsigned i=0;i<TT_Q38_GDN_INPUT_IMAGE_WORDS;i++){
    uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38GdnInputImageAddress(i),v) || v!=PioneerQ38GdnInputImageWord(i))return false;
   }
#endif
   return true;
  }
#endif
  for(unsigned i=0;i<TT_Q38_CONV_BATCH_IMAGE_WORDS;i++){
   uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,PioneerQ38ConvBatchImageAddress(i),v) || v!=PioneerQ38ConvBatchImageWord(i))return false;
  }
  return true;
 }
 template<class IO> bool Current(IO& io,unsigned w){
  auto& p=*projection;
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
  if(gdnImage[w]){
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
   if(!w || w>96 || !recurrent || !recurrent->OwnsFirstLayer(this,p.identity,gdnPhase)
    || !gdnJobs[w-1].Matches(*p.canonical,*recurrent,w-1,p.bank,p.identity,p.cohort->authority.source[w],gdnPhase)
#else
   if(!w || w>96 || !gdnJobs[w-1].Matches(*p.canonical,0,w-1,0,p.bank,p.identity,p.cohort->authority.source[w],gdnPhase)
#endif
    || !gdnJobs[w-1].LearnedPagesPresent(p.ledger,p.pages) || !Image(io,w))return false;
   for(unsigned i=0;i<64;i++){uint32_t v;
    if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_GDN_INPUT_DESCRIPTOR+i*4,v) || v!=gdnJobs[w-1].Descriptor()[i])return false;
   }
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
   for(unsigned i=0;i<64;i++){uint32_t v;
    if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_RGDN_DESCRIPTOR+i*4,v) || v!=gdnJobs[w-1].StateDescriptor()[i])return false;
   }
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
   if(!gatedJobs[w-1].Matches(*p.canonical,gdnJobs[w-1].Descriptor(),gdnJobs[w-1].StateDescriptor())
    || !gatedJobs[w-1].LearnedPagePresent(p.ledger,p.pages))return false;
   const auto* gd=gatedJobs[w-1].Descriptor();
   for(unsigned i=0;i<64;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_GDN_GATED_DESCRIPTOR+i*4,v) || v!=gd[i])return false;}
   if(gatedReceipted[w] || !p.running[w])for(unsigned i=0;i<16;i++){
    uint32_t v,expected=gatedReceipted[w]?
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
     PioneerQ38GdnRendezvousReadyWord(gd,i,false)
#else
     PioneerQ38GdnGatedReadyWord(gd,i,false)
#endif
     :0;
    if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_GDN_GATED_READY+i*4,v) || v!=expected)return false;
   } // ALL96 exact zero cells checked BEFORE ANY release;immutable exact stamps after receipt
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   if(gatedReceipted[w] || !p.running[w]){
    uint32_t clock[16];for(unsigned i=0;i<16;i++)if(!io.ProjectionGraphMemoryReadChecked(w,TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4,clock[i]))return false;
    if(gatedReceipted[w]){
     if(!PioneerQ38GdnRendezvousClockReceipt(clock,gd,gatedPolls[w]))return false;
     for(unsigned i=0;i<4;i++)if(clock[1+i]!=gatedClock[w][i])return false;
    }
    else for(unsigned i=0;i<16;i++)if(clock[i])return false;
   }
#endif
#endif
   return true;
  }
#endif
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
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
  if(io.RuntimeStatusRead(2)!=TT_Q38_CONV_BATCH_MARKER || io.RuntimeStatusRead(0)!=0x6400c
   || io.RuntimeStatusRead(1)!=jobs[w].Count()*16)return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
#elif defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
  // Preserve short-circuit order and EXACT original read count. A fault
  // marker/phase/count never causes a new diagnostic device read.
  observedWorker=w;
  for(auto& value:observedStatus)value=0;
  for(auto& value:observedPc)value=0;
  observedStatus[2]=io.RuntimeStatusRead(2);if(observedStatus[2]!=TT_Q38_CONV_BATCH_MARKER)return false;
  observedStatus[0]=io.RuntimeStatusRead(0);if(observedStatus[0]!=0x6400c)return false;
  observedStatus[1]=io.RuntimeStatusRead(1);if(observedStatus[1]!=jobs[w].Count()*16)return false;
  // Blackhole's PC snapshot is from the speculative frontend,not retire.
  // A self-JAL at64038 can expose its unexecuted fallthrough6403c. Accept
  // ONLY after the fresh success marker/phase/exact count above;the running
  // path still rejects6403c,and ALL independent receipts/holds remain below.
  for(unsigned i=0;i<2;i++){io.Pause();observedPc[i]=io.RuntimePcRead();if(!PioneerQ38ConvBatchTerminalPc(observedPc[i]) && observedPc[i]!=0x6403c)return false;}
#else
  if(io.RuntimeStatusRead(2)!=TT_Q38_CONV_BATCH_MARKER || io.RuntimeStatusRead(0)!=0x6400c
   || io.RuntimeStatusRead(1)!=jobs[w].Count()*16)return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38ConvBatchTerminalPc(io.RuntimePcRead()))return false;}
#endif
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
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 template<class IO> bool GdnTerminal(IO& io,unsigned w){
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
  if(!w || w>96 || !gdnImage[w] || io.RuntimeStatusRead(2)!=TT_Q38_GDN_RENDEZVOUS_MARKER
   || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=64)return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
#elif defined(PIONEER_Q38_GDN_GATED_NATIVE)
  if(!w || w>96 || !gdnImage[w] || io.RuntimeStatusRead(2)!=TT_Q38_GDN_GATED_MARKER
   || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=64)return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
  if(!w || w>96 || !gdnImage[w] || io.RuntimeStatusRead(2)!=TT_Q38_GDN_RECURRENT_MARKER
   || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=64)return false;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();if(pc!=0x64038 && pc!=0x6403c)return false;}
#elif defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
  observedWorker=w;
  for(auto& v:observedStatus)v=0;
  for(auto& v:observedPc)v=0;
  if(!w || w>96 || !gdnImage[w])return false;
  observedStatus[2]=io.RuntimeStatusRead(2);if(observedStatus[2]!=TT_Q38_GDN_INPUT_MARKER)return false;
  observedStatus[0]=io.RuntimeStatusRead(0);if(observedStatus[0]!=0x6400c)return false;
  observedStatus[1]=io.RuntimeStatusRead(1);if(observedStatus[1]!=64)return false;
  for(unsigned i=0;i<2;i++){
   io.Pause();observedPc[i]=io.RuntimePcRead();
   if(observedPc[i]!=0x64038 && observedPc[i]!=0x6403c)return false;
  } // speculative fallthrough accepted ONLY AFTER fresh success/phase/count
#else
  if(!w || w>96 || !gdnImage[w] || io.RuntimeStatusRead(2)!=TT_Q38_GDN_INPUT_MARKER
   || io.RuntimeStatusRead(0)!=0x6400c || io.RuntimeStatusRead(1)!=64)return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(io.RuntimePcRead()!=0x64038)return false;}
#endif
  return true;
 }
 template<class IO> bool GdnReceipt(IO& io,unsigned w){
  if(!GdnTerminal(io,w) || !PioneerQ38GraphCsrIdle(io) || !Current(io,w))return false;
  const auto* d=gdnJobs[w-1].Descriptor();uint32_t summary[32],after[8];
  for(unsigned i=0;i<32;i++)summary[i]=io.Q38PanelRead(TT_Q38_GDN_INPUT_SUMMARY+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
  uint32_t stateSummary[32],gatedSummary[32];
  for(unsigned i=0;i<32;i++){stateSummary[i]=io.Q38PanelRead(TT_Q38_RGDN_SUMMARY+i*4);gatedSummary[i]=io.Q38PanelRead(TT_Q38_GDN_GATED_SUMMARY+i*4);}
  const auto* state=gdnJobs[w-1].StateDescriptor();const auto* gd=gatedJobs[w-1].Descriptor();
  if(!PioneerQ38GdnInputSummary(summary,d,counts[w],summary+16)
   || !PioneerQ38ResidentGdnSummary(stateSummary,state,stateSummary+8,stateSummary+16)
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   || !PioneerQ38GdnRendezvousSummary(gatedSummary,gd,d,state,gatedSummary+8,after))return false;
#else
   || !PioneerQ38GdnGatedSummary(gatedSummary,gd,d,state,gatedSummary+8,after))return false;
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
  uint32_t clock[16];for(unsigned i=0;i<16;i++)clock[i]=io.Q38PanelRead(TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4);
  if(!PioneerQ38GdnRendezvousClockReceipt(clock,gd,gatedSummary[5]))return false;
  gatedPolls[w]=gatedSummary[5];
  for(unsigned i=0;i<4;i++)gatedClock[w][i]=clock[1+i]; // private immutable receipt,not a caller checksum
#endif
  for(unsigned i=0;i<8;i++)if(summary[16+i]!=stateSummary[8+i] || stateSummary[16+i]!=gatedSummary[8+i])return false;
  for(unsigned i=0;i<16;i++)if(io.Q38PanelRead(TT_Q38_GDN_GATED_READY+i*4)!=
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   PioneerQ38GdnRendezvousReadyWord(gd,i,false)
#else
   PioneerQ38GdnGatedReadyWord(gd,i,false)
#endif
   )return false;
  const uint32_t last[]={0x8ea00,0,gd[9],gd[4]+gd[43],0,gd[8],0,0x212,256,0};
  gatedReceipted[w]=true; // private real receipt,never caller metadata
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
  uint32_t stateSummary[32];for(unsigned i=0;i<32;i++)stateSummary[i]=io.Q38PanelRead(TT_Q38_RGDN_SUMMARY+i*4);
  const auto* state=gdnJobs[w-1].StateDescriptor();
  if(!PioneerQ38GdnInputSummary(summary,d,counts[w],summary+16)
   || !PioneerQ38ResidentGdnSummary(stateSummary,state,stateSummary+8,after))return false;
  const unsigned delta[]={9,536,11,9,2,512,11,2};
  for(unsigned i=0;i<8;i++)if(summary[16+i]!=stateSummary[8+i] || uint32_t(after[i]-counts[w][i])!=delta[i])return false;
  const uint32_t last[]={0xa4000,0,state[15],state[7]+16384,0,state[13],0,0x212,16384,0};
#else
  if(!PioneerQ38GdnInputSummary(summary,d,counts[w],after))return false;
  const uint32_t last[]={d[15],0,d[41],0x808c0,0,d[9],0,0,64,0};
#endif
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<8;i++)counts[w][i]=after[i];
  for(unsigned i=0;i<10;i++)command[w][i]=last[i];
  verified[w]=true;return Guard(io,w) && GdnTerminal(io,w);
 }
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 // Private one-shot DEBUG transport,inside the SAME real owner and call.
 // ALL120 physical holds/receipts BEFORE first read,BEFORE original restore.
 template<class IO> bool ValidationReadback(IO& io,uint32_t* values,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;
  if(readbackDone || readbackTraffic || r.validationPages || r.validationWords || !p.state
   || !p.state->OwnsExecution(p.identity,&p,p.input) || !p.state->InitializationPublished(p.identity))return false;
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  PioneerQ38ProjectionExtent span[2];const auto* d=jobs[0].Records();
  const unsigned offsets[]={TT_Q38_CONV_OUTPUT,TT_Q38_CONV_NEXT},pages[]={10,30};
  for(unsigned i=0;i<2;i++)if(!p.state->InitializeSpan(p.identity,0,offsets[i]/4096,pages[i],span[i])
   || span[i].bank!=p.bank || span[i].address!=d[4]+offsets[i] || span[i].route!=d[8]
   || span[i].pages!=pages[i] || span[i].bytes!=pages[i]*4096)return false;
  for(unsigned page=0;page<40;page++){
   r.worker=0;r.word=page;
   if(!Guard(io,0) || !Current(io,0))return false;
   // Reuse ONE original-snapshot-covered scratch page,with distinct NaN
   // poison on EVERY read. Stale/partial/foreign data cannot pass finite.
   for(unsigned word=0;word<1024;word++){
    uint32_t actual,poison=0x7fc40000+page*1024+word;
    if(!io.ProjectionGraphMemoryWrite(0,0x80000+word*4,poison)
     || !io.ProjectionGraphMemoryReadChecked(0,0x80000+word*4,actual) || actual!=poison)return false;
   }
   const auto& e=span[page<10?0:1];unsigned within=page<10?page:page-10;
   const uint32_t next[]={e.address+within*4096,0,e.route,0x80000,0,129,0,0,4096,0};
   for(unsigned i=0;i<10;i++){io.ResidentNocWrite(i*4,next[i]);if(io.ResidentNocRead(i*4)!=next[i])return false;command[0][i]=next[i];}
   if(!Guard(io,0))return false;
   uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
   uint32_t expected[8];const unsigned delta[]={1,64,1,1,0,0,1,0};
   for(unsigned i=0;i<8;i++)expected[i]=counts[0][i]+delta[i];
   readbackTraffic=true;io.ResidentNocWrite(0x40,1);bool done=false;
   for(unsigned poll=0;poll<65536;poll++){
    if(!Guard(io,0))return false;
    uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
    bool exact=io.ResidentNocRead(0x40)==0;
    for(unsigned i=0;i<8;i++)exact &= io.ResidentNocRead(PioneerQ38PanelCounter(i))==expected[i];
    if(exact){done=true;break;}io.Pause();
   }
   if(!done)return false;
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=next[i])return false;
   for(unsigned i=0;i<8;i++)counts[0][i]=expected[i];
   readbackTraffic=false;if(!Guard(io,0))return false;
   for(unsigned word=0;word<1024;word++){
    uint32_t value;if(!io.ProjectionGraphMemoryReadChecked(0,0x80000+word*4,value)
     || (value&0x7fffffff)>=0x7f800000)return false;
    values[page*1024+word]=value;r.validationWords++;
   }
   r.validationPages++;if(!Guard(io,0))return false;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  readbackDone=r.validationPages==40 && r.validationWords==40960;return readbackDone;
 }
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
 // Fixed LOCAL output readback ONLY,inside SAME owner/call,ALL120 held.
 // No NoC transfer/current-state read/CPU arithmetic/reference/fallback.
 template<class IO> bool GdnValidationReadback(IO& io,uint32_t* values,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;
  if(!gdnComplete || gdnReadbackDone || r.gdnReadbackWorkers || r.gdnReadbackWords || !p.state
   || !p.state->OwnsExecution(p.identity,&p,p.input) || !p.state->InitializationPublished(p.identity))return false;
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  phase=13;r.phase=13;
  for(unsigned w=1;w<=96;w++){
   r.worker=w;
   if(p.running[w] || resets[w]!=0x47800 || !verified[w] || !gdnImage[w] || !Guard(io,w) || !Current(io,w))return false;
   observedWorker=w;
   for(auto& v:observedStatus)v=0;
   for(auto& v:observedPc)v=0;
   observedStatus[2]=io.RuntimeStatusRead(2);if(observedStatus[2]!=TT_Q38_GDN_INPUT_MARKER)return false;
   observedStatus[0]=io.RuntimeStatusRead(0);if(observedStatus[0]!=0x6400c)return false;
   observedStatus[1]=io.RuntimeStatusRead(1);if(observedStatus[1]!=64)return false;
   for(unsigned i=0;i<322;i++){
    r.word=i;uint32_t value;
    if(!io.ProjectionGraphMemoryReadChecked(w,0x8e000+i*4,value) || (value&0x7fffffff)>=0x7f800000)return false;
    values[(w-1)*322+i]=value;r.gdnReadbackWords++;
   }
   if(!Guard(io,w) || !Current(io,w))return false;
   r.gdnReadbackWorkers++;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  gdnReadbackDone=r.gdnReadbackWorkers==96 && r.gdnReadbackWords==30912;return gdnReadbackDone;
 }
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE
 // Fixed LOCAL reads ONLY after actual private NEXT publication. No NoC
 // commands/current-state reads/arithmetic/CPU reference/fallback. SAME
 // native source/cookie/team/thread/ALLhost owner remains live throughout.
 template<class IO> bool GdnRecurrentValidationMarker(IO& io,unsigned w){
  return w && w<=96 && verified[w] && gdnImage[w] && io.RuntimeStatusRead(2)==TT_Q38_GDN_RECURRENT_MARKER
   && io.RuntimeStatusRead(0)==0x6400c && io.RuntimeStatusRead(1)==64;
 } // terminal PC was proved BEFORE reset/hold;do not reinterpret a reset-core PC as a live terminal
 template<class IO> bool GdnRecurrentValidationReadback(IO& io,uint32_t* values,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;
  if(!GdnLayerCompleted() || gdnRecurrentReadbackDone || r.gdnValidationWorkers || r.gdnValidationWords
   || !p.state || !p.state->OwnsExecution(p.identity,&p,p.input) || !p.state->InitializationPublished(p.identity))return false;
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  phase=13;r.phase=13;
  const unsigned starts[]={0x8e000,0xa0000,0x8e800},words[]={322,8192,64};
  for(unsigned w=1;w<=96;w++){
   r.worker=w;
   if(p.running[w] || resets[w]!=0x47800 || !verified[w] || !gdnImage[w] || !Guard(io,w) || !Current(io,w) || !GdnRecurrentValidationMarker(io,w))return false;
   unsigned at=(w-1)*TT_Q38_GDN_RECURRENT_VALIDATION_HALF_WORDS;
   for(unsigned block=0;block<3;block++)for(unsigned i=0;i<words[block];i++){
    r.word=at;uint32_t value;
    if(!io.ProjectionGraphMemoryReadChecked(w,starts[block]+i*4,value) || (value&0x7fffffff)>=0x7f800000)return false;
    values[at++]=value;r.gdnValidationWords++;
   }
   if(!Guard(io,w) || !Current(io,w) || !GdnRecurrentValidationMarker(io,w))return false;
   r.gdnValidationWorkers++;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  gdnRecurrentReadbackDone=r.gdnValidationWorkers==96 && r.gdnValidationWords==TT_Q38_GDN_RECURRENT_VALIDATION_WORDS;
  return gdnRecurrentReadbackDone && GdnLayerCompleted();
 }
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
 // Fixed LOCAL observer ONLY. SAME accepted image/clock/ready/source
 // receipts and ALL120 held BEFORE numeric reads. No live-PC sampling
 // after reset,NoC commands,current-state reads,CPU arithmetic or grants.
 template<class IO> bool GdnTimedValidationMarker(IO& io,unsigned w){
  return w && w<=96 && verified[w] && gdnImage[w] && gatedReceipted[w]
   && io.RuntimeStatusRead(2)==TT_Q38_GDN_RENDEZVOUS_MARKER
   && io.RuntimeStatusRead(0)==0x6400c && io.RuntimeStatusRead(1)==64;
 }
 template<class IO> bool GdnTimedValidationReadback(IO& io,uint32_t* values,PioneerQ38ResidentDecoderResult& r){
  auto& p=*projection;
  if(!GdnGatedCompleted() || !GdnLayerCompleted() || gdnTimedReadbackDone || r.gdnValidationWorkers || r.gdnValidationWords
   || !p.state || !p.state->OwnsExecution(p.identity,&p,p.input) || !p.state->InitializationPublished(p.identity))return false;
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  phase=13;r.phase=13;
  const unsigned starts[]={0x8e000,0xa0000,0x8e800,0x8ea00},words[]={322,8192,64,64};
  for(unsigned w=1;w<=96;w++){
   r.worker=w;
   if(p.running[w] || resets[w]!=0x47800 || !verified[w] || !gdnImage[w] || !Guard(io,w) || !Current(io,w) || !GdnTimedValidationMarker(io,w))return false;
   unsigned at=(w-1)*TT_Q38_GDN_RENDEZVOUS_VALIDATION_HALF_WORDS;
   for(unsigned block=0;block<4;block++)for(unsigned i=0;i<words[block];i++){
    r.word=at;uint32_t value;
    if(!io.ProjectionGraphMemoryReadChecked(w,starts[block]+i*4,value) || (value&0x7fffffff)>=0x7f800000)return false;
    values[at++]=value;r.gdnValidationWords++;
   }
   if(!Guard(io,w) || !Current(io,w) || !GdnTimedValidationMarker(io,w))return false;
   r.gdnValidationWorkers++;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return false;
  gdnTimedReadbackDone=r.gdnValidationWorkers==96 && r.gdnValidationWords==TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS;
  return gdnTimedReadbackDone && GdnGatedCompleted() && GdnLayerCompleted();
 }
#endif
public:
 PioneerQ38ResidentDecoderOwner()=default;
 PioneerQ38ResidentDecoderOwner(const PioneerQ38ResidentDecoderOwner&)=delete;
 PioneerQ38ResidentDecoderOwner& operator=(const PioneerQ38ResidentDecoderOwner&)=delete;
 bool Unsafe()const{return unsafe || (projection && projection->Unsafe()) || (history && history->Unsafe());}
 bool Closed()const{return closed && !Unsafe();}
 bool Claimed()const{return started;}
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
 const PioneerQ38GdnOutputProducer* GatedOutput()const{return gatedOutput.Current()?&gatedOutput:nullptr;}
#endif
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 bool GdnInputsClaimed()const{return gdnAttempted;}
 bool GdnInputsPrepared()const{return gdnComplete && !Unsafe() && !closed;}
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 bool MatchesRecurrentNativeOwner(const PioneerQ38GdnBanks& state)const{
  return gdnAttempted && recurrent==&state && projection && projection->graph && projection->proof && projection->banks
   && projection->graph->Recurrent(projection->identity)==&state
   && state.MatchesInitializationOwner(*projection->banks,*projection->proof,projection->identity)
   && state.OwnsFirstLayer(this,projection->identity,gdnPhase) && !Unsafe();
 }
 bool GdnLayerCompleted()const{return gdnComplete && !closed && projection && recurrent && MatchesRecurrentNativeOwner(*recurrent) && recurrent->layerNextPublished;}
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
 bool GdnGatedCompleted()const{return gatedPublished && GdnLayerCompleted();}
#endif
 bool MatchesClosedNativeOwner(const PioneerQ38ProjectionGraphOwner& p,const PioneerQ38ConvHistoryBanks& h)const{
  return started && consumed && closed && phase==9 && !unsafe && !complete && projection==&p && history==&h
   && p.decoderOwner==this && h.decoderOwner==this && p.proof && p.banks && p.cohort && p.memory && p.graph && p.canonical && p.input && p.ledger
   && h.banks==p.banks && h.baseline==p.proof && h.canonical==p.canonical && h.reservationOwner==p.graph
   && h.identity==p.identity && h.published && !h.initializationLease && h.Matches(p.identity)
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
   && recurrent && p.graph->Recurrent(p.identity)==recurrent && recurrent->OwnsFirstLayer(this,p.identity,gdnPhase) && recurrent->layerNextPublished
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
   && gatedPublished
#endif
   && p.MatchesClosedNativeOwner(*p.proof,*p.banks,*p.cohort,*p.memory,*p.graph,*p.canonical,*p.input,p.identity,p.ledger,p.pages);
 } // exact CLOSED objects BEFORE legacy END,not a caller clean/ready hint
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
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
   if(w)r.stop.held++; // independent old-cohort physical hold proof above
#endif
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
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
    r.worker=w;
    observedWorker=w;
    for(auto& value:observedStatus)value=0;
    for(auto& value:observedPc)value=0;
    r.word=40000;
    if(!Guard(io,w))return Fail(io,r);
    r.word=40001;
    uint32_t marker=io.RuntimeStatusRead(2);
    observedStatus[2]=marker;
    if(marker && marker!=TT_Q38_CONV_BATCH_MARKER)return Fail(io,r);
    if(marker){
     r.word=40002;
     if(!Guard(io,w))return Fail(io,r);
     r.word=40003;
     if(!Receipt(io,w))return Fail(io,r);
     done[w]=true;finished++;r.completed++;
    }else{
     r.word=40004;
     io.Pause();uint32_t pc=io.RuntimePcRead();
     observedPc[0]=pc;
     if(pc<0x64000 || pc>=0x64000+TT_Q38_CONV_BATCH_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);
    }
#else
    r.worker=w;if(!Guard(io,w))return Fail(io,r);uint32_t marker=io.RuntimeStatusRead(2);
    if(marker && marker!=TT_Q38_CONV_BATCH_MARKER)return Fail(io,r);
    if(marker){if(!Guard(io,w) || !Receipt(io,w))return Fail(io,r);done[w]=true;finished++;r.completed++;}
    else{io.Pause();uint32_t pc=io.RuntimePcRead();if(pc<0x64000 || pc>=0x64000+TT_Q38_CONV_BATCH_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);}
#endif
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
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 // SAME authenticated owner/call/snapshot;ROOT and workers97..119 stay
 // held in QCB1. No host q/k/gates/current-state input,NO second owner.
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 template<class IO> bool PrepareGdnInputs(IO&,PioneerQ38ResidentDecoderResult&){return false;} // no old prep-only promotion
 template<class IO> bool RunGdnRecurrent(IO& io,PioneerQ38GdnBanks& state,PioneerQ38ResidentDecoderResult& r){
#else
 template<class IO> bool PrepareGdnInputs(IO& io,PioneerQ38ResidentDecoderResult& r){
#endif
  if(!started || unsafe || closed || !complete || phase!=6 || gdnAttempted || !projection || !history)return false;
  auto& p=*projection;gdnAttempted=true;gdnPhase=operationPhase+1;
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
  if(p.graph->Recurrent(p.identity)!=&state || !state.MatchesInitializationOwner(*p.banks,*p.proof,p.identity))return Fail(io,r);
  recurrent=&state;
  if(!state.ClaimFirstLayer(this,p.identity,gdnPhase) || !io.ResidentGdnRecurrentScope(*this,p,state))return Fail(io,r);
#endif
  if(r.gdnInstalled || r.gdnArmed || r.gdnReleased || r.gdnCompleted || r.gdnHeld)return Fail(io,r);
  for(unsigned half=0;half<96;half++){
   unsigned w=half+1;
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
   if(!gdnJobs[half].Build(*p.canonical,state,half,p.bank,p.identity,p.cohort->authority.source[w],gdnPhase)
#else
   if(!gdnJobs[half].Build(*p.canonical,0,half,0,p.bank,p.identity,p.cohort->authority.source[w],gdnPhase)
#endif
    || !gdnJobs[half].LearnedPagesPresent(p.ledger,p.pages))return Fail(io,r);
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
   if(!gatedJobs[half].Build(*p.canonical,gdnJobs[half].Descriptor(),gdnJobs[half].StateDescriptor())
    || !gatedJobs[half].LearnedPagePresent(p.ledger,p.pages))return Fail(io,r);
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   for(unsigned i=0;i<TT_Q38_GDN_RENDEZVOUS_IMAGE_WORDS;i++)
    if(!PioneerQ38ProjectionGraphSnapshotContains(w,PioneerQ38GdnRendezvousImageAddress(i)))return Fail(io,r);
#else
   for(unsigned i=0;i<TT_Q38_GDN_GATED_IMAGE_WORDS;i++)
    if(!PioneerQ38ProjectionGraphSnapshotContains(w,PioneerQ38GdnGatedImageAddress(i)))return Fail(io,r);
#endif
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
   for(unsigned i=0;i<TT_Q38_GDN_RECURRENT_IMAGE_WORDS;i++)
    if(!PioneerQ38ProjectionGraphSnapshotContains(w,PioneerQ38GdnRecurrentImageAddress(i)))return Fail(io,r);
#else
   for(unsigned i=0;i<TT_Q38_GDN_INPUT_IMAGE_WORDS;i++)
    if(!PioneerQ38ProjectionGraphSnapshotContains(w,PioneerQ38GdnInputImageAddress(i)))return Fail(io,r);
#endif
  }
  phase=10;r.phase=10;
  // ALL120 independent old image/table/counter/source/physical holds
  // checked BEFORE first replacement byte,not inferred from reply counts.
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
  for(unsigned w=1;w<=96;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   for(unsigned i=0;i<TT_Q38_GDN_RENDEZVOUS_IMAGE_WORDS;i++){
    r.word=i;if(!Write(io,w,PioneerQ38GdnRendezvousImageAddress(i),PioneerQ38GdnRendezvousImageWord(i)))return Fail(io,r);
   }
   for(unsigned i=0;i<16;i++)if(!Write(io,w,TT_Q38_GDN_RENDEZVOUS_CLOCK+i*4,0))return Fail(io,r);
#else
   for(unsigned i=0;i<TT_Q38_GDN_GATED_IMAGE_WORDS;i++){
    r.word=i;if(!Write(io,w,PioneerQ38GdnGatedImageAddress(i),PioneerQ38GdnGatedImageWord(i)))return Fail(io,r);
   }
#endif
   for(unsigned i=0;i<64;i++)if(!Write(io,w,TT_Q38_RGDN_DESCRIPTOR+i*4,gdnJobs[w-1].StateDescriptor()[i])
    || !Write(io,w,TT_Q38_GDN_GATED_DESCRIPTOR+i*4,gatedJobs[w-1].Descriptor()[i]))return Fail(io,r);
   for(unsigned i=0;i<32;i++)if(!Write(io,w,TT_Q38_RGDN_SUMMARY+i*4,0) || !Write(io,w,TT_Q38_GDN_GATED_SUMMARY+i*4,0))return Fail(io,r);
   for(unsigned i=0;i<32;i++)if(!Write(io,w,TT_Q38_GDN_GATED_READY+i*4,0))return Fail(io,r);
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
   for(unsigned i=0;i<TT_Q38_GDN_RECURRENT_IMAGE_WORDS;i++){
    r.word=i;if(!Write(io,w,PioneerQ38GdnRecurrentImageAddress(i),PioneerQ38GdnRecurrentImageWord(i)))return Fail(io,r);
   }
   for(unsigned i=0;i<64;i++)if(!Write(io,w,TT_Q38_RGDN_DESCRIPTOR+i*4,gdnJobs[w-1].StateDescriptor()[i]))return Fail(io,r);
   for(unsigned i=0;i<32;i++)if(!Write(io,w,TT_Q38_RGDN_SUMMARY+i*4,0))return Fail(io,r);
#else
   for(unsigned i=0;i<TT_Q38_GDN_INPUT_IMAGE_WORDS;i++){
    r.word=i;if(!Write(io,w,PioneerQ38GdnInputImageAddress(i),PioneerQ38GdnInputImageWord(i)))return Fail(io,r);
   }
#endif
   const auto* d=gdnJobs[w-1].Descriptor();
   for(unsigned i=0;i<64;i++){r.word=i;if(!Write(io,w,TT_Q38_GDN_INPUT_DESCRIPTOR+i*4,d[i]))return Fail(io,r);}
   for(unsigned i=0;i<32;i++)if(!Write(io,w,TT_Q38_GDN_INPUT_SUMMARY+i*4,0))return Fail(io,r);
   for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
   gdnImage[w]=true;
   if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   r.gdnInstalled++;
  }
  // Complete second pass over ALL120 BEFORE any release. The untouched
  //24 workers are still independently guarded,not discarded/forgotten.
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
  for(unsigned w=1;w<=96;w++){
   r.worker=w;if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);r.gdnArmed++;
  }
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
  uint64_t releaseBegan=io.Q38CohortNow();
  if(releaseBegan>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
#endif
  for(unsigned w=1;w<=96;w++){
   r.worker=w;
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   uint64_t before=io.Q38CohortNow();if(before<releaseBegan || before-releaseBegan>=UINT64_C(30000000))return Fail(io,r);
#endif
   if(!Guard(io,w) || !Current(io,w))return Fail(io,r);
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   uint64_t after=io.Q38CohortNow();if(after<before || after-releaseBegan>=UINT64_C(30000000))return Fail(io,r);
#endif
   verified[w]=false;p.running[w]=p.cohort->worker[w].image.running=true;
   resets[w]=0x46800;io.RuntimeResetWrite(resets[w]);
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
   uint64_t issued=io.Q38CohortNow();if(issued<after || issued-releaseBegan>=UINT64_C(30000000))return Fail(io,r);
#endif
   if(!Guard(io,w))return Fail(io,r);
   r.gdnReleased++;
  }
  phase=11;r.phase=11;bool done[96]={};unsigned finished=0;
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
  uint64_t began=releaseBegan;
#else
  uint64_t began=io.Q38CohortNow();
#endif
  if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned attempt=0;attempt<65536 && finished<96;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   for(unsigned w=1;w<=96;w++)if(!done[w-1]){
    r.worker=w;
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
    observedWorker=w;
    for(auto& v:observedStatus)v=0;
    for(auto& v:observedPc)v=0;
#endif
    if(!Guard(io,w))return Fail(io,r);
    uint32_t marker=io.RuntimeStatusRead(2);
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
    observedStatus[2]=marker;
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
    if(marker && marker!=TT_Q38_GDN_RENDEZVOUS_MARKER)return Fail(io,r);
#elif defined(PIONEER_Q38_GDN_GATED_NATIVE)
    if(marker && marker!=TT_Q38_GDN_GATED_MARKER)return Fail(io,r);
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
    if(marker && marker!=TT_Q38_GDN_RECURRENT_MARKER)return Fail(io,r);
#else
    if(marker && marker!=TT_Q38_GDN_INPUT_MARKER)return Fail(io,r);
#endif
    if(marker){if(!Guard(io,w) || !GdnReceipt(io,w))return Fail(io,r);done[w-1]=true;finished++;r.gdnCompleted++;}
    else{io.Pause();uint32_t pc=io.RuntimePcRead();
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
     observedPc[0]=pc;
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
     if(pc<0x64000 || pc>=0x64000+TT_Q38_GDN_RENDEZVOUS_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);
#elif defined(PIONEER_Q38_GDN_GATED_NATIVE)
     if(pc<0x64000 || pc>=0x64000+TT_Q38_GDN_GATED_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
     if(pc<0x64000 || pc>=0x64000+TT_Q38_GDN_RECURRENT_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);
#else
     if(pc<0x64000 || pc>=0x64000+TT_Q38_GDN_INPUT_TEXT_BYTES || (pc&3) || (pc>=0x6403c && pc<=0x64058))return Fail(io,r);
#endif
    }
   }
   io.Pause();
  }
  if(finished!=96)return Fail(io,r);
  // ALL120 current image/command/counter/source proofs,including the24
  // untouched held workers,BEFORE first post-compute hold.
  for(unsigned w=0;w<120;w++)if(!Guard(io,w) || !Current(io,w)
   || ((!w || w>96) && (p.running[w] || resets[w]!=0x47800)))return Fail(io,r);
  for(unsigned w=1;w<=96;w++)if(!Guard(io,w) || !GdnTerminal(io,w) || !Current(io,w))return Fail(io,r);
  for(unsigned w=1;w<=96;w++){
   r.worker=w;if(!Guard(io,w) || !GdnTerminal(io,w) || !Current(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   p.running[w]=p.cohort->worker[w].image.running=false;p.resets[w]=resets[w];r.gdnHeld++;
  }
  for(unsigned w=0;w<120;w++)if(p.running[w] || resets[w]!=0x47800 || !Guard(io,w) || !Current(io,w))return Fail(io,r);
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
  for(unsigned w=1;w<=96;w++)if(!gatedReceipted[w])return Fail(io,r);
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
  if(!Scope(io) || !state.PublishFirstLayer(this,p.identity,gdnPhase))return Fail(io,r);
  r.gdnLayerPublished=1; // ONLY after ALL96 receipts+ALL120 physical held/current proofs
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
  gatedPublished=true;r.gdnGatedPublished=1; // private owner-bound scratch only,NOT current slot/token
#endif
  gdnComplete=true;phase=12;r.phase=12;
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
  if(!PublishGatedOutput())return Fail(io,r); // SAME real96receipts/ALL120holds,not result flags
#endif
  return true; // new role:private layer0 NEXT,never whole-token/current-slot flip
 }
#endif
 template<class IO> bool Close(IO& io,PioneerQ38ResidentDecoderResult& r
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
  ,uint32_t* values=nullptr
#elif defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
  ,uint32_t* values=nullptr
#elif defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
  ,uint32_t* values=nullptr
#endif
 ){
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
  if(!started || unsafe || closed || !complete || !projection || !history || !gdnComplete || phase!=12)return false;
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
  if(!GdnLayerCompleted())return false;
#endif
#else
  if(!started || unsafe || closed || !complete || phase!=6 || !projection || !history)return false;
#endif
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE
  uintptr_t output=reinterpret_cast<uintptr_t>(values);
  if(!values || (output&3) || output>UINTPTR_MAX-TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS*4
   || r.gdnValidationWorkers || r.gdnValidationWords || gdnTimedReadbackDone)return false;
  if(!GdnTimedValidationReadback(io,values,r))return Fail(io,r);
#elif defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
  uintptr_t output=reinterpret_cast<uintptr_t>(values);
  if(!values || (output&3) || output>UINTPTR_MAX-163840 || r.validationPages || r.validationWords || readbackDone)return false;
  if(!ValidationReadback(io,values,r))return Fail(io,r);
#elif defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE)
  uintptr_t output=reinterpret_cast<uintptr_t>(values);
  if(!values || (output&3) || output>UINTPTR_MAX-123648 || r.gdnReadbackWorkers || r.gdnReadbackWords || gdnReadbackDone)return false;
  if(!GdnValidationReadback(io,values,r))return Fail(io,r);
#elif defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE)
  uintptr_t output=reinterpret_cast<uintptr_t>(values);
  if(!values || (output&3) || output>UINTPTR_MAX-TT_Q38_GDN_RECURRENT_VALIDATION_WORDS*4
   || r.gdnValidationWorkers || r.gdnValidationWords || gdnRecurrentReadbackDone)return false;
  if(!GdnRecurrentValidationReadback(io,values,r))return Fail(io,r);
#endif
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
  if(r.memory.restoredWords!=TT_Q38_PROJECTION_GRAPH_SAVED_WORDS || r.memory.verifiedOriginalWords!=TT_Q38_PROJECTION_GRAPH_SAVED_WORDS || !Guard(io,0)
   || !p.state->CloseExecution(p.identity,&p,p.input))return Fail(io,r);
  p.closed=p.memory->closed=true;p.memory->installed=false;p.ready=false;closed=true;complete=false;phase=9;r.phase=9;r.closed=1;return true;
 }
};
#ifdef PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE
inline bool PioneerQ38GdnOutputProducer::Current()const{return owner && owner->MatchesGatedOutput(*this);}
inline bool PioneerQ38GdnOutputProducer::MatchesCohort(const PioneerQ38DispatchLease& cohort)const{
 return Current() && owner->projection && owner->projection->cohort==&cohort;
}
#endif
static_assert(sizeof(PioneerQ38ResidentDecoderOwner)>327680 && sizeof(PioneerQ38ResidentDecoderOwner)<524288,"actual decoder plans/lifetime HEAP ONLY");
#endif
