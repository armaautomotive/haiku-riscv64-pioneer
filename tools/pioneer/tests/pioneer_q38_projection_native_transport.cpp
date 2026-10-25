/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PROJECTION_MEMORY_FIXTURE_ONLY
#include "pioneer_q38_projection_memory_test.cpp"
#include "pioneer_q38_projection_execution_machine.h"
#include "pioneer_q38_projection_native_transport.h"
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "pioneer_q38_resident_output_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputImages.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputOwner.h"
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
#include "pioneer_q38_gdn_output_execution_machine.h"
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
#include "pioneer_q38_residual_rms_execution_bridge.h"
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
#include "pioneer_q38_ffn_residual_execution_machine.h"
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
#include "pioneer_q38_next_rms_execution_machine.h"
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
#include "pioneer_q38_next_projection_execution_machine.h"
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
#include "pioneer_q38_next_gdn_output_execution_machine.h"
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
#include "pioneer_q38_next_residual_rms_execution_bridge.h"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphOwner.h"
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#include "pioneer_q38_decoder_conv_bridge.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvBatchImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentDecoderOwner.h"
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
#include "pioneer_q38_gdn_input_bridge.h"
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
#include "pioneer_q38_gdn_recurrent_bridge.h"
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
#include "pioneer_q38_gdn_gated_native_bridge.h"
#endif
#endif
#endif
#endif
#if defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
static constexpr unsigned kGdnValidationWords=TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS,kGdnValidationHalfWords=TT_Q38_GDN_RENDEZVOUS_VALIDATION_HALF_WORDS;
#elif defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE)
static constexpr unsigned kGdnValidationWords=TT_Q38_GDN_RECURRENT_VALIDATION_WORDS,kGdnValidationHalfWords=TT_Q38_GDN_RECURRENT_VALIDATION_HALF_WORDS;
#endif
struct ProjectionNativeTransport::Impl:ProjectionMemoryIO,
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 ResidentOutputExecutionFabric
#else
 ProjectionExecutionFabric
#endif
 {
 std::vector<uint32_t>& workspace;std::unique_ptr<ProjectionExecutionMachine> machine;
 unsigned boots=0,starts=0,stops=0,fault=0;bool damaged=false;
 const PioneerQ38ProjectionGraphResult* result=nullptr;
 ProjectionExecutionLearnedBytes* learned=nullptr;
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 std::unique_ptr<ResidentOutputExecutionMachine> resident;
 ResidentOutputNativeStateBytes* residentBytes=nullptr;bool residentStarted[120]={};
 const PioneerQ38ResidentOutputResult* residentResult=nullptr;
 unsigned residentFault=0,residentFence=0,residentRootPcReads=0,residentFrontendSamples=0;bool residentInjected=false;
 bool ResidentAt(unsigned phase,unsigned worker)const{return residentResult && residentResult->phase==phase && current==worker;}
 std::vector<uint32_t> residentExtra;
 uint32_t ResidentRead(unsigned w,uint32_t a){
  assert(w==current);
  if(PioneerQ38ProjectionGraphSnapshotContains(w,a))return FullRead(w,a);
  unsigned offset=0;
  for(unsigned i=0;i<PioneerQ38ResidentOutputSpanCount(w);i++){auto span=PioneerQ38ResidentOutputSnapshotSpan(w,i);
   if(a>=span.address && a-span.address<span.words*4)return residentExtra.at(PioneerQ38ResidentOutputSnapshotBase(w)+offset+(a-span.address)/4);
   offset+=span.words;
  }
  // The pre-existing outer dispatch END audits/restores its FULL legacy
  // 0x42000-byte panel. These untouched bytes are NOT part of the new
  // snapshot or a new mapping grant; preserve their original endpoint.
  if(!(a&3) && a>=0xac000 && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES)return C().Q38PanelRead(a);
  fprintf(stderr,"NATIVE_OUTPUT_READ_UNMAPPED worker%u address%08x reset%08x\n",w,a,C().coreReset);assert(false);return 0;
 }
 void ResidentWrite(unsigned w,uint32_t a,uint32_t value){
  assert(w==current);
  if(PioneerQ38ProjectionGraphSnapshotContains(w,a)){FullWrite(w,a,value);return;}
  unsigned offset=0;
  for(unsigned i=0;i<PioneerQ38ResidentOutputSpanCount(w);i++){auto span=PioneerQ38ResidentOutputSnapshotSpan(w,i);
   if(a>=span.address && a-span.address<span.words*4){residentExtra.at(PioneerQ38ResidentOutputSnapshotBase(w)+offset+(a-span.address)/4)=value;return;}
   offset+=span.words;
  }
  if(!(a&3) && a>=0xac000 && a<TT_Q38_PANEL_VIEW_BASE+TT_Q38_PANEL_VIEW_BYTES){C().Q38PanelWrite(a,value);return;}
  assert(false);
 }
 bool ResidentInstalled(){
  if(!resident || !residentBytes)return false;
  for(unsigned i=0;i<PioneerQ38ResidentOutputCodeWords(current);i++){
   uint32_t a=PioneerQ38ResidentOutputCodeAddress(current,i),expected;
   if(!PioneerQ38ResidentOutputProgramWord(current,a,expected) || ResidentRead(current,a)!=expected)return false;
  }return true; // actual complete installed bytes,NEVER result/phase/ready fields
 }
 bool ReadState(unsigned b,uint32_t a,uint32_t* out,unsigned n)override{return residentBytes && residentBytes->ReadState(b,a,out,n);}
 bool WriteState(unsigned b,uint32_t a,const uint32_t* in,unsigned n)override{return residentBytes && residentBytes->WriteState(b,a,in,n);}
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 std::unique_ptr<GdnOutputExecutionMachine> output;bool outputStarted[120]={};unsigned outputFault=0;bool outputFaulted=false;
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
 const char* residualImage=nullptr;unsigned residualPc=0,residualFault=0;bool residualStarted=false,residualFaulted=false;
 bool ResidualInstalled(){
  if(current || !residualImage || !output || output->Stops()!=119)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(0);i++){
   unsigned a=PioneerQ38ProjectionGraphCodeAddress(0,i);
   uint32_t expected=i<=10240?PioneerQ38ResidualRmsProgramWord(i):PioneerQ38GdnOutputProgramWord(0,a);
   if(FullRead(0,a)!=expected)return false;
  }
  return true; // ACTUAL installed bytes+physical STOP,not result-phase readiness
 }
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 std::unique_ptr<FfnResidualExecutionMachine> ffn;bool ffnStarted[120]={};
 unsigned ffnFault=0;bool ffnFaulted=false,ffnCheckpointDone=false;
 void (*ffnCheckpoint)(void*)=nullptr;void* ffnCheckpointContext=nullptr;
 bool FfnInstalled(){
  if(!ffn || !residualStarted || residualPc!=0x64038 || !output || output->Stops()!=119)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(current);i++){
   unsigned a=PioneerQ38ProjectionGraphCodeAddress(current,i);
   uint32_t expected=PioneerQ38FfnGraphProgramAddress(current,a)?PioneerQ38FfnResidualProgramWord(current,a):0;
   if(FullRead(current,a)!=expected)return false;
  }
  return true; // ACTUAL installed image+actual parent terminal,NOT result phase
 }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 std::unique_ptr<NextRmsExecutionMachine> nextRms;const char* nextRmsImage=nullptr;
 bool nextRmsStarted=false,nextRmsFaulted=false;unsigned nextRmsFault=0;
 bool NextRmsInstalled(){
  if(current || !nextRmsImage || nextRmsStarted || !ffn || ffn->Stops()!=119)return false;
  for(unsigned w=1;w<120;w++)if(core[w]->coreReset!=0x47800)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(0);i++){
   unsigned a=PioneerQ38ProjectionGraphCodeAddress(0,i);
   uint32_t expected=i<=10240?PioneerQ38LearnedRmsProgramWord(i):
    PioneerQ38FfnGraphProgramAddress(0,a)?PioneerQ38FfnResidualProgramWord(0,a):0;
   if(FullRead(0,a)!=expected)return false;
  }
  return true; // actual installed bytes+actual119 STOP+physical holds,not result phase
 }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 std::unique_ptr<NextProjectionExecutionMachine> nextProjection;bool nextProjectionStarted[120]={};
 unsigned nextProjectionFault=0;bool nextProjectionFaulted=false,nextProjectionCheckpointDone=false;
 void (*nextProjectionCheckpoint)(void*)=nullptr;void* nextProjectionCheckpointContext=nullptr;
 bool NextProjectionInstalled(){
  if(!nextProjection || !nextRmsStarted || !nextRms || nextRms->Pc()!=0x64038 || !ffn || ffn->Stops()!=119)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(current);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(current,i),expected=PioneerQ38NextProjectionGraphProgramAddress(current,a)?PioneerQ38NextProjectionGraphProgramWord(current,a):0;
   if(FullRead(current,a)!=expected)return false;
  }return true; // actual complete installed image and actual prior terminal,not result flags
 }
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 bool nextConvRunning[120]={};unsigned nextConvPc[120]={},nextConvStarts=0;
 unsigned nextConvFault=0;bool nextConvFaulted=false;
 bool NextConvInstalled(){
  if(!nextProjection || !nextProjectionStarted[0] || nextProjection->Stops()!=119)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(current);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(current,i),expected=a==0x6000?PioneerQ38ConvBatchImageWord(0):
    a>=0x64000 && a-0x64000<TT_Q38_CONV_BATCH_TEXT_BYTES?PioneerQ38ConvBatchCodeWord((a-0x64000)/4):
    PioneerQ38NextProjectionGraphProgramAddress(current,a)?PioneerQ38NextProjectionGraphProgramWord(current,a):0;
   if(FullRead(current,a)!=expected)return false;
  }
  return true; // actual full installed image+actual119 prior STOP,not result flags
 }
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
 void (*outputCheckpoint)(void*)=nullptr;void* outputContext=nullptr;bool outputCheckpointDone=false;
 unsigned outputNumericReads=0,outputReadbackFault=0,outputReadbackWord=0;bool outputReadbackFaulted=false;
 bool validationFaulted=false,outputCopyReady=false;
 bool OutputReadbackLive()const{
  if(!output || !outputStarted[0] || output->Stops()!=119)return false;
  for(unsigned w=0;w<120;w++)if(core[w]->coreReset!=0x47800)return false;
  return true;
 }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 std::unique_ptr<GatedNativeCohort> nextGated;bool nextGdnStarted[120]={};unsigned nextGdnStarts=0,nextGdnFault=0;bool nextGdnFaulted=false;
 bool NextGdnInstalled(){
  if(!current || current>96 || !gdnImage || !decoderPlan || !decoderHistory || nextConvStarts!=120
   || FullRead(current,TT_Q38_GDN_INPUT_DESCRIPTOR+5*4)!=1)return false;
  for(unsigned w=0;w<120;w++)if(!nextConvRunning[w] || nextConvPc[w]!=0x64038)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(current);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(current,i),expected=a==0x6000?PioneerQ38GdnRendezvousImageWord(0):
    a>=0x64000 && a-0x64000<TT_Q38_GDN_RENDEZVOUS_TEXT_BYTES?PioneerQ38GdnRendezvousImageWord(1+(a-0x64000)/4):
    a>=0x64000 && a-0x64000<TT_Q38_CONV_BATCH_TEXT_BYTES?PioneerQ38ConvBatchCodeWord((a-0x64000)/4):
    PioneerQ38NextProjectionGraphProgramAddress(current,a)?PioneerQ38NextProjectionGraphProgramWord(current,a):0;
   if(PioneerQ38ProjectionGraphProgramImmutable(current,a) && FullRead(current,a)!=expected)return false;
  }
  return true; // actual prior120 CBO executions+actual installed next-layer image/descriptor,not result phase
 }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 std::unique_ptr<NextGdnOutputExecutionMachine> nextOutput;bool nextOutputStarted[120]={};unsigned nextOutputStarts=0,nextOutputFault=0;bool nextOutputFaulted=false;
 bool NextOutputInstalled(){
  if(!nextOutput || !nextGated || nextGdnStarts!=96)return false;
  for(unsigned w=1;w<=96;w++)if(!nextGdnStarted[w] || nextGated->Pc(w)!=0x64038)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(current);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(current,i);
   if(PioneerQ38ProjectionGraphProgramImmutable(current,a) && FullRead(current,a)!=PioneerQ38NextGdnOutputProgramWord(current,a))return false;
  }
  return true; // actual prior96 executions+complete installed QOP2/QOI2 bytes,NEVER result phase/ready flags
 }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 const char* nextResidualImage=nullptr;bool nextResidualStarted=false,nextResidualFaulted=false;
 unsigned nextResidualPc=0,nextResidualStarts=0,nextResidualFault=0,nextResidualFaultTouch=0;
 void NextResidualInject(){assert(!nextResidualFaulted);nextResidualFaulted=true;nextResidualFaultTouch=touches;}
 bool NextResidualInstalled(){
  if(current || !nextResidualImage || nextResidualStarted || !nextOutput || nextOutputStarts!=121
   || nextOutput->Stops()!=119 || nextOutput->Matrices()!=119 || nextOutput->Producers()!=24)return false;
  for(unsigned w=1;w<120;w++)if(core[w]->coreReset!=0x47800)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(0);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(0,i),v=i<=10240?PioneerQ38NextResidualRmsProgramWord(i):PioneerQ38NextGdnOutputProgramWord(0,a);
   if(FullRead(0,a)!=v)return false;
  }
  uint32_t descriptor[64];for(unsigned i=0;i<64;i++)descriptor[i]=FullRead(0,TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4);
  return PioneerQ38NextResidualRmsShape(descriptor) && descriptor[5]==1 && !descriptor[40];
 } // actual prior121starts+119STOP+holds+installed image;NEVER result flags
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 std::unique_ptr<FfnResidualExecutionMachine> nextFfn;bool nextFfnStarted[120]={};unsigned nextFfnStarts=0;
 unsigned nextFfnFault=0,nextFfnFaultTouch=0;bool nextFfnFaulted=false,nextFfnBeginReady=false,nextFfnCheckpointDone=false;
 void (*nextFfnCheckpoint)(void*)=nullptr;void* nextFfnCheckpointContext=nullptr;
 void NextFfnInject(){assert(!nextFfnFaulted);nextFfnFaulted=true;nextFfnFaultTouch=touches;}
 bool NextFfnInstalled(){
  if(!nextFfn || !nextResidualStarted || nextResidualPc!=0x64038 || !nextOutput || nextOutput->Stops()!=119)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(current);i++){
   unsigned a=PioneerQ38ProjectionGraphCodeAddress(current,i);
   uint32_t expected=PioneerQ38FfnGraphProgramAddress(current,a)?PioneerQ38FfnResidualProgramWord(current,a):0;
   if(FullRead(current,a)!=expected)return false;
  }return true; // actual emitted parent+full installed QFG3/QFP2,NEVER result-phase readiness
 }
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
 bool OutputInstalled(){
  if(!output || FullRead(current,0x6000)!=0x0006c06f)return false;
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(current);i++){
   unsigned address=PioneerQ38ProjectionGraphCodeAddress(current,i);
   if(FullRead(current,address)!=PioneerQ38GdnOutputProgramWord(current,address))return false;
  }
  return true; // TEST actual installed bytes,not a result-phase/ready facade
 }
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 const char* decoderImage=nullptr;const PioneerQ38CanonicalModelPlan* decoderPlan=nullptr;DecoderHistoryBytes* decoderHistory=nullptr;
 unsigned decoderBank=7,decoderStarts=0,decoderPc[120]={};bool decoderRunning[120]={};
 const PioneerQ38ResidentDecoderResult* decoderResult=nullptr;unsigned decoderFault=0;bool decoderDamaged=false;
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 const char* gdnImage=nullptr;unsigned gdnStarts=0,gdnFault=0;bool gdnRunning[120]={},gdnDamaged=false;
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
 std::unique_ptr<GatedNativeCohort> gated;
 unsigned gatedDelayWorker=0,gatedDelayQuanta=0;uint64_t gatedDelayHost=0;bool gatedDelayed=false;
