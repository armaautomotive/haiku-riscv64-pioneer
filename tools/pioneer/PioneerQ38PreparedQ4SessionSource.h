/* SPDX-License-Identifier: MIT */
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadPreparedQ4Role.h"
#ifndef PIONEER_Q38_PREPARED_Q4_SESSION_SOURCE_H
#define PIONEER_Q38_PREPARED_Q4_SESSION_SOURCE_H
#include "PioneerQ38PreparedQ4RootTables.h"
#include "PioneerQ38PreparedQ4PlanningPages.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38PreparedSourceProtocol.h"

// Own the pre-device preparation/readers for ONE public full-card session.
// The cache is derived weights only;never a CPU inference/reference path.
// Verification or cold conversion completes BEFORE the device is opened.
// Later page reads only open verified existing entries;no runtime conversion.
template<class Source>
class PioneerQ38PreparedQ4SessionSource {
 PioneerQ38NativeQ4WeightCache weights;
 PioneerQ38NativeQ4CachedPages payload;
 PioneerQ38PreparedQ4PlanningPages<> pages{payload};
 using Upload=PioneerQ38PreparedQ4UploadPlan<PioneerQ38NativeQ4WeightCache,PioneerQ38PreparedQ4PlanningPages<>>;
 Upload upload;
 PioneerQ38PreparedQ4RootTables<Upload> tables;
 Source* source=nullptr;
 const PioneerQwen38PackedPlan* original=nullptr;
 PioneerQ38NativeQ4WeightCache::Statistics statistics;
 bool attempted=false,ready=false;
public:
 using Span=typename Upload::Span;
 std::string error;
 bool Ready()const{return ready && source && original && source->Ready() && original->MatchesSource(source->Model())
  && upload.Ready() && tables.Ready();}
 bool Build(Source& owner,const PioneerQwen38PackedPlan& layout,
  const char* directory="/boot/home/config/cache/pioneer-q38-native-q4-v1"){
  if(attempted)return false;
  attempted=true;
  if(!owner.Ready() || !owner.BindPreparedCache(weights,layout,directory,error))return false;
  while(!weights.AllPrepared()){
   const unsigned fragment=weights.NextFragment();
   if(!weights.PrepareNext(statistics,error) || !pages.Capture(weights,fragment,error))return false;
  }
  if(statistics.fragments!=1351 || !pages.Bind(directory,weights,[&owner]{return owner.Ready();},error)
   || !upload.Build(weights,pages,error) || !pages.FinishPlanning() || !tables.Build(upload,owner.Model(),error)
   || upload.Spans().size()!=4895 || upload.Primaries()!=1344 || upload.Bytes()!=UINT64_C(20470759424))return false;
  source=&owner;original=&layout;ready=true;return Ready();
 }
 const std::vector<Span>& Spans()const{return upload.Spans();}
 uint64_t Bytes()const{return Ready()?upload.Bytes():0;}
 const auto& PreparationStatistics()const{return statistics;}
 uint64_t CapturedIndexPages()const{return pages.CapturedPages();}
 uint64_t PlanningIndexReads()const{return pages.PlanningReads();}
 uint64_t PayloadWindowFills()const{return payload.WindowFills();}
 bool ReadPage(unsigned span,unsigned page,uint8_t* output,unsigned capacity){
  if(!Ready())return false;
  auto raw=[this](unsigned f,unsigned p,uint8_t* out,unsigned bytes){return source->ReadWeightPage(*original,f,p,out,bytes);};
  return upload.ReadPage(span,page,output,capacity,raw,error) && Ready();
 }
 bool Publication(unsigned operation,unsigned index,PioneerQ38PreparedSource::Packet& output)const{
  namespace P=PioneerQ38PreparedSource;
  if(!Ready())return false;
  P::Packet staged;auto& q=staged.request;q.operation=operation;q.index=index;
  if(operation==P::Begin){
   if(index)return false;
   q.words=P::BeginWords;for(unsigned i=0;i<16;i++)q.data[i]=P::Identity[i];
   unsigned n=0;
   for(const auto& span:upload.Spans())if(span.kind==upload.Correction){
    if(n>=P::Parts)return false;
    q.data[16+n++]=span.bytes;
   }
   if(n!=P::Parts)return false;
  }else if(operation==P::Phase){
   if(index>=P::Phases)return false;
   q.words=P::Words;
   if(!tables.Read(index/3,index%3+2,0,reinterpret_cast<uint8_t*>(q.data),sizeof(q.data)))return false;
  }else if(operation!=P::Seal || index!=P::Phases)return false;
  if(!P::RequestShape(staged) || !Ready())return false;
  output=staged;return true;
 }
};
#endif
