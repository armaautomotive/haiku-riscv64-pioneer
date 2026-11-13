/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION) || PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Small recurrent upload staging is isolated offline data only
#endif
#ifndef PIONEER_Q38_SMALL_RECURRENT_UPLOAD_PLAN_H
#define PIONEER_Q38_SMALL_RECURRENT_UPLOAD_PLAN_H
#include "PioneerQ38HybridSmallRecurrentBfp8Plan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include <algorithm>
#include <cstring>
#include <vector>

// Builds the exact same physical-order spans as the separate receipt. The
// caller must separately own/authenticate the actual transfer and completion.
template<class PreparedPages>class PioneerQ38SmallRecurrentUploadPlan {
 using C=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridSmallRecurrentBfp8Plan;using Part=PioneerQ38HybridModelPlan::Part;
 const C* source=nullptr;PreparedPages* pages=nullptr;Plan::Proposal proposal;std::vector<unsigned> order;
 unsigned position=0,page=0,pendingPages=0;bool attempted=false,ready=false,pending=false,failed=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 bool Lose(std::string& error,const char* why){failed=true;return Fail(error,why);}
 static bool Same(const Part& a,const Part& b){return a.original==b.original && a.bank==b.bank && a.offset==b.offset
  && a.bytes==b.bytes && a.allocated==b.allocated && a.encoding==b.encoding;}
 static bool Overlap(const void* a,size_t n,const void* b,size_t m){
  const uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
  return (n && x>UINTPTR_MAX-n) || (m && y>UINTPTR_MAX-m) || (n && m && x<y+m && y<x+n);
 }
 bool Current()const{return ready && !failed && source && pages && source->OriginalFormatLayout() && pages->Ready()
  && proposal.parts.size()==source->Count() && order.size()==3551;}
 bool MetadataSafe(unsigned& fragment,unsigned& bank,uint64_t& offset,unsigned& length)const{
  const void* dest[]={&fragment,&bank,&offset,&length};const size_t sizes[]={sizeof(fragment),sizeof(bank),sizeof(offset),sizeof(length)};
  for(unsigned i=0;i<4;i++){
   if(!pages->OutputSafe(dest[i],sizes[i]) || Overlap(dest[i],sizes[i],this,sizeof(*this))
    || Overlap(dest[i],sizes[i],proposal.parts.data(),proposal.parts.size()*sizeof(Part))
    || Overlap(dest[i],sizes[i],order.data(),order.size()*sizeof(unsigned)))return false;
   for(unsigned j=0;j<i;j++)if(Overlap(dest[i],sizes[i],dest[j],sizes[j]))return false;
  }
  return true;
 }
public:
 PioneerQ38SmallRecurrentUploadPlan()=default;
 PioneerQ38SmallRecurrentUploadPlan(const PioneerQ38SmallRecurrentUploadPlan&)=delete;
 PioneerQ38SmallRecurrentUploadPlan& operator=(const PioneerQ38SmallRecurrentUploadPlan&)=delete;
 bool Bind(const C& c,PreparedPages& prepared,std::string& error){
  if(attempted)return Fail(error,"Small recurrent upload plan binding is one-shot.");attempted=true;
  if(!c.OriginalFormatLayout() || !prepared.AllVerified())return Fail(error,"Small recurrent upload requires verified prepared pages.");
  Plan::Proposal staged;if(!Plan::Build(c,staged) || staged.parts.size()!=c.Count())return false;
  std::vector<unsigned> indices;indices.reserve(3551);
  for(unsigned i=0;i<c.Count();i++){
   const auto* original=c.Part(i);const auto* p=prepared.Part(i);
   if(!original || !p || !Same(*p,staged.parts[i]))return Fail(error,"Prepared page layout differs from the independently derived layout.");
   if(original->kind==C::WeightKind)indices.push_back(i);
  }
  if(indices.size()!=3551)return Fail(error,"Unexpected weight count.");
  std::sort(indices.begin(),indices.end(),[&](unsigned a,unsigned b){const auto& x=staged.parts[a];const auto& y=staged.parts[b];
   return x.bank<y.bank || (x.bank==y.bank && x.offset<y.offset);});
  for(unsigned i=1;i<indices.size();i++){
   const auto& a=staged.parts[indices[i-1]];const auto& b=staged.parts[indices[i]];
   if(a.bank>b.bank || (a.bank==b.bank && uint64_t(a.offset)+a.allocated>b.offset))return Fail(error,"Overlapping upload allocations.");
  }
  source=&c;pages=&prepared;proposal=std::move(staged);order=std::move(indices);ready=true;
  if(!Current() || !pages->AllVerified())return Lose(error,"Prepared source changed during binding.");error.clear();return true;
 }
 bool Ready()const{return Current();}
 bool Next(unsigned& fragment,unsigned& bank,uint64_t& offset,unsigned& length)const{
  if(!Current() || pending || position>=order.size())return false;
  if(!MetadataSafe(fragment,bank,offset,length))return false;
  const unsigned i=order[position];const auto& p=proposal.parts[i];const auto* live=pages->Part(i);
  if(!live || !Same(*live,p) || page>=p.allocated/4096)return false;
  const unsigned remain=p.allocated/4096-page,n=remain<256?remain:256;
  fragment=i;bank=p.bank;offset=uint64_t(p.offset)+uint64_t(page)*4096;length=n*4096;return true;
 }
 bool Stage(uint8_t* output,unsigned capacity,unsigned& fragment,unsigned& bank,uint64_t& offset,unsigned& length,std::string& error){
  unsigned f=0,b=0,bytes=0;uint64_t off=0;
  if(!Next(f,b,off,bytes) || !MetadataSafe(fragment,bank,offset,length) || !output || capacity<bytes || !pages->OutputSafe(output,bytes)
   || Overlap(output,bytes,this,sizeof(*this)) || Overlap(output,bytes,proposal.parts.data(),proposal.parts.size()*sizeof(Part))
   || Overlap(output,bytes,order.data(),order.size()*sizeof(unsigned))
   || Overlap(output,bytes,&fragment,sizeof(fragment)) || Overlap(output,bytes,&bank,sizeof(bank))
   || Overlap(output,bytes,&offset,sizeof(offset)) || Overlap(output,bytes,&length,sizeof(length))
   || Overlap(&fragment,sizeof(fragment),&bank,sizeof(bank)) || Overlap(&fragment,sizeof(fragment),&offset,sizeof(offset))
   || Overlap(&fragment,sizeof(fragment),&length,sizeof(length)) || Overlap(&bank,sizeof(bank),&offset,sizeof(offset))
   || Overlap(&bank,sizeof(bank),&length,sizeof(length)) || Overlap(&offset,sizeof(offset),&length,sizeof(length)))
   return Fail(error,"Invalid or overlapping small recurrent upload staging request.");
  std::vector<uint8_t> stage(bytes);const unsigned count=bytes/4096;
  for(unsigned at=0;at<count;){
   const unsigned n=std::min(16U,count-at);
   if(!pages->ReadPages(f,page+at,n,stage.data()+at*4096,n*4096,error))return Lose(error,"Prepared weight page failed; no upload staged.");
   at+=n;
  }
  const auto* live=pages->Part(f);
  if(!Current() || !live || !Same(*live,proposal.parts[f]))return Lose(error,"Prepared source changed during staging.");
  std::memcpy(output,stage.data(),bytes);fragment=f;bank=b;offset=off;length=bytes;pendingPages=count;pending=true;error.clear();return true;
 }
 // Acknowledgement means ONLY that the caller decided this staged span is
 // complete. Any false/uncertain result permanently stops this plan.
 bool Acknowledge(bool completed){
  if(!Current() || !pending || !completed){failed=true;return false;}
  const auto& p=proposal.parts[order[position]];page+=pendingPages;pendingPages=0;pending=false;
  if(page==p.allocated/4096){page=0;position++;}return true;
 }
 bool Finished()const{return Current() && !pending && position==order.size() && pages->AllVerified();}
};
#endif