#endif
#if defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 unsigned gdnReadbackWords=0,gdnReadbackFault=UINT32_MAX;
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 void (*validationCheckpoint)(void*)=nullptr;void* validationContext=nullptr;bool validationCheckpointDone=false;
 bool gdnRestoreFault=false,gdnRestoreFaulted=false;
#endif
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 std::vector<uint32_t> decoderCompletionReads;unsigned decoderPcReads[120]={},decoderFrontendSnapshots=0;
#endif
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 unsigned validationReads=0,validationFault=0,validationFaultPage=0;bool validationPending=false;
 void ValidationRead(){
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
  assert(!current && OutputReadbackLive() && outputCheckpointDone && outputNumericReads>=3264 && validationReads<5 && !validationPending);
  unsigned page=validationReads++;
  uint32_t base=FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+4*4),route=FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+8*4);
  assert(FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+5*4)==2);
  unsigned offset=TT_Q38_GDN_OUTPUT_RESULT+page*4096;
  const uint32_t expected[]={base+offset,0,route,0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==expected[i]);
  for(unsigned i=0;i<1024;i++)assert(FullRead(0,0x80000+i*4)==0x7fc50000+page*1024+i);
  if(validationReads==validationFaultPage){
   if(validationFault<=6 || validationFault==9)validationFaulted=true; // mutation is performed by THIS transfer
   if(validationFault==6)now+=30000001;
  }
#elif defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
  assert(!current && boots==119 && starts==5 && machine->Stops()==119 && decoderStarts==120 && validationReads<40 && !validationPending);
  for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800 && decoderRunning[w]);
  unsigned page=validationReads++;
  uint32_t base=FullRead(0,TT_Q38_CONV_BATCH_TABLE+4*4),route=FullRead(0,TT_Q38_CONV_BATCH_TABLE+8*4);
  unsigned offset=page<10?TT_Q38_CONV_OUTPUT+page*4096:TT_Q38_CONV_NEXT+(page-10)*4096;
  const uint32_t expected[]={base+offset,0,route,0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==expected[i]);
  for(unsigned i=0;i<1024;i++)assert(FullRead(0,0x80000+i*4)==0x7fc40000+page*1024+i);
#else
  assert(!current && boots==119 && starts==5 && machine->Stops()==119 && validationReads<18 && !validationPending);
  for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800);
  unsigned page=validationReads++;
  uint32_t base=FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+4*4),route=FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+8*4);
  assert(FullRead(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+5*4)==2);
  const uint32_t expected[]={base+TT_Q38_PROJECTION_RESULT_BASE+page*4096,0,route,0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==expected[i]);
  for(unsigned i=0;i<1024;i++)assert(FullRead(0,0x80000+i*4)==0x7fc30000+page*1024+i);
  unsigned offset=TT_Q38_PROJECTION_RESULT_BASE+page*4096;
#endif
  if(validationReads==validationFaultPage && validationFault==1){validationPending=true;return;}
  for(unsigned i=0;i<1024;i++)if(!(validationReads==validationFaultPage && validationFault==2 && i==1023))
   WriteLocal(0,0x80000+i*4,workspace.at(offset/4+i));
  // Pure endpoint bytes/counters ONLY,never independent reference results.
  C().counters[0]++;C().counters[1]+=64;C().accepted++;C().counters[3]++;
  if(validationReads==validationFaultPage && validationFault==3)C().accepted++;
  if(validationReads==validationFaultPage && validationFault==4)C().source[11]^=1;
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
  outputCopyReady=true;
  if(validationReads==validationFaultPage && validationFault==5)C().command[9]^=1;
  if(validationReads==validationFaultPage && validationFault==9)C().writeCounters[0]++;
#endif
 }
#endif
 Impl(const char* root,const char* peer,std::vector<uint32_t>& gddr):workspace(gddr){
  (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
#if defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
  (void)&PioneerQ38GdnOutputValidationLearned;
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
  (void)&PioneerQ38ResidualRmsLearned;
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
  (void)&PioneerQ38FfnResidualLearned;
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
  (void)&PioneerQ38NextRmsLearned;
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
  (void)&PioneerQ38NextProjectionLearned;
#endif
  machine.reset(new ProjectionExecutionMachine(root,peer,*this));track=true;
 }
 uint32_t ReadLocal(unsigned w,uint32_t a)override{unsigned old=current;current=w;
  uint32_t v=
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
   resident?ResidentRead(w,a):
#endif
   FullRead(w,a);current=old;return v;}
 void WriteLocal(unsigned w,uint32_t a,uint32_t v)override{
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  if(resident && !PioneerQ38ProjectionGraphSnapshotContains(w,a)){
   unsigned old=current;current=w;ResidentWrite(w,a,v);current=old;return;
  }
#endif
  assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(a==0x6000)core[w]->entry=v;
  else if(a>=0x70000 && a<0x74000)core[w]->view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=v;
  else data.at(Index(w,a))=v;
 }
 uint32_t NoC(unsigned w,unsigned a)override{return core[w]->ResidentNocRead(a);}
 void SetControls(unsigned w,const uint32_t* command,const uint32_t* counts)override{
  auto& p=*core[w];for(unsigned i=0;i<10;i++)p.command[i]=command[i];
  p.counters[0]=counts[0];p.counters[1]=counts[1];p.accepted=counts[2];p.counters[3]=counts[3];
  p.writeCounters[0]=counts[4];p.writeCounters[1]=counts[5];p.writeCounters[3]=counts[7];
 }
 uint32_t Reset(unsigned w)override{return core[w]->coreReset;}
 std::vector<uint32_t>& Gddr()override{return workspace;}
 ProjectionExecutionLearnedBytes* LearnedBytes()override{return learned;}
 uint32_t LocalRead(uint32_t a){return PioneerQ38ProjectionGraphSnapshotContains(current,a)?FullRead(current,a):C().Q38PanelRead(a);}
 void LocalWrite(uint32_t a,uint32_t v){
  if(PioneerQ38ProjectionGraphSnapshotContains(current,a))FullWrite(current,a,v);else C().Q38PanelWrite(a,v);
 }
 unsigned ResetTarget(){
  // Existing per-worker mock windows normalize their private aperture to
  // (1,2). A selected writable window still addresses current,not ROOT.
  // The actual ALL-held/probe walk programs fixed read-only physical XY.
  bool selected=true;for(unsigned i=0;i<3;i++)selected&=C().tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i);
  if(selected)return current;
  for(unsigned w=0;w<120;w++)for(bool readOnly:{false,true}){
   bool same=true;for(unsigned i=0;i<3;i++)same&=C().tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,PioneerWorkerX(w),PioneerWorkerY(w),readOnly,i);
   if(same)return w;
  }
  assert(false);return 120;
 }
};
ProjectionNativeTransport::ProjectionNativeTransport(const char* root,const char* peer,std::vector<uint32_t>& workspace):implementation(new Impl(root,peer,workspace)){}
ProjectionNativeTransport::~ProjectionNativeTransport(){delete implementation;}
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
void ProjectionNativeTransport::ResidentOutputSources(const char* root,const char* peer,ResidentOutputNativeStateBytes& bytes){
 auto& p=*implementation;assert(!p.resident && p.machine->Stops()==119);
 for(unsigned w=0;w<120;w++)assert(p.core[w]->coreReset==0x47800);
 p.residentExtra.resize(TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS);
 for(unsigned i=0;i<p.residentExtra.size();i++)p.residentExtra[i]=0x3c5a7091U^(i*0x9e3779b9U);
 p.residentBytes=&bytes;p.resident.reset(new ResidentOutputExecutionMachine(root,peer,p));
} // TEST endpoints only:actual STOP/holds are prerequisites,NO native owner or completion is fabricated
uint32_t ProjectionNativeTransport::ResidentOutputRead(uint32_t a){auto& p=*implementation;p.Touch();assert(p.resident);
 auto value=p.ResidentRead(p.current,a);
 if(p.residentFault==8 && !p.residentInjected && p.ResidentAt(5,0) && a==TT_Q38_OUTPUT_COHORT_SHADOW+(TT_Q38_OUTPUT_COHORT_WORDS-1)*4){
  p.residentInjected=true;p.residentFence=p.touches;return value^1;
 }return value;}
