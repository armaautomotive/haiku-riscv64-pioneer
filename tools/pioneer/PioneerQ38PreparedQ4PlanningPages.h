/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_Q4_PLANNING_PAGES_H
#define PIONEER_Q38_PREPARED_Q4_PLANNING_PAGES_H
#include "PioneerQ38NativeQ4CachedPages.h"

// Capture ONLY correction-index metadata while PrepareNext still retains
// its just-verified bundle reader. This avoids reopening/rehashing every
// entire bundle merely to construct the pre-device upload/job plan.
// After FinishPlanning,all payload reads use the original read-only adapter
// with its full file/chunk verification. Never serve these metadata-only
// pages to a device. The original writer/encoding/recipe is unchanged.
template<class Weights=PioneerQ38NativeQ4WeightCache,class Payload=PioneerQ38NativeQ4CachedPages>
class PioneerQ38PreparedQ4PlanningPages {
 Weights* weights=nullptr;Payload& payload;
 std::vector<std::vector<uint8_t>> indices;
 unsigned captured=0;uint64_t capturePages=0,planningReads=0;
 bool attempted=false,bound=false,planning=true,failed=false;
 bool Fail(std::string& error,const char* why){failed=true;error=why;return false;}
 static bool Separate(const void* output,unsigned bytes,const void* data,size_t size){
  const uintptr_t o=reinterpret_cast<uintptr_t>(output),p=reinterpret_cast<uintptr_t>(data);
  return output && o<=UINTPTR_MAX-bytes && p<=UINTPTR_MAX-size && (o+bytes<=p || p+size<=o);
 }
 bool Output(const void* output,unsigned bytes)const{
  if(!Separate(output,bytes,this,sizeof(*this)) || !Separate(output,bytes,&payload,sizeof(payload))
   || (weights && (!Separate(output,bytes,weights,sizeof(*weights))
    || !Separate(output,bytes,weights->OriginalPlan().Parts().data(),weights->OriginalPlan().Parts().size()*sizeof(PioneerQ38PackedPart))))
   || !Separate(output,bytes,indices.data(),indices.size()*sizeof(indices[0])))return false;
  for(const auto& part:indices)if(!Separate(output,bytes,part.data(),part.size()))return false;
  return true;
 }
 static bool Selected(const PioneerQ38PackedPart& p){return p.kind==PIONEER_Q38_WEIGHT && p.layer<64 && p.role>=2 && p.role<=4;}
public:
 explicit PioneerQ38PreparedQ4PlanningPages(Payload& actual):payload(actual){}
 PioneerQ38PreparedQ4PlanningPages(const PioneerQ38PreparedQ4PlanningPages&)=delete;
 PioneerQ38PreparedQ4PlanningPages& operator=(const PioneerQ38PreparedQ4PlanningPages&)=delete;
 bool Capture(Weights& source,unsigned fragment,std::string& error){
  if(attempted || failed || (weights && weights!=&source) || !source.OriginalPlan().Ready())return false;
  const auto& parts=source.OriginalPlan().Parts();
  if(fragment>=parts.size())return Fail(error,"Invalid prepared index capture fragment.");
  if(!weights){weights=&source;indices.resize(parts.size());}
  const auto p=parts[fragment];if(!Selected(p))return true;
  if(!indices[fragment].empty() || !p.bytes || p.bytes%2304)return Fail(error,"Duplicate/invalid prepared index capture.");
  PioneerQ38NativeQ4WeightCache::CorrectionProposal side{};
  const unsigned bytes=(p.bytes/2304*4+15)&~15U;
  if(!source.Correction(fragment,side) || side.bytes<bytes)return Fail(error,"Unprepared correction index capture.");
  std::vector<uint8_t> staged(bytes);std::array<uint8_t,4096> page{};
  for(unsigned at=0;at<bytes;at+=4096){
   if(!source.ReadPage(fragment,at/4096,true,page.data(),page.size(),error))return Fail(error,"Cannot capture verified correction index.");
   memcpy(staged.data()+at,page.data(),std::min(4096U,bytes-at));capturePages++;
  }
  indices[fragment].swap(staged);captured++;return true;
 }
 bool Bind(const char* directory,Weights& source,const PioneerModelWeightCache::Ready& ready,std::string& error){
  if(attempted || failed)return false;
  attempted=true;
  if(weights!=&source || !source.AllPrepared() || captured!=1344 || !ready || !ready())return Fail(error,"Incomplete prepared index capture.");
  const auto& parts=source.OriginalPlan().Parts();
  if(indices.size()!=parts.size())return Fail(error,"Prepared capture geometry changed.");
  for(unsigned f=0;f<parts.size();f++){
   const unsigned expected=Selected(parts[f])?((parts[f].bytes/2304*4+15)&~15U):0;
   if(indices[f].size()!=expected)return Fail(error,"Prepared capture coverage changed.");
  }
  if(!payload.Bind(directory,source,ready,error))return Fail(error,"Cannot bind prepared payload reader.");
  bound=true;return ReadyFor(source);
 }
 bool ReadyFor(const Weights& source)const{return bound && !failed && weights==&source && source.AllPrepared() && payload.ReadyFor(source);}
 bool FinishPlanning(){
  if(!weights || !ReadyFor(*weights) || !planning)return false;
  planning=false;std::vector<std::vector<uint8_t>>().swap(indices);return true;
 }
 bool ReadPage(unsigned fragment,unsigned page,bool correction,uint8_t* output,unsigned capacity,std::string& error){
  if(!weights || !ReadyFor(*weights) || !output || capacity<4096 || !Output(output,4096))return false;
  if(!planning)return payload.ReadPage(fragment,page,correction,output,capacity,error);
  if(!correction || fragment>=indices.size() || uint64_t(page)*4096>=indices[fragment].size())return false;
  std::array<uint8_t,4096> staged{};const auto& data=indices[fragment];const size_t at=size_t(page)*4096;
  memcpy(staged.data(),data.data()+at,std::min(size_t(4096),data.size()-at));
  if(!ReadyFor(*weights))return false;
  memcpy(output,staged.data(),staged.size());planningReads++;return true;
 }
 uint64_t CapturedPages()const{return capturePages;}
 uint64_t PlanningReads()const{return planningReads;}
 unsigned CapturedFragments()const{return captured;}
};
#endif
