/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_Q4_UPLOAD_PLAN_H
#define PIONEER_Q38_PREPARED_Q4_UPLOAD_PLAN_H
#include "PioneerQ38NativeQ4CachedPages.h"
#include "PioneerQ38PreparedQ4Layout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
// Pre-device upload/expected-read DATA. No transport,source allocation,
// initialization,ROOT/native ownership or CPU inference is conferred here.
// Embedding Q4 MUST remain original:its scalar lookup decoder is unchanged.
// Only the 192 gate/up/down tensors use the distinct matrix-worker format.
template<class Weights=PioneerQ38NativeQ4WeightCache,class Pages=PioneerQ38NativeQ4CachedPages>
class PioneerQ38PreparedQ4UploadPlan {
public:
 enum Kind:unsigned {Original=0,PreparedPrimary=1,Correction=2};
 struct Span {unsigned fragment,bank,kind,address,bytes,allocated;};
 struct Job {unsigned descriptor[16]={},extraReads=0,extraFlits=0;};
private:
 using L=PioneerQ38PreparedQ4Layout;
 Weights* weights=nullptr;Pages* pages=nullptr;
 std::vector<Span> spans;
 std::vector<PioneerQ38PackedPart> originalParts;
 std::vector<unsigned> corrections;
 std::vector<std::vector<unsigned>> indices;
 uint64_t used[7]={},totalBytes=0;unsigned primaryCount=0;
 bool attempted=false,ready=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 template<class T> static bool Separate(const void* output,unsigned bytes,const T* object){
  const auto o=reinterpret_cast<uintptr_t>(output),p=reinterpret_cast<uintptr_t>(object);
  return output && o<=UINTPTR_MAX-bytes && (!object || o+bytes<=p || p+sizeof(*object)<=o);
 }
 bool Output(const void* output,unsigned bytes)const{
  if(!Separate(output,bytes,this) || !Separate(output,bytes,weights) || !Separate(output,bytes,pages))return false;
  const auto o=reinterpret_cast<uintptr_t>(output);
  for(const auto& range:{std::make_pair(reinterpret_cast<uintptr_t>(spans.data()),spans.size()*sizeof(Span)),
    std::make_pair(reinterpret_cast<uintptr_t>(corrections.data()),corrections.size()*sizeof(unsigned)),
    std::make_pair(reinterpret_cast<uintptr_t>(originalParts.data()),originalParts.size()*sizeof(PioneerQ38PackedPart)),
    std::make_pair(reinterpret_cast<uintptr_t>(weights->OriginalPlan().Parts().data()),weights->OriginalPlan().Parts().size()*sizeof(PioneerQ38PackedPart))})
   if(o<range.first+range.second && range.first<o+bytes)return false;
  return true;
 }
public:
 PioneerQ38PreparedQ4UploadPlan()=default;
 PioneerQ38PreparedQ4UploadPlan(const PioneerQ38PreparedQ4UploadPlan&)=delete;
 PioneerQ38PreparedQ4UploadPlan& operator=(const PioneerQ38PreparedQ4UploadPlan&)=delete;
 bool Ready()const{return ready && weights && pages && weights->AllPrepared() && pages->ReadyFor(*weights);}
 const PioneerQwen38PackedPlan* OriginalPlan()const{return Ready()?&weights->OriginalPlan():nullptr;}
 uint64_t Bytes()const{return Ready()?totalBytes:0;}
 uint64_t Used(unsigned bank)const{return Ready() && bank<7?used[bank]:0;}
 unsigned Primaries()const{return Ready()?primaryCount:0;}
 const std::vector<Span>& Spans()const{return spans;}
 bool Build(Weights& source,Pages& reader,std::string& error){
  if(attempted)return Fail(error,"Prepared Q4 upload planning is one-shot.");attempted=true;
  if(!source.AllPrepared() || !reader.ReadyFor(source))return Fail(error,"Prepare and authenticate ALL Q4 cache fragments before upload planning.");
  const auto& original=source.OriginalPlan();const auto& parts=original.Parts();
  if(!original.Tiled() || original.DeltaSlots()!=2 || parts.size()!=4678)return Fail(error,"Prepared Q4 requires the original full8192/two-state plan.");
  std::vector<Span> staged;std::vector<unsigned> mapped(parts.size(),UINT32_MAX);uint64_t next[7]={},bytes=0;unsigned selected=0,weightsCount=0;
  std::vector<std::vector<unsigned>> captured(parts.size());std::array<uint8_t,4096> page{};
  for(unsigned bank=0;bank<7;bank++)next[bank]=original.Used(bank);
  for(unsigned f=0;f<parts.size();f++){
   const auto& p=parts[f];if(p.kind!=PIONEER_Q38_WEIGHT)continue;
   if(p.bank>=7 || !p.bytes || !p.allocated || ((p.proposedAddress|p.allocated)&4095)
    || p.proposedAddress<original.Base || p.bytes>p.allocated || p.allocated-p.bytes>=4096
    || uint64_t(p.proposedAddress)-original.Base+p.allocated>original.Used(p.bank))return Fail(error,"Invalid original upload fragment.");
   const bool native=p.layer<64 && p.role>=2 && p.role<=4;
   staged.push_back({f,p.bank,native?PreparedPrimary:Original,p.proposedAddress,p.bytes,p.allocated});bytes+=p.allocated;weightsCount++;
   if(!native)continue;
   PioneerQ38NativeQ4WeightCache::CorrectionProposal side{};PioneerModelWeightCache::Key key;
   if(!source.Correction(f,side) || !source.KeyFor(f,false,key) || side.fragment!=f || side.bank!=p.bank
    || !side.bytes || !side.allocated || side.allocated!=L::Allocated(side.bytes) || key.bytes!=uint64_t(p.bytes)+side.bytes
    || !p.rowBytes || p.rowBytes%144 || p.bytes%2304 || (p.firstRow&15) || (p.rows&15)
    || (p.rowBytes!=2880 && p.rowBytes!=9792) || next[p.bank]>original.Capacity
    || side.allocated>original.Capacity-next[p.bank])return Fail(error,"Invalid prepared FFN correction fragment.");
   mapped[f]=unsigned(staged.size());staged.push_back({f,p.bank,Correction,unsigned(original.Base+next[p.bank]),side.bytes,side.allocated});
   // Capture each verified integer index ONCE,not by repeatedly reopening
   // the entire bundle as the119-way scheduler interleaves source banks.
   auto& index=captured[f];index.resize(p.bytes/2304);
   for(unsigned at=0;at<index.size();){
    if(!reader.ReadPage(f,at/1024,true,page.data(),page.size(),error) || !source.AllPrepared() || !reader.ReadyFor(source))return false;
    const unsigned count=std::min(1024U,unsigned(index.size())-at);
    for(unsigned i=0;i<count;i++)index[at+i]=unsigned(page[i*4])|(unsigned(page[i*4+1])<<8)|(unsigned(page[i*4+2])<<16)|(unsigned(page[i*4+3])<<24);
    at+=count;
   }
   next[p.bank]+=side.allocated;bytes+=side.allocated;selected++;
  }
  if(weightsCount!=3551 || selected!=1344 || staged.size()!=4895 || !source.AllPrepared() || !reader.ReadyFor(source))
   return Fail(error,"Prepared Q4 upload coverage is incomplete.");
  originalParts=parts; // private immutable metadata,not a Ready-dependent accessor during later reads
  weights=&source;pages=&reader;spans.swap(staged);corrections.swap(mapped);indices.swap(captured);totalBytes=bytes;primaryCount=selected;
  for(unsigned bank=0;bank<7;bank++)used[bank]=next[bank];ready=true;return Ready();
 }
 template<class OriginalReader> bool ReadPage(unsigned span,unsigned page,uint8_t* output,unsigned capacity,
  OriginalReader& raw,std::string& error){
  if(!Ready() || span>=spans.size() || !output || capacity<4096 || !Output(output,4096))return Fail(error,"Invalid prepared upload read.");
  const auto p=spans[span];if(page>=p.allocated/4096)return Fail(error,"Invalid prepared upload page.");
  std::array<uint8_t,4096> stage{};
  const bool okay=p.kind==Original?raw(p.fragment,page,stage.data(),stage.size()):
   pages->ReadPage(p.fragment,page,p.kind==Correction,stage.data(),stage.size(),error);
  if(!okay || !Ready())return Fail(error,"Prepared upload source changed or failed.");
  if(p.kind==Correction){
   const auto& captured=indices[p.fragment];const uint64_t first=uint64_t(page)*1024;
   for(unsigned i=0;i<1024 && first+i<captured.size();i++){
    const unsigned actual=unsigned(stage[i*4])|(unsigned(stage[i*4+1])<<8)|(unsigned(stage[i*4+2])<<16)|(unsigned(stage[i*4+3])<<24);
    if(actual!=captured[size_t(first+i)])return Fail(error,"Q4 upload index changed after job planning.");
   }
  }
  const uint64_t at=uint64_t(page)*4096;if(at>=p.bytes)return Fail(error,"Prepared upload page lacks logical data.");
  for(unsigned i=unsigned(std::min(UINT64_C(4096),uint64_t(p.bytes)-at));i<4096;i++)if(stage[i])return Fail(error,"Nonzero prepared upload padding.");
  memcpy(output,stage.data(),stage.size());return true;
 }
 // Integer metadata planning ONLY,performed before a future device Open.
 // Expected read/flit counts come from the authenticated cached index,
 // never from worker receipts,its local summary or a caller-ready flag.
 bool Describe(unsigned fragment,unsigned firstRow,unsigned groups,unsigned worker,unsigned request,Job& output,std::string& error)const{
  if(!Ready() || !Output(&output,sizeof(output)) || fragment>=corrections.size() || corrections[fragment]==UINT32_MAX
   || !groups || groups>16 || !L::RootSlot(worker) || !request || request>=0x7fffffff)return Fail(error,"Invalid prepared Q4 job.");
  const auto& p=originalParts[fragment];const auto& side=spans[corrections[fragment]];
  if(firstRow<p.firstRow || firstRow-p.firstRow>=p.rows || ((firstRow-p.firstRow)&15) || groups*16>p.rows-(firstRow-p.firstRow))
   return Fail(error,"Prepared Q4 job leaves its original fragment.");
  const unsigned inputs=p.rowBytes/144*256,firstTile=(firstRow-p.firstRow)/16*(inputs/256);
  Job stage;const unsigned fields[]={L::Magic,worker,request,12,inputs,groups,p.proposedAddress+firstTile*2304,
   PioneerQ38ProjectionRoute(p.bank),side.address,side.bytes,firstTile,p.bytes/2304,p.proposedAddress,p.bytes,1,0};
  for(unsigned i=0;i<16;i++)stage.descriptor[i]=fields[i];
  unsigned job[16]={};job[1]=12;job[2]=2304;job[5]=inputs;job[6]=fields[6];job[7]=groups;job[8]=fields[7];job[11]=worker;job[12]=request;job[14]=1;
  if(!L::Shape(fields,job))return Fail(error,"Prepared descriptor/source geometry mismatch.");
  stage.extraReads=1;stage.extraFlits=1;
  for(unsigned group=0;group<groups;group++){
   unsigned index[128]={};const unsigned bytes=L::IndexReadBytes(fields,group),first=(firstTile+group*(inputs/256))&~15U;L::Window window;
   for(unsigned i=0;i<bytes/4 && first+i<indices[fragment].size();i++)index[i]=indices[fragment][first+i];
   if(!Ready() || !L::GroupWindow(fields,group,index,window))return Fail(error,"Cannot plan authenticated Q4 correction reads.");
   stage.extraReads+=1+(window.bytes+16383)/16384;stage.extraFlits+=(bytes+window.bytes)/64;
  }
  if(!Ready())return Fail(error,"Prepared source changed during job planning.");
  output=stage;return true;
 }
};
#endif