void ProjectionNativeTransport::ResidentOutputWrite(uint32_t a,uint32_t v){auto& p=*implementation;p.Touch();assert(p.resident);p.ResidentWrite(p.current,a,v);}
unsigned ProjectionNativeTransport::ResidentOutputBoots()const{return implementation->resident?implementation->resident->Boots():0;}
unsigned ProjectionNativeTransport::ResidentOutputStops()const{return implementation->resident?implementation->resident->Stops():0;}
unsigned ProjectionNativeTransport::ResidentOutputFrontendSamples()const{return implementation->residentFrontendSamples;}
void ProjectionNativeTransport::VerifyResidentOutput(){assert(implementation->resident);implementation->resident->VerifyOutput();}
void ProjectionNativeTransport::ResidentOutputFault(unsigned kind,const PioneerQ38ResidentOutputResult& result){auto& p=*implementation;
 assert(p.resident && !p.residentInjected);p.residentFault=kind;p.residentResult=&result;
}
bool ProjectionNativeTransport::ResidentOutputFaultInjected()const{return implementation->residentInjected;}
unsigned ProjectionNativeTransport::ResidentOutputFaultFence()const{return implementation->residentFence;}
#endif
void ProjectionNativeTransport::SeedRoot(const uint32_t* addresses,const uint32_t* values,unsigned count,
 const uint32_t* source,const uint32_t* command,const uint32_t* counters){
 auto& p=*implementation;assert(!p.boots && !p.starts && !p.current);
 // Exact device state after genuine QRN1. Do not create/rebind a context,
 // borrowed proof, model object, private RMS lease, or completion seal.
 for(unsigned i=0;i<count;i++)p.FullWrite(0,addresses[i],values[i]);
 for(unsigned w=0;w<120;w++){
  auto& c=*p.core[w];c.config=source[0];
  for(unsigned i=0;i<16;i++)c.source[i]=source[i];
  c.source[0]&=~0x10000U; // fixed DRAM NIU source,not worker config register
  if(w)c.config&=~0x10000U;
 }
 p.SetControls(0,command,counters);
}
bool ProjectionNativeTransport::Select(unsigned w){
 auto& p=*implementation;p.Touch();return p.CodeIO::SelectRuntimeCore(w);
}
bool ProjectionNativeTransport::MapPanel(){auto& p=*implementation;p.Touch();return p.C().mapping;}
bool ProjectionNativeTransport::PanelStable(){return implementation->C().mapping;}
uint32_t ProjectionNativeTransport::Read32(Read kind,unsigned a,unsigned b,unsigned c){
 auto& p=*implementation;p.Touch();
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(p.resident && p.residentFault==12 && !p.residentInjected && p.ResidentAt(6,119) && kind==Noc && a==BlackholeDramSourceOffset(11)){
  p.residentInjected=true;p.residentFence=p.touches;return p.C().ResidentNocRead(a)^1;
 }
 if(p.resident && kind==Pc){
  assert(p.residentStarted[p.current] && p.C().coreReset==0x46800);p.resident->Advance(p.current);
  auto pc=p.resident->Pc(p.current);
  if(!p.current && pc==0x72038){
   if(p.residentFault>=14 && p.residentFault<=16 && p.ResidentAt(5,0)){
    p.residentInjected=true;p.residentFence=p.touches;
    return p.residentFault==14?0x72060:p.residentFault==15?0x72040:0x7207c;
   }
   // Model ONLY the debug register's next-fetch report after ACTUAL VM
   // completion. No ready flag,status,product,owner or reset is fabricated.
   if(++p.residentRootPcReads&1){p.residentFrontendSamples++;return 0x7203c;}
  }return pc;
 }
 if(p.resident && (kind==Status || kind==Panel || (kind==Local && a>=2032 && a<2040))){
  // Status/PC polls advance real instructions. Immutable-code verification
  // reads must NOT copy a whole emulated L1 after EVERY instruction word.
  bool poll=kind==Status || kind==Local || (kind==Panel && a==0x74140);
  if(poll && p.residentStarted[p.current] && p.C().coreReset==0x46800)p.resident->Advance(p.current);
  uint32_t address=kind==Status?0x17ffc0+a*4:kind==Panel?a:0x17e000+a*4;
  auto value=p.ResidentRead(p.current,address);
  if(p.residentFault==17 && !p.residentInjected && p.ResidentAt(5,0) && address==0x7203c
   && p.residentStarted[0] && p.resident->Pc(0)==0x72038){
   assert(value==0xffdff06f);p.residentInjected=true;p.residentFence=p.touches;
   return 0x001802b7; // actual old aliased fault-entry word MUST NOT qualify the new image
  }
  if(!p.residentInjected && ((p.residentFault==9 && p.ResidentAt(8,119) && address==0x7417c && p.resident->Stops())
   || (p.residentFault==13 && p.ResidentAt(11,119) && address==0x17ffdc))){
   p.residentInjected=true;p.residentFence=p.touches;return value^1;
  }
  return value;
 }
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 if(p.decoderResult && p.decoderResult->phase==4 && p.decoderRunning[p.current] && (kind==Status || kind==Pc))
  p.decoderCompletionReads.push_back((p.current<<16)|(kind==Status?0x100+a:0x200));
#endif
 switch(kind){
  case Source:return p.C().SourceRead(a);
  case Reset:return p.core[p.ResetTarget()]->coreReset;
  case ComputeNoc:{unsigned target=p.ResetTarget();auto& node=*p.core[target];
   if(a==0x44)return 0x1c611000|BlackholeDramSourceX(node.source,PioneerWorkerX(target))|(PioneerWorkerY(target)<<6);
   if(a==0x48)return 0x10000|target;
   for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i))return i?node.source[i]:node.config;
   if(a==0x40 || a==0x840 || a==0x1040 || a==0x1840 || (a>=0x240 && a<0x280))return 0;
   assert(false);return 0;}
  case Tlb:return p.C().DramTlbRead(a,b);
  case Local:{auto value=a>=2032 && a<2040?p.LocalRead(0x17e000+a*4):p.C().ResidentL1Read(a);
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   if(!p.decoderDamaged && p.decoderResult && p.decoderResult->phase==8 && p.decoderFault==6 && p.current==119 && a==2039){p.decoderDamaged=true;return value^1;}
#endif
   return value;}
  case Code:assert(!a);return p.C().entry;
  case Control:return p.C().RuntimeControlRead(a);
  case Pc:
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(p.nextFfnStarted[p.current]){
    assert(p.C().coreReset==0x46800);
    if(p.nextFfnFault==5 && !p.nextFfnFaulted && p.current==119 && p.nextFfn->Stops()==119){p.NextFfnInject();return 0xdeadc0dc;}
    p.nextFfn->Advance(p.current);return p.nextFfn->Pc(p.current);
   }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
   if(!p.current && p.nextResidualStarted){assert(p.C().coreReset==0x46800);return p.nextResidualPc;}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
   if(p.nextOutputStarted[p.current]){
    assert(p.C().coreReset==0x46800); // ANY held-PC sample is an actual controller bug
    p.nextOutput->Advance(p.current);
    if(p.nextOutputFault==5 && !p.nextOutputFaulted && p.current==119 && p.nextOutput->Stops()==119){p.nextOutputFaulted=true;return 0xdeadc0dc;}
    return p.nextOutput->Pc(p.current);
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
   if(p.nextGdnStarted[p.current]){
    assert(p.C().coreReset==0x46800); // catch ANY held-PC sampling by the actual owner
    if(p.nextGdnFault==5 && !p.nextGdnFaulted && p.current==96 && p.LocalRead(0x17ffc8)==TT_Q38_GDN_RENDEZVOUS_MARKER){p.nextGdnFaulted=true;return 0x64058;}
    return p.nextGated->Pc(p.current);
   }
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
   if(p.nextConvRunning[p.current]){
    assert(p.C().coreReset==0x46800);
    if(p.nextConvFault==5 && !p.nextConvFaulted && p.current==119 && p.decoderResult->phase==34
     && p.LocalRead(0x17ffc8)==TT_Q38_CONV_BATCH_MARKER){p.nextConvFaulted=true;return 0x64058;}
    return p.nextConvPc[p.current];
   }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
   if(p.nextProjectionStarted[p.current]){
    if(p.C().coreReset==0x46800)p.nextProjection->Advance(p.current);
    if(p.nextProjectionFault==5 && !p.nextProjectionFaulted && p.current==119
     && p.decoderResult->phase==29 && p.nextProjection->Stops()==119){p.nextProjectionFaulted=true;return 0xdeadc0dc;}
    return p.nextProjection->Pc(p.current);
   }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
   if(!p.current && p.nextRmsStarted){
    if(p.C().coreReset==0x46800)p.nextRms->Advance();
    if(p.nextRmsFault==5 && !p.nextRmsFaulted && p.nextRms->Pc()==0x64038){p.nextRmsFaulted=true;return 0x64058;}
    return p.nextRms->Pc();
   }
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
   if(p.ffnStarted[p.current]){
    if(p.C().coreReset==0x46800)p.ffn->Advance(p.current);
    if(p.ffnFault==5 && !p.ffnFaulted && p.current==119 && p.decoderResult->phase==21 && p.ffn->Stops()==119){p.ffnFaulted=true;return 0xdeadc0dc;}
    return p.ffn->Pc(p.current);
   }
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
   if(!p.current && p.residualStarted)return p.residualPc;
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
   if(p.outputStarted[p.current]){
    if(p.C().coreReset==0x46800)p.output->Advance(p.current);
    if(p.outputFault==5 && !p.outputFaulted && p.current==119 && p.decoderResult->phase==15 && p.output->Stops()==119){p.outputFaulted=true;return 0xdeadc0dc;}
    return p.output->Pc(p.current);
   }
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
   if(p.gdnRunning[p.current]){
    assert(p.gated && p.C().coreReset==0x46800);
    if(p.current==96 && p.gdnDamaged && p.gdnFault==10)return 0x64058;
    return p.gated->Pc(p.current);
   }
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
   if(p.gdnStarts==96 && p.gdnReadbackWords<kGdnValidationWords){
    bool held=true;for(unsigned w=0;w<120;w++)held=held && p.core[w]->coreReset==0x47800;
    assert(!held && "post-reset/hold readback must NOT sample a live-terminal PC");
   }
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   if(p.decoderRunning[p.current]){
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
    if((p.decoderFault==14 || p.decoderFault==18) && !p.current){
     unsigned sample=++p.decoderPcReads[0];
     if(sample==2)return 0x64058;
     if(p.decoderFault==18 && sample==1)return 0x6403c;
    }
    // TEST endpoint snapshot only,AFTER actual emitted QCB1 instructions.
    // No marker/result/counter/code/authority changes or arithmetic inputs.
    if(p.decoderFault>=15 && p.decoderFault<=17){
     assert(p.decoderPc[p.current]==0x64038);
     unsigned sample=++p.decoderPcReads[p.current];
     if(p.decoderFault==17 || (sample&1)==(p.decoderFault==15?1U:0U)){
      p.decoderFrontendSnapshots++;return 0x6403c;
     }
    }
#endif
    return p.decoderPc[p.current];
   }
#endif
   if(p.C().coreReset==0x46800)p.machine->Advance(p.current);return p.machine->Pc(p.current);
  case Status:
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(p.nextFfnStarted[p.current]){
    if(!p.current && p.C().coreReset==0x46800)p.nextFfn->Advance(0);
    if(!p.current && a==2 && p.LocalRead(0x17ffc8)==TT_Q38_FFN_RESIDUAL_GRAPH_MARKER
     && p.LocalRead(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+5*4)==1){
     if(p.nextFfnCheckpoint && !p.nextFfnCheckpointDone){p.nextFfnCheckpointDone=true;p.nextFfnCheckpoint(p.nextFfnCheckpointContext);}
     if(!p.nextFfnFaulted && (p.nextFfnFault==4 || p.nextFfnFault==8 || p.nextFfnFault==10 || p.nextFfnFault==11)){
      p.NextFfnInject();
      if(p.nextFfnFault==4)p.WriteLocal(0,TT_Q38_FFN_RESIDUAL_GRAPH_STATS+56*4,p.ReadLocal(0,TT_Q38_FFN_RESIDUAL_GRAPH_STATS+56*4)^1);
      if(p.nextFfnFault==8)p.WriteLocal(0,0x68000,p.ReadLocal(0,0x68000)^1);
      if(p.nextFfnFault==10)p.now+=30000001;
      if(p.nextFfnFault==11){assert(p.now);p.now--;}
     }
    }
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
   if(!p.current && p.nextResidualStarted){
    if(a==2 && !p.nextResidualFaulted && (p.nextResidualFault==10 || p.nextResidualFault==11)){
     p.NextResidualInject();if(p.nextResidualFault==10)p.now+=30000001;else{assert(p.now);p.now--;}
    }
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
   if(p.nextOutputStarted[p.current]){
    if(p.C().coreReset==0x46800)p.nextOutput->Advance(p.current);
    if(!p.current && !p.nextOutputFaulted && a==2 && p.LocalRead(0x17ffc8)==TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER){
     bool stop=p.LocalRead(TT_Q38_PROJECTION_GRAPH_MANIFEST+5*4)==2;
     if(!stop && (p.nextOutputFault==4 || p.nextOutputFault==8 || p.nextOutputFault==10 || p.nextOutputFault==11)){
      p.nextOutputFaulted=true;
      if(p.nextOutputFault==4)p.WriteLocal(0,TT_Q38_PROJECTION_GRAPH_STATS+56*4,p.ReadLocal(0,TT_Q38_PROJECTION_GRAPH_STATS+56*4)^1);
      if(p.nextOutputFault==8)p.WriteLocal(0,0x64000,p.ReadLocal(0,0x64000)^1);
      if(p.nextOutputFault==10)p.now+=30000001;
      if(p.nextOutputFault==11){assert(p.now);p.now--;}
     }
     if(stop && p.nextOutputFault==16){p.nextOutputFaulted=true;p.now+=30000001;}
    } // actual terminal before kernel accepts it;NO synthetic completion/receipt
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
   if(p.nextGdnStarted[p.current]){
    if(!p.nextGdnFaulted && p.current==96 && a==2 && p.decoderResult->phase==39
     && p.LocalRead(0x17ffc8)==TT_Q38_GDN_RENDEZVOUS_MARKER && (p.nextGdnFault==11 || p.nextGdnFault==12)){
     p.nextGdnFaulted=true;
     if(p.nextGdnFault==11)p.now+=30000001;else{assert(p.now);p.now--;}
    } // actual completed worker96 marker returned with late/rollback CPU clock;NO forged ready/receipt
    if(!p.nextGdnFaulted && p.current==96 && p.LocalRead(0x17ffc8)==TT_Q38_GDN_RENDEZVOUS_MARKER
     && (p.nextGdnFault==4 || p.nextGdnFault==8 || p.nextGdnFault==9 || p.nextGdnFault==10)){
     p.nextGdnFaulted=true;unsigned address=p.nextGdnFault==4?TT_Q38_RGDN_SUMMARY+4*4:p.nextGdnFault==8?0x72000:
      p.nextGdnFault==9?TT_Q38_GDN_GATED_PEER_READY+2*4:TT_Q38_GDN_RENDEZVOUS_CLOCK+7*4;
     p.WriteLocal(96,address,p.ReadLocal(96,address)^1);
    }
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
   if(p.nextConvRunning[p.current]){
    if(!p.nextConvFaulted && p.current==119 && p.decoderResult->phase==34
     && p.LocalRead(0x17ffc8)==TT_Q38_CONV_BATCH_MARKER && (p.nextConvFault==4 || p.nextConvFault==8)){
     p.nextConvFaulted=true;unsigned address=p.nextConvFault==4?TT_Q38_FFN_RESIDENT_SUMMARY+4*4:0x72000;
     p.WriteLocal(119,address,p.ReadLocal(119,address)^1);
    } // AFTER actual emitted completion,BEFORE the kernel accepts the receipt
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
   if(p.nextProjectionStarted[p.current]){
    if(p.C().coreReset==0x46800)p.nextProjection->Advance(p.current);
    if(!p.current && p.decoderResult->phase==28 && p.LocalRead(0x17ffc8)==TT_Q38_NEXT_PROJECTION_GRAPH_MARKER){
     if(p.nextProjectionCheckpoint && !p.nextProjectionCheckpointDone
      && p.LocalRead(TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST+43*4)==6){
      p.nextProjectionCheckpointDone=true;p.nextProjectionCheckpoint(p.nextProjectionCheckpointContext);
     }
     if(!p.nextProjectionFaulted && (p.nextProjectionFault==4 || p.nextProjectionFault==8)){
      p.nextProjectionFaulted=true;unsigned address=p.nextProjectionFault==4?TT_Q38_NEXT_PROJECTION_GRAPH_STATS+56*4:TT_Q38_NEXT_PROJECTION_GRAPH_SOURCES+11*4;
      p.WriteLocal(0,address,p.ReadLocal(0,address)^1);
     }
    }
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
   if(!p.current && p.nextRmsStarted){
    if(p.C().coreReset==0x46800)p.nextRms->Advance();
    if(!p.nextRmsFaulted && p.nextRms->Pc()==0x64038 && (p.nextRmsFault==4 || p.nextRmsFault==8)){
     p.nextRmsFaulted=true;unsigned address=p.nextRmsFault==4?TT_Q38_FFN_RESIDENT_SUMMARY+4*4:0x72000;
     p.WriteLocal(0,address,p.ReadLocal(0,address)^1);
    }
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
   if(p.ffnStarted[p.current]){
    if(!p.current && p.C().coreReset==0x46800)p.ffn->Advance(0);
    if(!p.current && p.decoderResult->phase==20 && p.LocalRead(0x17ffc8)==TT_Q38_FFN_RESIDUAL_GRAPH_MARKER){
     if(p.ffnCheckpoint && !p.ffnCheckpointDone){p.ffnCheckpointDone=true;p.ffnCheckpoint(p.ffnCheckpointContext);}
     if(!p.ffnFaulted && (p.ffnFault==4 || p.ffnFault==8)){
      p.ffnFaulted=true;unsigned address=p.ffnFault==4?TT_Q38_FFN_RESIDUAL_GRAPH_STATS+56*4:0x68000;
      p.WriteLocal(0,address,p.ReadLocal(0,address)^1);
     }
    }
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
   if(!p.current && p.residualStarted)return p.LocalRead(0x17ffc0+a*4);
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
   if(p.outputStarted[p.current]){
    if(!p.current && p.C().coreReset==0x46800)p.output->Advance(0);
    if(p.outputFault==4 && !p.outputFaulted && !p.current && p.decoderResult->phase==14
     && p.LocalRead(0x17ffc8)==TT_Q38_GDN_OUTPUT_GRAPH_MARKER){
     p.outputFaulted=true;unsigned address=TT_Q38_PROJECTION_GRAPH_STATS+56*4;p.WriteLocal(0,address,p.ReadLocal(0,address)^1);
    }
    return p.LocalRead(0x17ffc0+a*4);
   }
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
   // TEST mutation only AFTER the actual worker96 reaches its terminal.
   // No completion/readiness/producer bit is fabricated to accelerate it.
   if(!p.gdnDamaged && p.gated && p.gdnStarts==96 && p.current==96
    && p.gated->Pc(96)==0x64038 && p.gdnFault>=3 && p.gdnFault<=14){
    p.gdnDamaged=true;
    if(p.gdnFault==3)p.WriteLocal(96,TT_Q38_GDN_GATED_SUMMARY,p.ReadLocal(96,TT_Q38_GDN_GATED_SUMMARY)^1);
    if(p.gdnFault==4)p.C().counters[0]++;
    if(p.gdnFault==5)p.C().source[11]^=1;
    if(p.gdnFault==6)p.C().command[9]^=1;
    if(p.gdnFault==7)p.WriteLocal(96,0x17ffc8,0xdeadbeef);
    if(p.gdnFault==8)p.WriteLocal(96,0x17ffc4,0);
    if(p.gdnFault==9)p.WriteLocal(96,0x17ffc0,0);
    if(p.gdnFault==11)p.WriteLocal(96,0x64000,p.ReadLocal(96,0x64000)^1);
    if(p.gdnFault==12)p.WriteLocal(96,TT_Q38_GDN_GATED_DESCRIPTOR+63*4,p.ReadLocal(96,TT_Q38_GDN_GATED_DESCRIPTOR+63*4)^1);
    if(p.gdnFault==13)p.core[0]->counters[0]++;
    if(p.gdnFault==14)p.WriteLocal(119,TT_Q38_CONV_BATCH_TABLE+383*4,p.ReadLocal(119,TT_Q38_CONV_BATCH_TABLE+383*4)^1);
   }
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   if(p.decoderRunning[p.current])return p.LocalRead(0x17ffc0+a*4);
#endif
   if(!p.current && p.C().coreReset==0x46800)p.machine->Advance(0);
   if(p.fault>=3 && p.fault<=8 && !p.damaged && p.starts && !p.current && p.C().coreReset==0x46800 && p.machine->Pc(0)==TT_Q38_DISPATCH_PARK){
    p.damaged=true;if(p.fault==3)p.C().source[11]^=1;if(p.fault==4)p.C().coreReset=0x47800;
    if(p.fault==5)p.C().command[9]^=1;if(p.fault==6)p.C().counters[0]++;
    if(p.fault==7)p.data.at(p.Index(0,0x64000))^=1;if(p.fault==8)p.C().mapping=false;
   }
   return p.LocalRead(0x17ffc0+a*4);
  case Noc:
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
   if(a==0x40 && p.validationPending)return 1;
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
   if(p.validationReads==p.validationFaultPage && ((p.validationFault==7 && a==0x840) || (p.validationFault==8 && a==0x240))){
    p.validationFaulted=true;return 1; // actual bad CSR observation,not an armed intent
   }
#endif
   return p.C().ResidentNocRead(a);
  case Panel:{
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(!p.nextFfnFaulted && p.current==119){
    if(p.nextFfnFault==16 && p.nextFfnStarts==119 && a==0x7414c && p.LocalRead(a)==TT_Q38_FFN_PERSISTENT_MARKER){p.NextFfnInject();p.now+=30000001;}
    if(p.nextFfnFault==13 && p.nextFfnStarts==121 && p.C().coreReset==0x47800 && a==0x64000){p.NextFfnInject();p.now+=30000001;}
   }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
   if(!p.nextResidualFaulted && p.nextResidualStarted && p.current==119 && a==0x64000 && p.core[0]->coreReset==0x47800
    && (p.nextResidualFault==13 || p.nextResidualFault==16)){
    p.NextResidualInject();if(p.nextResidualFault==13)p.now+=30000001;else{assert(p.now);p.now--;}
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
   if(!p.nextOutputFaulted && p.current==119 && p.nextOutputStarts==119 && a==0x7414c
    && p.LocalRead(a)==TT_Q38_FFN_PERSISTENT_MARKER && (p.nextOutputFault==14 || p.nextOutputFault==15)){
    p.nextOutputFaulted=true;if(p.nextOutputFault==14)p.now+=30000001;else{assert(p.now);p.now--;}
   } // actual final boot marker arrives late/with clock rollback,not a fake ready flag
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
   bool outputNumeric=!p.current && p.outputNumericReads<8384 && p.OutputReadbackLive()
    && ((a>=0xa0000 && a<0xa3000) || (a>=0xab000 && a<0xab300) || (p.outputCopyReady && a>=0x80000 && a<0x81000));
   if(outputNumeric && !p.outputNumericReads && !p.outputCheckpointDone){
    assert(a==0xa0000);p.outputCheckpointDone=true;if(p.outputCheckpoint)p.outputCheckpoint(p.outputContext);
   }
#endif
   uint32_t v=p.LocalRead(a);
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
   if(outputNumeric){
    unsigned word=a>=0xab000?3072+(a-0xab000)/4:a>=0xa0000?(a-0xa0000)/4:3264+(p.validationReads-1)*1024+(a-0x80000)/4;
    if(word!=p.outputNumericReads || p.gdnReadbackWords!=829632)
     fprintf(stderr,"DEBUG55_TEST_NUMERIC_ORDER address%08x word%u expected%u gdn%u packets%u\n",a,word,p.outputNumericReads,p.gdnReadbackWords,p.validationReads);
    assert(word==p.outputNumericReads++ && p.gdnReadbackWords==829632);
    if((p.outputReadbackFault==1 && word==p.outputReadbackWord)
     || (p.outputReadbackFault==2 && word==3072+p.outputReadbackWord)
     || (p.outputReadbackFault==3 && word==3264+p.outputReadbackWord)){
     p.outputReadbackFaulted=true;v=0x7fc00000;
    }
   }
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
   if(p.gdnStarts==96 && p.current && p.current<=96 && p.C().coreReset==0x47800
    && p.gdnReadbackWords<kGdnValidationWords){
    unsigned word=UINT32_MAX;
    if(a>=0x8e000 && a<0x8e508)word=(a-0x8e000)/4;
    if(a>=0xa0000 && a<0xa8000)word=322+(a-0xa0000)/4;
    if(a>=0x8e800 && a<0x8e900)word=8514+(a-0x8e800)/4;
#if defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
    if(a>=0x8ea00 && a<0x8eb00)word=8578+(a-0x8ea00)/4;
#endif
    if(word!=UINT32_MAX){
     word+=(p.current-1)*kGdnValidationHalfWords;assert(word==p.gdnReadbackWords);
     for(unsigned w=0;w<120;w++)assert(p.core[w]->coreReset==0x47800); // BEFORE first numeric read,not just per-peer
     if(!word && !p.validationCheckpointDone){
      p.validationCheckpointDone=true;
      if(p.validationCheckpoint)p.validationCheckpoint(p.validationContext);
     } // fork actual SAME live source/cookie/call/initialized state/ALL120 holds;NO producer minted
     if(p.gdnReadbackWords++==p.gdnReadbackFault)v=0x7fc00000;
    }
   }
#endif
#ifdef PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE
   if(p.gdnStarts==96 && p.current && p.current<=96 && p.C().coreReset==0x47800
    && a>=0x8e000 && a<0x8e508 && p.gdnReadbackWords<30912){
    unsigned word=(p.current-1)*322+(a-0x8e000)/4;assert(word==p.gdnReadbackWords);
    if(p.gdnReadbackWords++==p.gdnReadbackFault)v=0x7fc00000;
   }
#endif
   if(p.starts && !p.current && ((p.fault==1 && a==TT_Q38_PROJECTION_GRAPH_STATS+56*4)
    || (p.fault==2 && a==TT_Q38_PROJECTION_GRAPH_STATS+6*4)))v^=0x80000000;return v;}
  case Pci:{auto v=p.C().HostPciRead(a,uint16_t(b),uint8_t(c));
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
   if(p.validationFault==10 && p.validationReads==p.validationFaultPage && !a && b==4){
    p.validationFaulted=true;return v|4; // actual bad PCI observation
   }
#endif
   return v;}
 }
 assert(false);return 0;
}
void ProjectionNativeTransport::Write32(Write kind,unsigned a,uint32_t v,unsigned b){
 auto& p=*implementation;p.Touch();
 switch(kind){
  case SetTlb:p.C().DramTlbWrite(a,b,v);return;
  case SetLocal:
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
   if(p.resident && p.residentFault==11 && !p.residentInjected && p.ResidentAt(10,119) && a==2039){
    p.residentInjected=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(p.nextFfnFault==6 && !p.nextFfnFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.NextFfnInject();p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
   if(p.nextResidualFault==6 && !p.nextResidualFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.NextResidualInject();p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
   if(p.nextOutputFault==6 && !p.nextOutputFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.nextOutputFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
   if(p.nextGdnFault==6 && !p.nextGdnFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.nextGdnFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
   if(p.nextConvFault==6 && !p.nextConvFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.nextConvFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
   if(p.nextProjectionFault==6 && !p.nextProjectionFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.nextProjectionFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
   if(p.nextRmsFault==6 && !p.nextRmsFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.nextRmsFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
   if(p.ffnFault==6 && !p.ffnFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.ffnFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
   if(p.residualFault==6 && !p.residualFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.residualFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
   if(p.outputReadbackFault==4 && !p.outputReadbackFaulted && p.outputNumericReads==8384 && p.current==119 && a==2039){
    p.outputReadbackFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
   if(p.outputFault==6 && !p.outputFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==2039){
    p.outputFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;
   }
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
   if(p.gdnRestoreFault && !p.gdnRestoreFaulted && p.gdnReadbackWords==kGdnValidationWords
    && p.current==119 && a==2039){p.gdnRestoreFaulted=true;p.LocalWrite(0x17e000+a*4,v^1);return;}
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   if(!p.decoderDamaged && p.decoderResult && p.decoderResult->phase==7 && p.decoderFault==5 && p.current==119 && a==2039){p.decoderDamaged=true;p.LocalWrite(0x17e000+a*4,v^1);return;}
#endif
   if(p.fault==10 && p.result && p.result->phase==9 && p.current==119 && a==2039)v^=1;
   if(a>=2032 && a<2040)p.LocalWrite(0x17e000+a*4,v);else p.C().ResidentL1Write(a,v);return;
  case SetCode:assert(!a);
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(p.nextFfnFault==1 && !p.nextFfnFaulted && p.decoderResult && p.decoderResult->phase==50 && !p.current && v==0x0006c06f){
    p.NextFfnInject();p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
   if(p.nextResidualFault==1 && !p.nextResidualFaulted && p.decoderResult && p.decoderResult->phase==47 && !p.current && v==0x0005e06f){
    p.NextResidualInject();p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
   if(p.nextOutputFault==1 && !p.nextOutputFaulted && p.decoderResult && p.decoderResult->phase==42 && !p.current && v==0x0006c06f){
    p.nextOutputFaulted=true;p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
   if(p.nextGdnFault==1 && !p.nextGdnFaulted && p.decoderResult && p.decoderResult->phase==37 && p.current==1 && v==PioneerQ38GdnRendezvousImageWord(0)){
    p.nextGdnFaulted=true;p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
   if(p.nextConvFault==1 && !p.nextConvFaulted && p.decoderResult && p.decoderResult->phase==32 && !p.current && v==PioneerQ38ConvBatchImageWord(0)){
    p.nextConvFaulted=true;p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
   if(p.nextProjectionFault==1 && !p.nextProjectionFaulted && p.decoderResult && p.decoderResult->phase==27 && !p.current && v==0x0006c06f){
    p.nextProjectionFaulted=true;p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
   if(p.nextRmsFault==1 && !p.nextRmsFaulted && p.decoderResult && p.decoderResult->phase==25 && !p.current && v==0x0005e06f){
    p.nextRmsFaulted=true;p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
   if(p.ffnFault==1 && !p.ffnFaulted && p.decoderResult && p.decoderResult->phase==19 && !p.current && v==0x0006c06f){
    p.ffnFaulted=true;p.C().RuntimeCodeWrite(0,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
   if(p.residualFault==1 && !p.residualFaulted && p.decoderResult && p.decoderResult->phase==17 && !p.current && v==0x0005e06f){p.residualFaulted=true;return;}
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
   if(p.outputFault==1 && !p.outputFaulted && p.decoderResult && p.decoderResult->phase==13 && !p.current && v==0x0006c06f){p.outputFaulted=true;return;}
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   if(!p.decoderDamaged && p.decoderFault==1 && p.current==119 && v==PioneerQ38ConvBatchImageWord(0)){p.decoderDamaged=true;return;}
#endif
   p.C().RuntimeCodeWrite(0,v);return;
  case SetStatus:p.LocalWrite(0x17ffc0+a*4,v);return;
  case SetNoc:
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(p.nextFfnFault==15 && p.decoderResult && p.decoderResult->phase==50 && p.current==119 && a==36)p.nextFfnBeginReady=true;
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
   if(a==0x40){assert(v==1);p.ValidationRead();return;}
#endif
   p.C().ResidentNocWrite(a,v);return;
  case SetPanel:
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(p.nextFfnFault==2 && !p.nextFfnFaulted && p.decoderResult && p.decoderResult->phase==51 && !p.current && a==TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+41*4){
    p.NextFfnInject();p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
   if(p.nextResidualFault==2 && !p.nextResidualFaulted && p.decoderResult && p.decoderResult->phase==47 && !p.current && a==TT_Q38_FFN_RESIDENT_DESCRIPTOR+5*4){
    p.NextResidualInject();p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
   if(p.nextOutputFault==2 && !p.nextOutputFaulted && p.decoderResult && p.decoderResult->phase==42 && p.current==119 && a==0x7404c){
    p.nextOutputFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
   if(p.nextGdnFault==2 && !p.nextGdnFaulted && p.decoderResult && p.decoderResult->phase==37 && p.current==1 && a==TT_Q38_GDN_INPUT_DESCRIPTOR+5*4){
    p.nextGdnFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
   if(p.nextConvFault==2 && !p.nextConvFaulted && p.decoderResult && p.decoderResult->phase==32
    && !p.current && a==TT_Q38_CONV_BATCH_TABLE+5*4){
    p.nextConvFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
   if(p.nextProjectionFault==2 && !p.nextProjectionFaulted && p.decoderResult && p.decoderResult->phase==28
    && !p.current && a==TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST+40*4){
    p.nextProjectionFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
   if(p.nextRmsFault==2 && !p.nextRmsFaulted && p.decoderResult && p.decoderResult->phase==25 && !p.current && a==TT_Q38_FFN_RESIDENT_DESCRIPTOR+40*4){
    p.nextRmsFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
   if(p.ffnFault==2 && !p.ffnFaulted && p.decoderResult && p.decoderResult->phase==20 && !p.current && a==TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+41*4){
    p.ffnFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
   if(p.residualFault==2 && !p.residualFaulted && p.decoderResult && p.decoderResult->phase==17 && !p.current && a==TT_Q38_FFN_RESIDENT_DESCRIPTOR+41*4){
    p.residualFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
   if(!p.current && a==0x80000 && (v&0xffffe000)==0x7fc50000)p.outputCopyReady=false;
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
   if(p.outputFault==2 && !p.outputFaulted && p.decoderResult && p.decoderResult->phase==13 && p.current==119 && a==0x7404c){
    p.outputFaulted=true;p.LocalWrite(a,v^1);return;
   }
#endif
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
   if(!p.gdnDamaged && p.decoderStarts==120 && !p.gdnStarts && p.current==96
    && p.gdnFault==2 && a==TT_Q38_GDN_INPUT_DESCRIPTOR && v==TT_Q38_GDN_INPUT_MARKER){p.gdnDamaged=true;return;}
#endif
   p.LocalWrite(a,v);return;
  case SetReset:
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
   if(p.resident){
    if(p.residentFault==10 && !p.residentInjected && p.ResidentAt(9,119) && v==0x87ffff){p.residentInjected=true;return;}
    p.C().coreReset=v;p.C().resets.push_back(v);
    if(v==0x46800){assert(!p.residentStarted[p.current] && p.ResidentInstalled());p.residentStarted[p.current]=true;p.resident->Start(p.current);}
    return;
   }
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
   if(v==0x46800 && p.gated && p.current==p.gatedDelayWorker && !p.gatedDelayed){
    assert(!p.gdnRunning[p.current] && p.C().coreReset==0x87ffff);p.gatedDelayed=true;
    for(unsigned i=0;i<p.gatedDelayQuanta;i++)p.gated->Advance(); // actually execute already-released streams while this peer remains HELD
    p.now+=p.gatedDelayHost;
   }
#endif
   p.C().coreReset=v;p.C().resets.push_back(v);
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
   if(p.nextFfnFault==12 && !p.nextFfnFaulted && p.current==119 && p.nextFfnStarts==121 && p.nextFfn->Stops()==119 && v==0x87ffff){p.NextFfnInject();p.now+=30000001;}
   if(v==0x46800 && p.NextFfnInstalled()){
    assert(!p.current || !p.nextFfnStarted[p.current]);
    p.nextFfn->Start(p.current);p.nextFfnStarted[p.current]=true;p.nextFfnStarts++;
    if(!p.nextFfnFaulted && ((p.nextFfnFault==3 && !p.current) || (p.nextFfnFault==7 && p.current==1))){
     p.NextFfnInject();if(p.nextFfnFault==3)p.C().source[11]^=1;else p.now+=30000001;
    }return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
   if(!p.current && p.nextResidualStarted && v==0x87ffff && p.nextResidualFault==12 && !p.nextResidualFaulted){
    p.NextResidualInject();p.now+=30000001;
   }
   if(v==0x46800 && p.NextResidualInstalled()){
    assert(p.decoderPlan && p.learned && p.learned->Ready());
    p.nextResidualStarted=true;p.nextResidualStarts++;
    p.nextResidualPc=ExecuteNextResidualRmsNativeBytes(p.nextResidualImage,*p.decoderPlan,p,p.nextResidualFault==9);
    if(p.nextResidualFault>=3 && p.nextResidualFault<=9 && p.nextResidualFault!=6 && !p.nextResidualFaulted){
     p.NextResidualInject();
     if(p.nextResidualFault==3)p.C().source[11]^=1;
     if(p.nextResidualFault==4)p.WriteLocal(0,TT_Q38_FFN_RESIDENT_SUMMARY+4*4,p.ReadLocal(0,TT_Q38_FFN_RESIDENT_SUMMARY+4*4)^1);
     if(p.nextResidualFault==5)p.nextResidualPc=0x64058;
     if(p.nextResidualFault==7)p.now+=30000001;
     if(p.nextResidualFault==8)p.WriteLocal(0,0x72000,p.ReadLocal(0,0x72000)^1);
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
   if(p.nextOutputFault==12 && !p.nextOutputFaulted && p.current==119 && p.nextOutputStarted[119]
    && p.decoderResult->phase==45 && v==0x87ffff){p.nextOutputFaulted=true;p.now+=30000001;}
   if(v==0x46800 && p.NextOutputInstalled()){
    assert(!p.current || !p.nextOutputStarted[p.current]);
    p.nextOutput->Start(p.current);p.nextOutputStarted[p.current]=true;p.nextOutputStarts++;
    if(!p.nextOutputFaulted){
     if(p.current==1 && (p.nextOutputFault==3 || p.nextOutputFault==7)){
      p.nextOutputFaulted=true;if(p.nextOutputFault==3)p.C().source[11]^=1;else p.now+=30000001;
     }
     if(!p.current && ((p.nextOutputStarts==120 && p.nextOutputFault==9) || (p.nextOutputStarts==121 && p.nextOutputFault==13))){
      p.nextOutputFaulted=true;p.now+=30000001;
     }
    } // actual stream release precedes fault;NO instruction/results injected
    return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
   if(p.nextGdnFault==13 && !p.nextGdnFaulted && p.current==96 && p.nextGdnStarted[96]
    && p.decoderResult->phase==40 && v==0x87ffff){
    p.nextGdnFaulted=true;p.now+=30000001; // actual final worker's first hold reset BEFORE observing expiry
   }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
   if(v==0x46800 && p.NextGdnInstalled()){
    assert(!p.nextGdnStarted[p.current] && p.learned && p.learned->Ready());
    if(!p.nextGated)p.nextGated.reset(new GatedNativeCohort(p.gdnImage,*p.decoderPlan,p.decoderBank,p,*p.decoderHistory));
    p.nextGated->Start(p.current);p.nextGdnStarted[p.current]=true;p.nextGdnStarts++;
    if(p.current==1 && !p.nextGdnFaulted && (p.nextGdnFault==3 || p.nextGdnFault==7)){
     if(p.nextGdnFault==3)p.C().source[11]^=1;else p.now+=30000001;
     p.nextGdnFaulted=true; // AFTER actual first stream release,BEFORE its emitted instructions
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
   if(v==0x46800 && p.NextConvInstalled()){
    assert(!p.nextConvRunning[p.current] && p.decoderImage && p.decoderPlan && p.decoderHistory && p.learned && p.learned->Ready());
    p.nextConvPc[p.current]=ExecuteDecoderConv(p.decoderImage,*p.decoderPlan,p.current,p.decoderBank,p,*p.decoderHistory);
    p.nextConvRunning[p.current]=true;p.nextConvStarts++;
    if(!p.current && !p.nextConvFaulted && (p.nextConvFault==3 || p.nextConvFault==7)){
     if(p.nextConvFault==3)p.C().source[11]^=1;else p.now+=30000001;
     p.nextConvFaulted=true; // synchronous bridge:AFTER actual first CBO execution,BEFORE its kernel proof
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
   if(v==0x46800 && p.NextProjectionInstalled()){
    assert(!p.current || !p.nextProjectionStarted[p.current]);p.nextProjection->Start(p.current);p.nextProjectionStarted[p.current]=true;
    if(!p.current && !p.nextProjectionFaulted && (p.nextProjectionFault==3 || p.nextProjectionFault==7)){
     if(p.nextProjectionFault==3)p.C().source[11]^=1;else p.now+=30000001;
     p.nextProjectionFaulted=true; // AFTER actual first ROOT release,BEFORE its emitted instructions
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
   if(v==0x46800 && p.NextRmsInstalled()){
    assert(p.decoderPlan && !p.nextRms);p.nextRms.reset(new NextRmsExecutionMachine(p.nextRmsImage,*p.decoderPlan,p,p.nextRmsFault==9));p.nextRmsStarted=true;
    if(p.nextRmsFault==3 || p.nextRmsFault==7){
     if(p.nextRmsFault==3)p.C().source[11]^=1;else p.now+=30000001;
     p.nextRmsFaulted=true; // AFTER actual release/BEFORE first emitted instruction
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
   if(v==0x46800 && p.FfnInstalled()){
    assert(!p.current || !p.ffnStarted[p.current]);
    p.ffn->Start(p.current);p.ffnStarted[p.current]=true;
    if(!p.current && !p.ffnFaulted && (p.ffnFault==3 || p.ffnFault==7)){
     if(p.ffnFault==3)p.C().source[11]^=1;else p.now+=30000001;
     p.ffnFaulted=true; // actual release but BEFORE first emitted ROOT instruction
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
   if(v==0x46800 && p.ResidualInstalled()){
    assert(p.decoderPlan && p.learned && p.learned->Ready() && !p.residualStarted);
    for(unsigned w=1;w<120;w++)assert(p.core[w]->coreReset==0x47800);
    p.residualStarted=true;p.residualPc=ExecuteResidualRmsNativeBytes(p.residualImage,*p.decoderPlan,p,p.residualFault==9);
    if(p.residualFault>=3 && p.residualFault!=6 && !p.residualFaulted){
     if(p.residualFault==3)p.C().source[11]^=1;
     if(p.residualFault==4)p.WriteLocal(0,TT_Q38_FFN_RESIDENT_SUMMARY+4*4,p.ReadLocal(0,TT_Q38_FFN_RESIDENT_SUMMARY+4*4)^1);
     if(p.residualFault==5)p.residualPc=0x64058;
     if(p.residualFault==7)p.now+=30000001;
     if(p.residualFault==8)p.WriteLocal(0,0x72000,p.ReadLocal(0,0x72000)^1);
     p.residualFaulted=true;
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
#if defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
   if(v==0x46800 && p.OutputInstalled()){
#else
   if(v==0x46800 && p.output && p.decoderResult && p.decoderResult->phase>=13){
#endif
    assert(p.FullRead(p.current,0x6000)==0x0006c06f);
    for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(p.current);i++){
     unsigned address=PioneerQ38ProjectionGraphCodeAddress(p.current,i);
     assert(p.FullRead(p.current,address)==PioneerQ38GdnOutputProgramWord(p.current,address));
    }
    p.output->Start(p.current);p.outputStarted[p.current]=true;
    if(p.outputFault==3 && !p.outputFaulted && p.current==119){p.outputFaulted=true;p.C().source[11]^=1;}
    return;
   }
#endif
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
   if(v==0x46800 && p.FullRead(p.current,0x64024)==
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
    PioneerQ38GdnRendezvousImageWord(10)
#else
    PioneerQ38GdnGatedImageWord(10)
#endif
    ){
    assert(p.current && p.current<=96 && p.gdnImage && p.decoderHistory && p.decoderPlan && p.decoderStarts==120 && !p.gdnRunning[p.current]);
    if(!p.gated)p.gated.reset(new GatedNativeCohort(p.gdnImage,*p.decoderPlan,p.decoderBank,p,*p.decoderHistory));
    p.gdnRunning[p.current]=true;p.gdnStarts++;p.gated->Start(p.current);return;
   }
#endif
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
   if(v==0x46800 && p.FullRead(p.current,0x64024)==PioneerQ38GdnRecurrentImageWord(10)){
    assert(p.current && p.current<=96 && p.gdnImage && p.decoderHistory && p.decoderPlan && p.decoderStarts==120 && !p.gdnRunning[p.current]);
    assert(p.core[0]->coreReset==0x47800);for(unsigned w=97;w<120;w++)assert(p.core[w]->coreReset==0x47800);
    p.gdnRunning[p.current]=true;p.gdnStarts++;
    p.decoderPc[p.current]=ExecuteGdnRecurrentNativeBytes(p.gdnImage,*p.decoderPlan,p.current,p.decoderBank,p,*p.decoderHistory);
    if(!p.gdnDamaged && p.current==96 && p.gdnFault>=3 && p.gdnFault<=14){p.gdnDamaged=true;
     if(p.gdnFault==3)p.WriteLocal(96,TT_Q38_RGDN_SUMMARY,p.ReadLocal(96,TT_Q38_RGDN_SUMMARY)^1);
     if(p.gdnFault==4)p.C().counters[0]++;if(p.gdnFault==5)p.C().source[11]^=1;if(p.gdnFault==6)p.C().command[9]^=1;
     if(p.gdnFault==7)p.WriteLocal(96,0x17ffc8,TT_Q38_GDN_RECURRENT_FAULT);
     if(p.gdnFault==8)p.WriteLocal(96,0x17ffc4,0);if(p.gdnFault==9)p.WriteLocal(96,0x17ffc0,0);
     if(p.gdnFault==10)p.decoderPc[96]=0x64058;if(p.gdnFault==11)p.WriteLocal(96,0x64000,p.ReadLocal(96,0x64000)^1);
     if(p.gdnFault==12)p.WriteLocal(96,TT_Q38_RGDN_DESCRIPTOR+63*4,1);
     if(p.gdnFault==13)p.core[0]->counters[0]++;
     if(p.gdnFault==14)p.WriteLocal(119,TT_Q38_CONV_BATCH_TABLE+383*4,p.ReadLocal(119,TT_Q38_CONV_BATCH_TABLE+383*4)^1);
    }
    return;
   }
#endif
   if(v==0x46800 && p.FullRead(p.current,0x64024)==PioneerQ38GdnInputImageWord(10)){
    assert(p.current && p.current<=96 && p.gdnImage && p.decoderImage && p.decoderPlan && p.decoderHistory
     && p.decoderStarts==120 && p.decoderRunning[p.current] && !p.gdnRunning[p.current]);
    assert(p.core[0]->coreReset==0x47800);
    for(unsigned w=97;w<120;w++)assert(p.core[w]->coreReset==0x47800 && p.decoderRunning[w]);
    p.gdnRunning[p.current]=true;p.gdnStarts++;
    p.decoderPc[p.current]=ExecuteGdnInputNativeBytes(p.gdnImage,*p.decoderPlan,p.current,p.decoderBank,p);
    if(!p.gdnDamaged && p.current==96 && p.gdnFault>=3 && p.gdnFault<=14){p.gdnDamaged=true;
     if(p.gdnFault==3)p.WriteLocal(96,TT_Q38_GDN_INPUT_SUMMARY,p.ReadLocal(96,TT_Q38_GDN_INPUT_SUMMARY)^1);
     if(p.gdnFault==4)p.C().counters[0]++;if(p.gdnFault==5)p.C().source[11]^=1;if(p.gdnFault==6)p.C().command[9]^=1;
     if(p.gdnFault==7)p.WriteLocal(96,0x17ffc8,TT_Q38_GDN_INPUT_FAULT);
     if(p.gdnFault==8)p.WriteLocal(96,0x17ffc4,0);if(p.gdnFault==9)p.WriteLocal(96,0x17ffc0,0);
     if(p.gdnFault==10)p.decoderPc[96]=0x64058;
     if(p.gdnFault==11)p.WriteLocal(96,0x64000,p.ReadLocal(96,0x64000)^1);
     if(p.gdnFault==12)p.WriteLocal(96,TT_Q38_GDN_INPUT_DESCRIPTOR+63*4,1);
     if(p.gdnFault==13)p.core[0]->counters[0]++;
     if(p.gdnFault==14)p.WriteLocal(119,TT_Q38_CONV_BATCH_TABLE+383*4,p.ReadLocal(119,TT_Q38_CONV_BATCH_TABLE+383*4)^1);
    }
    return;
   }
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
   if(v==0x46800 && p.FullRead(p.current,0x6000)==PioneerQ38ConvBatchImageWord(0)){
    assert(p.decoderImage && p.decoderPlan && p.decoderHistory && !p.decoderRunning[p.current]);p.decoderRunning[p.current]=true;p.decoderStarts++;
    p.decoderPc[p.current]=ExecuteDecoderConv(p.decoderImage,*p.decoderPlan,p.current,p.decoderBank,p,*p.decoderHistory);
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
    if(!p.decoderDamaged && p.decoderFault>=7 && p.decoderFault<=14 && p.current==(p.decoderFault==12?119U:0U)){
     p.decoderDamaged=true;
     if(p.decoderFault==7 || p.decoderFault==12)p.WriteLocal(p.current,0x17ffc8,TT_Q38_CONV_BATCH_FAULT);
     if(p.decoderFault==8)p.WriteLocal(0,0x17ffc4,0);
     if(p.decoderFault==9 || p.decoderFault==11){
      p.WriteLocal(0,0x17ffc8,0);p.decoderPc[0]=p.decoderFault==9?0x6000:0x6403c;
     }
     if(p.decoderFault==10)p.decoderPc[0]=0x64058;
     if(p.decoderFault==13)p.WriteLocal(0,0x17ffc0,0x64008);
    }
#endif
    if(!p.decoderDamaged && p.current==119 && p.decoderFault>=2 && p.decoderFault<=4){p.decoderDamaged=true;
     if(p.decoderFault==2)p.C().source[11]^=1;
     if(p.decoderFault==3)p.WriteLocal(119,TT_Q38_FFN_RESIDENT_SUMMARY,p.ReadLocal(119,TT_Q38_FFN_RESIDENT_SUMMARY)^1);
     if(p.decoderFault==4)p.C().counters[0]++;
    }
    return;
   }
#endif
   if(v==0x46800){p.machine->Start(p.current);if(p.current)p.boots++;else p.starts++;}
   if(v==0x47800)p.stops++;return;
 }
 assert(false);
}
uint32_t ProjectionNativeTransport::MappedRead(uint32_t a){auto& p=*implementation;p.Touch();
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(p.resident)return p.ResidentRead(p.current,a);
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 if(p.nextFfnFault==13 && !p.nextFfnFaulted && p.current==119 && p.nextFfnStarts==121 && p.C().coreReset==0x47800 && a==0x64000){p.NextFfnInject();p.now+=30000001;}
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 if(!p.nextResidualFaulted && p.nextResidualStarted && p.current==119 && a==0x64000 && p.core[0]->coreReset==0x47800
  && (p.nextResidualFault==13 || p.nextResidualFault==16)){
  p.NextResidualInject();if(p.nextResidualFault==13)p.now+=30000001;else{assert(p.now);p.now--;}
 }
#endif
 return p.FullRead(p.current,a);}
void ProjectionNativeTransport::MappedWrite(uint32_t a,uint32_t v){auto& p=*implementation;p.Touch();
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 if(p.resident){p.ResidentWrite(p.current,a,v);return;}
#endif
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 if(p.nextFfnFault==2 && !p.nextFfnFaulted && p.decoderResult && p.decoderResult->phase==51 && !p.current && a==TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+41*4){
  p.NextFfnInject();p.FullWrite(p.current,a,v^1);return;
 }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 if(p.nextResidualFault==2 && !p.nextResidualFaulted && p.decoderResult && p.decoderResult->phase==47 && !p.current && a==TT_Q38_FFN_RESIDENT_DESCRIPTOR+5*4){
  p.NextResidualInject();p.FullWrite(p.current,a,v^1);return;
 }
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 if(p.nextProjectionFault==2 && !p.nextProjectionFaulted && p.decoderResult && p.decoderResult->phase==28
  && !p.current && a==TT_Q38_NEXT_PROJECTION_GRAPH_MANIFEST+40*4){
  p.nextProjectionFaulted=true;p.FullWrite(p.current,a,v^1);return;
 }
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 if(p.outputFault==2 && !p.outputFaulted && p.decoderResult && p.decoderResult->phase==13 && p.current==119 && a==0x7404c){
  p.outputFaulted=true;p.FullWrite(p.current,a,v^1);return;
 }
 if(p.outputFault==6 && !p.outputFaulted && p.decoderResult && p.decoderResult->phase==7 && p.current==119 && a==0x17ffdc){
  p.outputFaulted=true;p.FullWrite(p.current,a,v^1);return;
 }
#endif
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
 if(!p.gdnDamaged && p.decoderStarts==120 && !p.gdnStarts && p.current==96
  && p.gdnFault==1 && a==0x64024 && v==
#ifdef PIONEER_Q38_GDN_RENDEZVOUS_NATIVE
  PioneerQ38GdnRendezvousImageWord(10)
#elif defined(PIONEER_Q38_GDN_GATED_NATIVE)
  PioneerQ38GdnGatedImageWord(10)
#elif defined(PIONEER_Q38_GDN_RECURRENT_NATIVE)
  PioneerQ38GdnRecurrentImageWord(10)
#else
  PioneerQ38GdnInputImageWord(10)
#endif
  ){p.gdnDamaged=true;return;}
#endif
 p.FullWrite(p.current,a,v);}
uint32_t* ProjectionNativeTransport::PanelSaved(){auto& p=*implementation;p.Touch();return p.C().Q38PanelSavedWords();}
uint32_t* ProjectionNativeTransport::Backup(unsigned words){auto& p=*implementation;p.Touch();return p.C().HostReadBackup(words);}
void ProjectionNativeTransport::Quarantine(){implementation->quarantined=true;} // MEMORY ONLY
void ProjectionNativeTransport::Pause(){implementation->now++;
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 auto& nf=*implementation;
 if(nf.nextFfnStarts){
  if(nf.nextFfnStarted[0]){if(nf.core[0]->coreReset==0x46800)nf.nextFfn->Advance(0);return;}
  for(unsigned w=1;w<120;w++)if(nf.nextFfnStarted[w] && nf.core[w]->coreReset==0x46800)nf.nextFfn->Advance(w);
  return; // newly released peers advance independently;historical streams never replay
 }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 if(implementation->nextResidualStarted)return; // never replay historical ROOT on the new release
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
 auto& no=*implementation;
 if(no.nextOutputStarts){
  if(no.nextOutputStarted[0]){if(no.core[0]->coreReset==0x46800)no.nextOutput->Advance(0);return;}
  for(unsigned w=1;w<120;w++)if(no.nextOutputStarted[w] && no.core[w]->coreReset==0x46800)no.nextOutput->Advance(w);
  return; // held historical native61 streams MUST NOT run again
 }
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
 if(implementation->nextGated){implementation->nextGated->Advance();return;}
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
 for(unsigned w=0;w<120;w++)if(implementation->nextConvRunning[w])return;
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
 auto& next=*implementation;
 if(next.nextProjection){
  if(next.nextProjectionStarted[0]){if(next.core[0]->coreReset==0x46800)next.nextProjection->Advance(0);return;}
  bool active=false;for(unsigned w=1;w<120;w++)if(next.nextProjectionStarted[w] && next.core[w]->coreReset==0x46800){next.nextProjection->Advance(w);active=true;}
  if(active)return;
 }
#endif
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
 if(implementation->nextRmsStarted){if(implementation->core[0]->coreReset==0x46800)implementation->nextRms->Advance();return;}
#endif
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
 auto& f=*implementation;
 if(f.ffn){
  if(f.ffnStarted[0]){if(f.core[0]->coreReset==0x46800)f.ffn->Advance(0);return;}
  bool active=false;
  for(unsigned w=1;w<120;w++)if(f.ffnStarted[w] && f.core[w]->coreReset==0x46800){f.ffn->Advance(w);active=true;}
  if(active)return; // released FFN boot advances independently of marker/PC polls
 }
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
 auto& p=*implementation;
 if(p.output){
  if(p.outputStarted[0]){if(p.core[0]->coreReset==0x46800)p.output->Advance(0);}
  else for(unsigned w=1;w<120;w++)if(p.outputStarted[w] && p.core[w]->coreReset==0x46800)p.output->Advance(w);
 }
#endif
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
 if(implementation->gated)implementation->gated->Advance(); // already-released streams only;NO wait-for-all shortcut
#endif
}
uint64_t ProjectionNativeTransport::Now()const{auto& p=*implementation;
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
 if(!p.nextFfnFaulted){
  if(p.nextFfnFault==14 && p.nextFfnStarts==121 && p.decoderResult && p.decoderResult->phase==55){p.NextFfnInject();p.now+=30000001;}
  if(p.nextFfnFault==15 && p.nextFfnBeginReady){p.NextFfnInject();return UINT64_MAX-29999999;}
 }
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
 if(p.nextResidualFault==14 && !p.nextResidualFaulted && p.nextResidualStarted && p.decoderResult && p.decoderResult->phase==49){p.NextResidualInject();p.now+=30000001;}
 if(p.nextResidualFault==15 && !p.nextResidualFaulted && p.decoderResult && p.decoderResult->phase==47 && !p.current && p.C().coreReset==0x87ffff){
  p.NextResidualInject();return UINT64_MAX-29999999;
 }
#endif
 return p.starts && p.fault==11?p.now+30000001:p.starts && p.fault==12?0:p.now;}
unsigned ProjectionNativeTransport::Boots()const{return implementation->boots;}
unsigned ProjectionNativeTransport::Starts()const{return implementation->starts;}
unsigned ProjectionNativeTransport::Stops()const{return implementation->stops;}
unsigned ProjectionNativeTransport::Matrices()const{return implementation->machine->Matrices();}
unsigned ProjectionNativeTransport::Producers()const{return implementation->machine->Producers();}
unsigned ProjectionNativeTransport::Touches()const{return implementation->touches;}
void ProjectionNativeTransport::VerifyOutput(){implementation->machine->VerifyOutput();}
void ProjectionNativeTransport::Diagnostic()const{implementation->machine->Diagnostic();}
void ProjectionNativeTransport::Fault(unsigned kind){implementation->fault=kind;}
void ProjectionNativeTransport::Observe(const PioneerQ38ProjectionGraphResult& result){implementation->result=&result;}
void ProjectionNativeTransport::LearnedSource(ProjectionExecutionLearnedBytes& source){
 auto& p=*implementation;assert(!p.learned && !p.boots && !p.starts && source.Ready());p.learned=&source;
}
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
void ProjectionNativeTransport::NextFfnSources(const char* root,const char* peer){
 auto& p=*implementation;assert(root && peer && !p.nextFfn && !p.boots && !p.starts);p.nextFfn.reset(new FfnResidualExecutionMachine(root,peer,p));
}
void ProjectionNativeTransport::VerifyNextFfn(){auto& p=*implementation;assert(p.nextFfn && p.nextFfnStarts==121);p.nextFfn->VerifyOutput();}
unsigned ProjectionNativeTransport::NextFfnStarts()const{return implementation->nextFfnStarts;}
void ProjectionNativeTransport::NextFfnFault(unsigned kind){
 auto& p=*implementation;assert(kind>=1 && kind<=16 && p.nextFfn && !p.nextFfnFault);p.nextFfnFault=kind;
 if(kind==9)p.nextFfn->FaultLearnedTile();
}
bool ProjectionNativeTransport::NextFfnFaultInjected()const{const auto& p=*implementation;return p.nextFfnFaulted || (p.nextFfn && p.nextFfn->FaultInjected());}
unsigned ProjectionNativeTransport::NextFfnFaultTouch()const{return implementation->nextFfnFaultTouch;}
void ProjectionNativeTransport::NextFfnRunTerminalCheckpoint(void (*callback)(void*),void* context){
 auto& p=*implementation;assert(callback && context && p.nextFfn && !p.nextFfnStarted[0] && !p.nextFfnCheckpoint);p.nextFfnCheckpoint=callback;p.nextFfnCheckpointContext=context;
}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE
void ProjectionNativeTransport::NextOutputSources(const char* root,const char* peer){
 auto& p=*implementation;assert(root && peer && !p.nextOutput && !p.boots && !p.starts);p.nextOutput.reset(new NextGdnOutputExecutionMachine(root,peer,p));
}
void ProjectionNativeTransport::VerifyNextOutput(){auto& p=*implementation;assert(p.nextOutput && p.nextOutputStarts==121);p.nextOutput->VerifyOutput();}
unsigned ProjectionNativeTransport::NextOutputStarts()const{return implementation->nextOutputStarts;}
void ProjectionNativeTransport::NextOutputFault(unsigned kind){auto& p=*implementation;assert(kind>=1 && kind<=16 && !p.nextOutputFault && !p.nextOutputStarts);p.nextOutputFault=kind;}
bool ProjectionNativeTransport::NextOutputFaultInjected()const{return implementation->nextOutputFaulted;}
#endif
#ifdef PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE
void ProjectionNativeTransport::NextResidualSource(const char* image){auto& p=*implementation;assert(image && !p.nextResidualImage && !p.boots && !p.starts);p.nextResidualImage=image;}
unsigned ProjectionNativeTransport::NextResidualStarts()const{return implementation->nextResidualStarts;}
void ProjectionNativeTransport::NextResidualFault(unsigned kind){auto& p=*implementation;assert(kind>=1 && kind<=16 && !p.nextResidualFault && !p.nextResidualStarted);p.nextResidualFault=kind;}
bool ProjectionNativeTransport::NextResidualFaultInjected()const{return implementation->nextResidualFaulted;}
unsigned ProjectionNativeTransport::NextResidualFaultTouch()const{return implementation->nextResidualFaultTouch;}
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE
void ProjectionNativeTransport::OutputSources(const char* root,const char* peer){
 auto& p=*implementation;assert(root && peer && !p.output && !p.boots && !p.starts);p.output.reset(new GdnOutputExecutionMachine(root,peer,p));
}
void ProjectionNativeTransport::VerifyGdnOutput(){assert(implementation->output);implementation->output->VerifyOutput();}
void ProjectionNativeTransport::OutputFault(unsigned kind){assert(kind>=1 && kind<=6 && !implementation->outputFault);implementation->outputFault=kind;}
bool ProjectionNativeTransport::OutputFaultInjected()const{return implementation->outputFaulted;}
#endif
#ifdef PIONEER_Q38_RESIDUAL_RMS_NATIVE
void ProjectionNativeTransport::ResidualRmsSource(const char* image){auto& p=*implementation;assert(image && !p.residualImage && !p.residualStarted);p.residualImage=image;}
void ProjectionNativeTransport::ResidualRmsFault(unsigned kind){auto& p=*implementation;assert(kind>=1 && kind<=9 && !p.residualFault && !p.residualStarted);p.residualFault=kind;}
bool ProjectionNativeTransport::ResidualRmsFaultInjected()const{return implementation->residualFaulted;}
#ifdef PIONEER_Q38_FFN_RESIDUAL_NATIVE
void ProjectionNativeTransport::FfnSources(const char* root,const char* peer){
 auto& p=*implementation;assert(root && peer && !p.ffn);p.ffn.reset(new FfnResidualExecutionMachine(root,peer,p));
}
void ProjectionNativeTransport::VerifyFfn(){assert(implementation->ffn);implementation->ffn->VerifyOutput();}
#ifdef PIONEER_Q38_NEXT_RMS_NATIVE
void ProjectionNativeTransport::NextRmsSource(const char* image){auto& p=*implementation;assert(image && !p.nextRmsImage && !p.nextRmsStarted);p.nextRmsImage=image;}
void ProjectionNativeTransport::NextRmsFault(unsigned kind){auto& p=*implementation;assert(kind>=1 && kind<=9 && !p.nextRmsFault && !p.nextRmsStarted);p.nextRmsFault=kind;}
bool ProjectionNativeTransport::NextRmsFaultInjected()const{const auto& p=*implementation;return p.nextRmsFaulted || (p.nextRms && p.nextRms->FaultInjected());}
void ProjectionNativeTransport::VerifyNextRms(){auto& p=*implementation;assert(p.nextRms && p.nextRms->Packets()==8);p.nextRms->Verify();}
#endif
#ifdef PIONEER_Q38_NEXT_PROJECTION_NATIVE
void ProjectionNativeTransport::NextProjectionSources(const char* root,const char* peer){auto& p=*implementation;assert(root && peer && !p.nextProjection);p.nextProjection.reset(new NextProjectionExecutionMachine(root,peer,p));}
void ProjectionNativeTransport::VerifyNextProjections(){auto& p=*implementation;assert(p.nextProjection && p.nextProjection->Matrices()==244 && p.nextProjection->Producers()==80 && p.nextProjection->Stops()==119);p.nextProjection->VerifyOutput();}
void ProjectionNativeTransport::NextProjectionFault(unsigned kind){auto& p=*implementation;assert(kind>=1 && kind<=8 && p.nextProjection && !p.nextProjectionFault);p.nextProjectionFault=kind;}
bool ProjectionNativeTransport::NextProjectionFaultInjected()const{return implementation->nextProjectionFaulted;}
void ProjectionNativeTransport::NextProjectionRunTerminalCheckpoint(void (*callback)(void*),void* context){
 auto& p=*implementation;assert(callback && context && p.nextProjection && !p.nextProjectionStarted[0] && !p.nextProjectionCheckpoint);
 p.nextProjectionCheckpoint=callback;p.nextProjectionCheckpointContext=context;
}
#endif
#ifdef PIONEER_Q38_NEXT_CONV_NATIVE
void ProjectionNativeTransport::NextConvFault(unsigned kind){auto& p=*implementation;
 assert(kind>=1 && kind<=8 && !p.nextConvFault && !p.nextConvStarts && p.nextProjection && p.nextProjection->Stops()==119);
 p.nextConvFault=kind; // endpoint fault scheduling only,NOT producer/state publication
}
bool ProjectionNativeTransport::NextConvFaultInjected()const{return implementation->nextConvFaulted;}
unsigned ProjectionNativeTransport::NextConvStarts()const{return implementation->nextConvStarts;}
void ProjectionNativeTransport::VerifyNextConv(){auto& p=*implementation;assert(p.nextConvStarts==120);
 for(unsigned w=0;w<120;w++)assert(p.nextConvRunning[w] && p.nextConvPc[w]==0x64038 && p.core[w]->coreReset==0x47800);
}
#endif
#ifdef PIONEER_Q38_NEXT_GDN_NATIVE
void ProjectionNativeTransport::NextGdnFault(unsigned kind){auto& p=*implementation;
 assert(kind>=1 && kind<=13 && !p.nextGdnFault && !p.nextGdnStarts && p.nextConvStarts==120);p.nextGdnFault=kind;
}
bool ProjectionNativeTransport::NextGdnFaultInjected()const{return implementation->nextGdnFaulted;}
unsigned ProjectionNativeTransport::NextGdnStarts()const{return implementation->nextGdnStarts;}
void ProjectionNativeTransport::VerifyNextGdn(){auto& p=*implementation;assert(p.nextGated && p.nextGdnStarts==96);
 for(unsigned w=0;w<120;w++)assert(p.core[w]->coreReset==0x47800 && (w && w<=96?p.nextGdnStarted[w] && p.nextGated->Pc(w)==0x64038:!p.nextGdnStarted[w]));
}
#endif
void ProjectionNativeTransport::FfnFault(unsigned kind){
 auto& p=*implementation;assert(kind>=1 && kind<=9 && p.ffn && !p.ffnFault);p.ffnFault=kind;
 if(kind==9)p.ffn->FaultLearnedTile();
}
bool ProjectionNativeTransport::FfnFaultInjected()const{const auto& p=*implementation;return p.ffnFaulted || (p.ffn && p.ffn->FaultInjected());}
void ProjectionNativeTransport::FfnRunTerminalCheckpoint(void (*callback)(void*),void* context){
 auto& p=*implementation;assert(callback && context && p.ffn && !p.ffnStarted[0] && !p.ffnCheckpoint);p.ffnCheckpoint=callback;p.ffnCheckpointContext=context;
}
#endif
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
void ProjectionNativeTransport::DecoderSources(const char* image,const PioneerQ38CanonicalModelPlan& plan,unsigned bank,DecoderHistoryBytes& history){
 auto& p=*implementation;assert(!p.decoderImage && !p.decoderPlan && !p.decoderHistory && !p.boots && !p.starts && bank<7);
 p.decoderImage=image;p.decoderPlan=&plan;p.decoderBank=bank;p.decoderHistory=&history;
}
unsigned ProjectionNativeTransport::DecoderStarts()const{return implementation->decoderStarts;}
#ifdef PIONEER_Q38_GDN_INPUT_OWNER
void ProjectionNativeTransport::GdnInputSource(const char* image){auto& p=*implementation;assert(image && !p.gdnImage && !p.gdnStarts);p.gdnImage=image;}
void ProjectionNativeTransport::GdnInputFault(unsigned kind){assert(kind>=1 && kind<=14);implementation->gdnFault=kind;}
unsigned ProjectionNativeTransport::GdnInputStarts()const{return implementation->gdnStarts;}
bool ProjectionNativeTransport::GdnInputFaultInjected()const{return implementation->gdnDamaged;}
#ifdef PIONEER_Q38_GDN_GATED_NATIVE
void ProjectionNativeTransport::GatedLaunchDelay(unsigned w,unsigned quanta,uint64_t hostMicroseconds){
 auto& p=*implementation;assert(!p.gdnStarts && !p.gatedDelayWorker && w>=2 && w<=96 && quanta<=65536 && hostMicroseconds<=30000100);
 p.gatedDelayWorker=w;p.gatedDelayQuanta=quanta;p.gatedDelayHost=hostMicroseconds;
}
bool ProjectionNativeTransport::GatedLaunchDelayInjected()const{return implementation->gatedDelayed;}
#endif
#if defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
void ProjectionNativeTransport::GdnReadbackFault(unsigned word){assert(word<
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
 kGdnValidationWords
#else
 30912
#endif
 );implementation->gdnReadbackFault=word;}
unsigned ProjectionNativeTransport::GdnValidationReads()const{return implementation->gdnReadbackWords;}
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
void ProjectionNativeTransport::GdnValidationCheckpoint(void (*callback)(void*),void* context){
 auto& p=*implementation;assert(callback && context && !p.gdnStarts && !p.validationCheckpoint);p.validationCheckpoint=callback;p.validationContext=context;
}
void ProjectionNativeTransport::GdnValidationRootFault(){
 auto& p=*implementation;assert(p.gdnStarts==96 && p.validationCheckpointDone && !p.gdnReadbackWords);p.core[0]->counters[0]++;
}
void ProjectionNativeTransport::GdnValidationStatusFault(unsigned field){
 auto& p=*implementation;assert(field<3 && p.gdnStarts==96 && p.validationCheckpointDone && !p.gdnReadbackWords);
 unsigned address=0x17ffc0+field*4;p.WriteLocal(1,address,p.ReadLocal(1,address)^1);
}
void ProjectionNativeTransport::GdnValidationRestoreFault(){
 auto& p=*implementation;assert(p.gdnStarts==96 && p.validationCheckpointDone && !p.gdnReadbackWords);p.gdnRestoreFault=true;
}
bool ProjectionNativeTransport::GdnValidationRestoreInjected()const{return implementation->gdnRestoreFaulted;}
#endif
#endif
void ProjectionNativeTransport::ObserveDecoder(const PioneerQ38ResidentDecoderResult& r){implementation->decoderResult=&r;}
void ProjectionNativeTransport::DecoderFault(unsigned kind){assert(kind>=1 && kind<=
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
 18
#else
 6
#endif
 );implementation->decoderFault=kind;}
#ifdef PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE
std::vector<uint32_t> ProjectionNativeTransport::DecoderCompletionReads()const{return implementation->decoderCompletionReads;}
unsigned ProjectionNativeTransport::DecoderFrontendSnapshots()const{return implementation->decoderFrontendSnapshots;}
#endif
#endif
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
void ProjectionNativeTransport::OutputValidationCheckpoint(void (*callback)(void*),void* context){
 auto& p=*implementation;assert(callback && context && !p.outputCheckpoint && !p.gdnStarts);p.outputCheckpoint=callback;p.outputContext=context;
}
void ProjectionNativeTransport::OutputReadbackFault(unsigned kind,unsigned word){
 auto& p=*implementation;assert(kind>=1 && kind<=4 && !p.outputReadbackFault && p.outputCheckpointDone && !p.outputNumericReads);
 assert(word<(kind==1?3072U:kind==2?192U:kind==3?5120U:1U));p.outputReadbackFault=kind;p.outputReadbackWord=word;
}
bool ProjectionNativeTransport::OutputReadbackFaultInjected()const{return implementation->outputReadbackFaulted;}
unsigned ProjectionNativeTransport::OutputNumericReads()const{return implementation->outputNumericReads;}
bool ProjectionNativeTransport::ValidationFaultInjected()const{return implementation->validationFaulted;}
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE)
void ProjectionNativeTransport::ValidationFault(unsigned kind,unsigned page){
 assert(kind>=1 && kind<=
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
 10
#else
 4
#endif
 && page>=1 && page<=
#ifdef PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE
  5
#elif defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
  40
#else
  18
#endif
 );implementation->validationFault=kind;implementation->validationFaultPage=page;
}
unsigned ProjectionNativeTransport::ValidationReads()const{return implementation->validationReads;}
#endif
