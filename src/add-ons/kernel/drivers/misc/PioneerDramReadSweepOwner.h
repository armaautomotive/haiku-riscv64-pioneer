/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_DRAM_READ_SWEEP_OWNER_H
#define PIONEER_DRAM_READ_SWEEP_OWNER_H
#include "BlackholeComputeNocProbe.h"
#include "BlackholeDramReadSweepImage.h"
#include "BlackholeDramQueuedReadSweepImage.h"
#include "../../../../../tools/pioneer/PioneerDramReadSweep.h"
#include "../../../../../tools/pioneer/PioneerDramQueuedReadSweep.h"
// Separate single-attempt owner component, only exposed by diagnostic76.
// IO must bind this EXACT object/proofs/private backing under the native global
// lock and cold/active-owner gates. Scope/clock/test callbacks are NOT a public
// authority or substitutes for implementing that native adapter.
struct PioneerDramReadSweepResult {
 uint32_t version=1,started=0,phase=0,unsafe=0,launched=0,observed=0,held=0,closed=0;
 uint32_t savedWords=0,restoredWords=0,comparedWords=0,bank=0,report[16]={};
};
static_assert(sizeof(PioneerDramReadSweepResult)==112,"private fixed-word DATA,not a public completion lease");
template<class Image,class Program,unsigned FrameVersion>
class PioneerDramReadSweepOwnerType {
 const tt_dram_samples_result* proof=nullptr;
 const tt_compute_noc_result* noc=nullptr;
 uint32_t* saved=nullptr;uint32_t* scratch=nullptr;
 uint32_t tlb[2][3]={},command[10]={},frame[16]={};
 uint32_t before[4]={},config=0,bank=0;
 bool attempted=false,unsafe=false;
 template<class IO> bool Fail(IO& io,PioneerDramReadSweepResult& r){
  unsafe=true;r.unsafe=1;io.SweepQuarantineMemoryOnly();return false;
 } // no hardware IO/disposal/rollback/retry after first uncertainty
 template<class IO> bool Scope(IO& io){
  return attempted && !unsafe && proof && noc && saved && scratch
   && io.SweepNativeScope(this,proof,noc,saved,scratch,Image::SavedWords,Image::RawWords);
 }
 template<class IO> bool Sources(IO& io){
  if(!Scope(io))return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++){
   if(io.HostPciRead(p,4,2)!=0x402)return false;
   uint32_t cap=io.HostPciRead(p,0x34,1);uint64_t visited=0;bool found=false;
   for(unsigned hop=0;cap && hop<48;hop++){
    if(cap<0x40 || cap>0xfc || (cap&3) || (visited&(UINT64_C(1)<<(cap/4))))return false;
    visited|=UINT64_C(1)<<(cap/4);uint32_t h=io.HostPciRead(p,cap,2);
    if((h&255)==0x10){
     if(found || cap>0xf4)return false;
     unsigned v=io.HostPciRead(p,cap+2,2)&15;if(v!=1 && v!=2)return false;
     uint32_t s=io.HostPciRead(p,cap+0xa,2);if(s>0xffff || (s&0x2f))return false;
     found=true;
    }
    cap=(h>>8)&255;
   }
   if(cap || !found)return false;
  }
  return true;
 }
 template<class IO> bool SetTlb(IO& io,unsigned slot,uint32_t base,unsigned x,unsigned y,bool ro){
  if(!Scope(io))return false;
  for(unsigned i=0;i<3;i++){
   const uint32_t value=BlackholeDramFixedTlbWord(base,x,y,ro,i);
   io.DramTlbWrite(slot,i,value);
   if(io.DramTlbRead(slot,i)!=value)return false;
  }
  return true;
 }
 template<class IO> bool Idle(IO& io,bool fifo){
  for(unsigned i=0;i<4;i++)if(io.ResidentNocRead(0x40+i*0x800))return false;
  // Disabled FIFOs can hide stale requests. After enabling, prove ALL four
  // are empty BEFORE the first doorbell. Drain FIFO availability separately
  // before outstanding counters: a queued request is not yet counted there.
  if(io.ResidentNocRead(0x64)!=(fifo?0x10101010U:0U) || (io.ResidentNocRead(0x68)&15))return false;
  for(unsigned i=0;i<32;i++)if(io.ResidentNocRead(0x240+i*4)&255)return false;
  return true;
 }
 template<class IO> bool AllHeld(IO& io){
  const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
  for(unsigned x:columns)for(unsigned y=2;y<=11;y++){
   if(!Sources(io) || !SetTlb(io,1,0xffa00000,x,y,true)
    || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800)return false;
  }
  return SetTlb(io,1,0xffa00000,1,2,false);
 }
 template<class IO> bool Guard(IO& io,uint32_t reset,uint32_t expectedConfig,bool idle,
  uint32_t base=0,unsigned x=1,unsigned y=2,bool ro=false){
  if(!Sources(io))return false;
  for(unsigned i=0;i<3;i++){
   if(io.DramTlbRead(0,i)!=BlackholeDramFixedTlbWord(base,x,y,ro,i)
    || io.DramTlbRead(1,i)!=BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i))return false;
  }
  if(io.ResetRead()!=reset || io.ResetRead()!=reset)return false;
  // TRISC0 must use its authenticated L1 boot jump, not a firmware-selected
  // alternate entry. Read only; never clear someone else's override. Repeat
  // at every guard, including after backend init and immediately pre-launch.
  const uint32_t entry=io.SweepEntryOverrideRead();
  if((entry&1) || io.SweepEntryOverrideRead()!=entry)return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=(i?noc->source[0][i]:expectedConfig))return false;
  return !idle || Idle(io,(expectedConfig&0x10000)!=0);
 }
 template<class IO> bool CodeAndFrame(IO& io){
  uint32_t v=0;
  for(unsigned i=0;i<Image::CodeWords;i++)if(!io.SweepReadChecked(Image::Address(i),v) || v!=Image::Code(i))return false;
  for(unsigned i=0;i<16;i++)if(!io.SweepReadChecked(Program::Job+i*4,v) || v!=frame[i])return false;
  return true;
 }
 template<class IO> bool Counters(IO& io){
  const unsigned reads=(Program::Span>>frame[3])*frame[2];
  for(unsigned i=0;i<4;i++){
   const unsigned expected=i==1?reads*((1U<<frame[3])/64):reads;
   if(uint32_t(io.ResidentNocRead(0x208+i*4)-before[i])!=expected)return false;
  }
  return true;
 }
