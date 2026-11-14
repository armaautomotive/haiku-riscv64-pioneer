/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION_H
#define PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION_H
#if defined(PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION) && (PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC))
#error Two-core learned-band owner is offline inspection only
#endif
#if !defined(PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION) && !defined(PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC)
#error Two-core learned-band owner is offline inspection only
#endif
#if defined(PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC) && (PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC != 1 \
 || !defined(_KERNEL_MODE) || !defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) || !defined(TT_PIONEER_DRIVER_RELOAD) \
 || PIONEER_DRIVER_BUILD_TAG != 0x2026111300010153ULL)
#error Two-core learned-band owner requires its distinct fixed diagnostic role
#endif
#if (defined(PIONEER_NATIVE_MODEL_BAND_PAIR_INSPECTION) && defined(PIONEER_NATIVE_MODEL_BAND_PAIR_DIAGNOSTIC)) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Two-core learned-band owner requires a distinct diagnostic or offline inspection role
#endif
#include "PioneerNativeModelBandLease.h"

// Two distinct cores, one shared pair of host windows. This fixed diagnostic
// owner is intentionally independent of the single-core lease: that lease's
// all-cores-held condition cannot be used while another core is running.
// Heap allocation required. No destructor I/O; uncertainty retains ownership.
class PioneerNativeModelBandPairInspection {
 using Band=PioneerNativeModelBandLease;
 uint32_t saved[2][Band::MaxSavedWords]={},tlb[2][3]={};
 PioneerNativeBfpResult result[2]={};
 bool attempted=false,owned=false,unknown=false,closed=false,overlapVerified=false;
 static unsigned Y(unsigned i){return 2+i;}
 bool Fail(){unknown=true;for(auto& r:result){r.state=PIONEER_NATIVE_BFP_UNKNOWN;r.cold=1;}return false;}
 template<class IO> bool Select(IO& io,unsigned i){
  return i<2 && BlackholeDramConfigure(io,0,0,1,Y(i),false)
   && BlackholeDramConfigure(io,1,0xffa00000,1,Y(i),false);
 }
 template<class IO> bool Scope(IO& io,const tt_dram_samples_result& baseline,unsigned i,uint32_t reset){
  if(!io.BfpHealthy() || !BlackholeDramSourceStable(io,baseline))return false;
  for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)
   if(io.DramTlbRead(s,w)!=BlackholeDramFixedTlbWord(s?0xffa00000:0,1,Y(i),false,w))return false;
  return io.ResetRead()==reset && io.ResetRead()==reset;
 }
 template<class IO> bool Terminal(IO& io,const tt_dram_samples_result& baseline,unsigned i){
  if(!Scope(io,baseline,i,0x46800))return false;
  auto& r=result[i];r.phase=io.BfpRead(0x17ffc0);r.count=io.BfpRead(0x17ffc4);r.marker=io.BfpRead(0x17ffc8);
  if(r.phase!=0x7000c || r.count!=(0x04000000|5120) || r.marker!=0x3342464e)return false;
  io.Pause();r.pc[0]=io.BfpPcRead();io.Pause();r.pc[1]=io.BfpPcRead();
  for(unsigned pc:r.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
  return Scope(io,baseline,i,0x46800) && io.BfpRead(0x17ffc0)==r.phase
   && io.BfpRead(0x17ffc4)==r.count && io.BfpRead(0x17ffc8)==r.marker;
 }
public:
 const PioneerNativeBfpResult& Result(unsigned i)const{return result[i];} // i must be 0 or 1
 bool OverlapVerified()const{return overlapVerified;}
 void Abandon(){if(owned)Fail();} // bookkeeping only, never accesses the device
 template<class IO,class Image> bool Run(IO& io,tt_dram_samples_result& baseline,unsigned first,unsigned second){
  static_assert(Image::Width==5120 && Image::Phases==4,"fixed real-band image only");
  if(attempted || first>=3 || second>=3 || first==second)return false;
  attempted=true;for(auto& r:result)r.state=PIONEER_NATIVE_BFP_REFUSED;
  if(baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK || baseline.routes_verified!=7
   || !baseline.tlb_restored || !baseline.data_restored || baseline.recovery_required || !BlackholeDramBuildRoutes(baseline))return false;
  for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++){
   tlb[s][w]=io.DramTlbRead(s,w);if(w==2 && (tlb[s][w]&0xffffc000))return false;
  }
  owned=true;if(!BlackholeDramAllCoresHeld(io,baseline))return Fail();
  const unsigned samples[]={first,second},words=Band::SavedWords(5120);
  // Snapshot and verify each complete image before either core is released.
  for(unsigned i=0;i<2;i++){
   if(!Select(io,i) || !Scope(io,baseline,i,0x47800))return Fail();
   for(unsigned k=0;k<3;k++)result[i].controls[k]=io.BfpControlRead(k);
   if((result[i].controls[0]&1) || result[i].controls[1] || result[i].controls[2])return Fail();
   for(unsigned n=0;n<words;n++)saved[i][n]=io.BfpRead(Band::Address(5120,n));
   if(!Scope(io,baseline,i,0x47800))return Fail();
   for(unsigned n=1;n<words;n++)io.BfpWrite(Band::Address(5120,n),Band::StagedWord<Image>(samples[i],5120,n));
   if(!Scope(io,baseline,i,0x47800))return Fail();
   io.BfpWrite(0x6000,Band::StagedWord<Image>(samples[i],5120,0));
   for(unsigned n=0;n<words;n++)if(io.BfpRead(Band::Address(5120,n))!=Band::StagedWord<Image>(samples[i],5120,n))return Fail();
   if(!Scope(io,baseline,i,0x47800))return Fail();
  }
  // A successful per-core write/read does not exclude aliasing between
  // routes. Re-read BOTH staged images only after both have been written.
  // Different pinned samples make an accidental cross-core alias visible.
  for(unsigned i=0;i<2;i++){
   if(!Select(io,i) || !Scope(io,baseline,i,0x47800))return Fail();
   for(unsigned n=0;n<words;n++)
    if(io.BfpRead(Band::Address(5120,n))!=Band::StagedWord<Image>(samples[i],5120,n))return Fail();
   if(!Scope(io,baseline,i,0x47800))return Fail();
  }
  // Recheck the entire fleet after staging, before releasing either core.
  if(!BlackholeDramAllCoresHeld(io,baseline) || !BlackholeDramSourceStable(io,baseline))return Fail();
  // First reset and verify both processors while neither can run. Then
  // release them back-to-back; defer post-release checks to the immediate
  // PC bracket so an avoidable full Scope() on core 0 cannot consume the
  // entire short diagnostic before core 1 starts.
  for(unsigned i=0;i<2;i++){
   if(!Select(io,i) || !Scope(io,baseline,i,0x47800))return Fail();
   io.BfpResetWrite(0x87ffff);io.Pause();if(!Scope(io,baseline,i,0x87ffff))return Fail();
  }
  for(unsigned i=0;i<2;i++){
   if(!Select(io,i) || !Scope(io,baseline,i,0x87ffff))return Fail();
   result[i].started=1;io.BfpResetWrite(0x46800);
  }
  // Bracket peer 1's running interval with two live observations of peer 0.
  // Order is core0, core1, core1, core0: each image has one-way completion
  // into its terminal park, so a live core1 at both middle observations
  // lies inside core0's live interval when both outer observations are live.
  // These same bounded scopes also verify both release writes. If either
  // peer already reached its verified park, simply report no overlap;
  // numerical execution and explicit END remain independently gated.
  uint32_t bracket[2][2]={};
  static const unsigned bracketOrder[4]={0,1,1,0};
  for(unsigned observation=0;observation<4;observation++){
   unsigned sample=bracketOrder[observation];
   if(!Select(io,sample) || !Scope(io,baseline,sample,0x46800))return Fail();
   io.Pause();bracket[sample][observation&1]=io.BfpPcRead();
   if(bracket[sample][observation&1]==0x70040)return Fail();
  }
  auto live=[](uint32_t pc){return pc!=0x70038 && pc!=0x7003c;};
  overlapVerified=live(bracket[0][0]) && live(bracket[0][1])
   && live(bracket[1][0]) && live(bracket[1][1]);
  const uint64_t began=io.BfpNow();bool done[2]={};
  for(unsigned poll=0;poll<20000 && !(done[0] && done[1]);poll++)for(unsigned i=0;i<2;i++)if(!done[i]){
   if(!Select(io,i) || !Scope(io,baseline,i,0x46800))return Fail();
   const uint64_t now=io.BfpNow();if(now<began || now-began>UINT64_C(2000000000))return Fail();
   result[i].polls++;const uint32_t marker=io.BfpRead(0x17ffc8);
   if(marker==0x4542464e)return Fail();
   if(marker==0x3342464e){
    if(!Terminal(io,baseline,i))return Fail();
    for(unsigned n=0;n<64;n++)result[i].output[n]=io.BfpRead(PioneerNativeBfpBandLayout::Output+n*4);
    if(!Terminal(io,baseline,i))return Fail();
    done[i]=true;result[i].completed=1;
   }else io.Pause();
  }
  if(!done[0] || !done[1])return Fail();
  for(auto& r:result)r.state=PIONEER_NATIVE_BFP_RESULT;
  return true;
 }
 template<class IO> bool End(IO& io,tt_dram_samples_result& baseline){
  if(!owned || unknown || closed || !result[0].completed || !result[1].completed)return false;
  // Prove both terminals before modifying either. Once any proof fails no
  // subsequent reset, restore, or TLB operation is attempted.
  for(unsigned i=0;i<2;i++)if(!Select(io,i) || !Terminal(io,baseline,i))return Fail();
  for(unsigned i=0;i<2;i++){
   if(!Select(io,i) || !Terminal(io,baseline,i))return Fail();
   io.BfpResetWrite(0x87ffff);io.Pause();if(!Scope(io,baseline,i,0x87ffff))return Fail();
   io.BfpResetWrite(0x47800);io.Pause();if(!Scope(io,baseline,i,0x47800))return Fail();result[i].held=1;
  }
  if(!BlackholeDramAllCoresHeld(io,baseline))return Fail();
  const unsigned words=Band::SavedWords(5120);
  for(unsigned i=0;i<2;i++){
   if(!Select(io,i) || !Scope(io,baseline,i,0x47800))return Fail();
   for(unsigned n=1;n<words;n++)io.BfpWrite(Band::Address(5120,n),saved[i][n]);
   if(!Scope(io,baseline,i,0x47800))return Fail();
   io.BfpWrite(0x6000,saved[i][0]);
   for(unsigned n=0;n<words;n++)if(io.BfpRead(Band::Address(5120,n))!=saved[i][n])return Fail();
   if(!Scope(io,baseline,i,0x47800))return Fail();
   result[i].l1_restored=1;
  }
  for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)io.DramTlbWrite(s,w,tlb[s][w]);
  for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)if(io.DramTlbRead(s,w)!=tlb[s][w])return Fail();
  if(!BlackholeDramSourceStable(io,baseline))return Fail();
  for(auto& r:result){r.tlb_restored=1;r.state=PIONEER_NATIVE_BFP_CLOSED;}closed=true;owned=false;return true;
 }
};
#endif