public:
 using SnapshotImage=Image;
 using DeviceProgram=Program;
 static constexpr bool Queued=FrameVersion==2;
 template<class IO> bool Execute(IO& io,const tt_dram_samples_result& d,const tt_compute_noc_result& n,
  unsigned selectedBank,unsigned shift,unsigned sweeps,uint32_t nonce,PioneerDramReadSweepResult& r,unsigned depth=1){
  const PioneerDramReadSweepResult pristine;
  const auto* a=reinterpret_cast<const uint8_t*>(&r);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(attempted || (!Queued && depth!=1) || selectedBank>=7 || d.version!=1 || d.state!=TT_DRAM_SAMPLE_ROUTES_OK
   || d.routes_verified!=7 || !d.data_restored || !d.tlb_restored || d.recovery_required
   || n.version!=1 || n.state!=TT_COMPUTE_NOC_PASSED || !n.tlb_restored || n.recovery_required
   || !BlackholeComputeNocRouteValid(n.source[0]) || (n.source[0][0]&0x10000))return false;
  tt_dram_samples_result routes=d;
  if(!BlackholeDramBuildRoutes(routes))return false;
  for(unsigned k=0;k<7;k++)for(unsigned i=0;i<2;i++)if(routes.translated[k][i]!=d.translated[k][i])return false;
  for(unsigned sample=0;sample<3;sample++){
   if((n.node[sample][0]&0xfff)!=129 || ((n.node[sample][0]>>12)&127)!=17
    || ((n.node[sample][0]>>19)&127)!=12 || (n.node[sample][1]&0xffffff00)!=0x10000)return false;
   for(unsigned i=0;i<16;i++)if(n.source[sample][i]!=n.source[0][i] || (i && n.source[0][i]!=d.source[i]))return false;
   for(unsigned i=0;i<4;i++)if(n.commands[sample][i])return false;
   for(unsigned i=0;i<16;i++)if(n.outstanding[sample][i]&255)return false;
  }
  frame[0]=Program::Magic;frame[1]=FrameVersion;frame[2]=sweeps;frame[3]=shift;
  frame[4]=routes.translated[selectedBank][0]|(routes.translated[selectedBank][1]<<6);frame[5]=nonce;
  if(Queued)frame[6]=depth;
  if(!Program::Shape(frame))return false;
  if(io.SweepAllocationWords()!=Image::SavedWords+Image::RawWords)return false;
  saved=io.SweepSavedWords();scratch=io.SweepScratchWords();
  if(!saved || !scratch || scratch!=saved+Image::SavedWords)return false;
  proof=&d;noc=&n;bank=selectedBank;config=n.source[0][0];attempted=true;
  if(!Scope(io))return false; // no physical mutation or foreign-owner cleanup
  r.started=1;r.phase=1;r.bank=bank;
  if(!Sources(io) || !io.SweepClockBegin())return Fail(io,r);
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){
   tlb[s][i]=io.DramTlbRead(s,i);if(i==2 && (tlb[s][i]&0xffffc000))return Fail(io,r);
  }
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)if(io.DramTlbRead(s,i)!=tlb[s][i])return Fail(io,r);
  // Recheck EVERY physical worker under the same lifetime; no synthetic
  // earlier all-held flag is accepted as evidence that peers remain held.
  if(!AllHeld(io) || !SetTlb(io,0,0,1,2,false)
   || !Guard(io,0x47800,config,true) || (io.ResidentNocRead(0x178)&0xffff))return Fail(io,r);
  r.phase=2;
  for(unsigned i=0;i<10;i++){
   command[i]=io.ResidentNocRead(i*4);
   if(command[i])return Fail(io,r); // pristine initiator only, no unknown flags
  }
  for(unsigned i=0;i<Image::SavedWords;i++){
   if(!(i&255) && !Guard(io,0x47800,config,true))return Fail(io,r);
   if(!io.SweepReadChecked(Image::Address(i),saved[i]))return Fail(io,r);
   r.savedWords++;
  }
  for(unsigned i=0;i<Image::SavedWords;i++){
   if(!(i&255) && !Guard(io,0x47800,config,true))return Fail(io,r);
   uint32_t actual=0;
   if(!io.SweepReadChecked(Image::Address(i),actual) || actual!=saved[i])return Fail(io,r);
  }
  if(!Guard(io,0x47800,config,true))return Fail(io,r);
  r.phase=3;
  io.ResidentNocWrite(0x100,config|0x10000);
  if(!Guard(io,0x47800,config|0x10000,true))return Fail(io,r);
  for(unsigned i=1;i<Image::CodeWords;i++)if(!io.SweepWrite(Image::Address(i),Image::Code(i)))return Fail(io,r);
  for(unsigned i=0;i<16;i++)if(!io.SweepWrite(Program::Job+i*4,frame[i])
   || !io.SweepWrite(Program::Report+i*4,0))return Fail(io,r);
  for(unsigned i=0;i<16;i++){
   uint32_t cleared=0;
   if(!io.SweepReadChecked(Program::Report+i*4,cleared) || cleared)return Fail(io,r);
  }
  if(!io.SweepWrite(0x6000,Image::Code(0)) || !CodeAndFrame(io)
   || !Guard(io,0x47800,config|0x10000,true))return Fail(io,r);
  r.phase=4;
  io.SweepResetWrite(0x87ffff);
  if(!Guard(io,0x87ffff,config|0x10000,true))return Fail(io,r);
  for(unsigned i=0;i<4;i++)before[i]=io.ResidentNocRead(0x208+i*4);
  r.phase=5;r.launched=1;io.SweepResetWrite(0x46800);
  bool stopped=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!Guard(io,0x46800,config|0x10000,false))return Fail(io,r);
   uint32_t marker=0;if(!io.SweepReadChecked(Program::Report,marker))return Fail(io,r);
   if(marker && marker!=Program::ReportMagic)return Fail(io,r);
   if(marker){
    // PC is fetch-ahead DATA, not a receipt. ONLY the two authenticated
    // self-jumps are allowed, combined with current code/frame/report/NoC.
    const uint32_t p=io.SweepPcRead(),q=io.SweepPcRead();
    if((p==Image::Park || p==Image::Park+4) && (q==Image::Park || q==Image::Park+4)){
     stopped=true;break;
    }
   }
   io.Pause();
  }
  if(!stopped || !Guard(io,0x46800,config|0x10000,true) || !CodeAndFrame(io) || !Counters(io))return Fail(io,r);
  for(unsigned i=0;i<16;i++)if(!io.SweepReadChecked(Program::Report+i*4,r.report[i]))return Fail(io,r);
  const auto* v=r.report;const unsigned bytes=1U<<shift,reads=(Program::Span>>shift)*sweeps;
  const unsigned batchBytes=bytes*depth,lastOffset=(depth-1)*bytes;
  const uint64_t start=(uint64_t(v[9])<<32)|v[8],end=(uint64_t(v[11])<<32)|v[10];
  if(v[0]!=Program::ReportMagic || v[1]!=FrameVersion || v[2]!=nonce || v[3]!=frame[4]
   || v[4]!=bytes || v[5]!=sweeps || v[6]!=Program::Span*sweeps || v[7]!=reads
   || end<=start || ((uint64_t(v[13])<<32)|v[12])!=end-start)return Fail(io,r);
  r.observed=1;r.phase=6;
  io.SweepResetWrite(0x47800);
  if(!Guard(io,0x47800,config|0x10000,true) || !AllHeld(io)
   || !Guard(io,0x47800,config|0x10000,true))return Fail(io,r);
  r.held=1;
  for(unsigned i=0;i<batchBytes/4;i++)if(!io.SweepReadChecked(Program::Raw+i*4,scratch[i]))return Fail(io,r);
  if(scratch[lastOffset/4]!=v[14] || scratch[(lastOffset+bytes)/4-1]!=v[15])return Fail(io,r);
  r.phase=7;
  // Independent PIO check of the ENTIRE last batch, after actual HOLD.
  // All live return slots, not just the final one; not full8MiB integrity.
  const unsigned x=proof->translated[bank][0],y=proof->translated[bank][1];
  const uint32_t last=Program::Base+Program::Span-batchBytes,page=last&~0x1fffffU;
  if(!SetTlb(io,0,page,x,y,true) || !Guard(io,0x47800,config|0x10000,true,page,x,y,true))return Fail(io,r);
  for(unsigned i=0;i<batchBytes/4;i++){
   uint32_t actual=0;
   if(!io.SweepWindowReadChecked(last-page+i*4,actual) || actual!=scratch[i])return Fail(io,r);
   r.comparedWords++;
  }
  if(!Guard(io,0x47800,config|0x10000,true,page,x,y,true) || !SetTlb(io,0,0,1,2,false)
   || !Guard(io,0x47800,config|0x10000,true) || !CodeAndFrame(io) || !Counters(io))return Fail(io,r);
  r.phase=8;
  for(unsigned i=1;i<Image::SavedWords;i++){
   if(!(i&255) && !Guard(io,0x47800,config|0x10000,true))return Fail(io,r);
   uint32_t actual=0;
   if(!io.SweepWrite(Image::Address(i),saved[i]) || !io.SweepReadChecked(Image::Address(i),actual)
    || actual!=saved[i])return Fail(io,r);
   r.restoredWords++;
  }
  uint32_t actual=0;
  if(!io.SweepWrite(0x6000,saved[0]) || !io.SweepReadChecked(0x6000,actual) || actual!=saved[0]
   || !Guard(io,0x47800,config|0x10000,true))return Fail(io,r);
  r.restoredWords++;
  for(unsigned i=0;i<Image::SavedWords;i++){
   if(!(i&255) && !Guard(io,0x47800,config|0x10000,true))return Fail(io,r);
   if(!io.SweepReadChecked(Image::Address(i),actual) || actual!=saved[i])return Fail(io,r);
  }
  for(unsigned i=0;i<10;i++){
   io.ResidentNocWrite(i*4,command[i]);if(io.ResidentNocRead(i*4)!=command[i])return Fail(io,r);
  }
  io.ResidentNocWrite(0x100,config);
  if(!Guard(io,0x47800,config,true))return Fail(io,r);
  r.phase=9;
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){
   io.DramTlbWrite(s,i,tlb[s][i]);if(io.DramTlbRead(s,i)!=tlb[s][i])return Fail(io,r);
  }
  if(!Sources(io) || !io.SweepClockEnd() || !Sources(io))return Fail(io,r);
  for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)if(io.DramTlbRead(s,i)!=tlb[s][i])return Fail(io,r);
  r.phase=10;r.closed=1;return true;
 }
};
using PioneerDramReadSweepControlOwner=PioneerDramReadSweepOwnerType<BlackholeDramReadSweepImage,PioneerDramReadSweep,1>;
using PioneerDramQueuedReadSweepOwner=PioneerDramReadSweepOwnerType<BlackholeDramQueuedReadSweepImage,PioneerDramQueuedReadSweep,2>;
#ifdef PIONEER_DRAM_SWEEP_QUEUED
using PioneerDramReadSweepOwner=PioneerDramQueuedReadSweepOwner;
#else
using PioneerDramReadSweepOwner=PioneerDramReadSweepControlOwner;
#endif
#endif
