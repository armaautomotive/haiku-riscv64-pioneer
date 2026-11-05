/* SPDX-License-Identifier: MIT */
#include "PioneerQ38FullOffloadPreparedQ4Role.h"
#ifndef PIONEER_Q38_PREPARED_NATIVE_SOURCE_H
#define PIONEER_Q38_PREPARED_NATIVE_SOURCE_H
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38ProjectionJobs.h"
#include "../../../../../tools/pioneer/PioneerQ38PreparedQ4RootLayout.h"
#include <new>
// Private immutable source metadata bound to the SAME retained bank/cookie/
// upload ledger. Its side pages lie after canonical Used(bank),not in the
// mutable runtime suffix. The whole21GiB bank owner retains those pages.
// No constructor/read method itself grants code installation or execution.
class PioneerQ38PreparedNativeSource {
 using L=PioneerQ38PreparedQ4RootLayout;using Q=PioneerQ38PreparedQ4Layout;
 struct Side {uint32_t canonical,bank,address,bytes,allocated;};
 PioneerQ38CanonicalModelPlan canonical;
 Side sides[1344]={};uint32_t tables[192][L::Bytes/4]={};
 const void* context=nullptr;const void* cookie=nullptr;const void* hashes=nullptr;const uint8_t* ledger=nullptr;
 int64_t team=-1;uint64_t generation=0;unsigned phases=0;
 bool attempted=false,prepared=false,sealed=false,failed=false;
 static bool Separate(const void* p,unsigned n,const void* object,unsigned bytes){
  const uintptr_t a=reinterpret_cast<uintptr_t>(p),b=reinterpret_cast<uintptr_t>(object);
  return p && a<=UINTPTR_MAX-n && (a+n<=b || b+bytes<=a);
 }
 bool Fail(){failed=true;return false;}
 const Side* Find(unsigned part)const{for(const auto& s:sides)if(s.canonical==part)return &s;return nullptr;}
public:
 PioneerQ38PreparedNativeSource()=default;
 PioneerQ38PreparedNativeSource(const PioneerQ38PreparedNativeSource&)=delete;
 PioneerQ38PreparedNativeSource& operator=(const PioneerQ38PreparedNativeSource&)=delete;
 bool Ready()const{return prepared && sealed && !failed && phases==192;}
 bool Failed()const{return failed;}
 void Abandon(){failed=true;} // CPU metadata only;never grants hardware cleanup
 unsigned AcceptedPhases()const{return phases;}
 bool Output(const void* output,unsigned bytes)const{return Separate(output,bytes,this,sizeof(*this));}
 bool Begin(const void* c,const void* fd,const void* h,const uint8_t* initialized,int64_t owner,uint64_t gen,
  const PioneerResidentBanks& banks,const tt_dram_samples_result& proof,const uint32_t* sizes,unsigned count,unsigned initializedPages){
  if(attempted || !c || !fd || !h || !initialized || owner<0 || !gen || !sizes || count!=1344
   || initializedPages!=PioneerResidentBanks::Pages || !Separate(sizes,count*4,this,sizeof(*this)) || !banks.Matches(proof))return false;
  attempted=true;if(!canonical.Build())return Fail();
  uint64_t next[7];for(unsigned b=0;b<7;b++)next[b]=canonical.Used(b);unsigned n=0;
  for(unsigned i=0;i<canonical.Count();i++){
   const auto* p=canonical.Part(i);if(p->kind!=canonical.WeightKind || p->layer>=64 || p->role<2 || p->role>4)continue;
   if(n>=1344 || p->type!=12 || p->bytes%2304)return Fail();
   const unsigned logical=sizes[n],tiles=p->bytes/2304,allocated=Q::Allocated(logical);
   if(!logical || (logical&15) || logical<((tiles*4+15)&~15U) || uint64_t(logical)>uint64_t(tiles)*528+16
    || logical>Q::OffsetMask || next[p->bank]>canonical.Capacity || allocated>canonical.Capacity-next[p->bank]
    || !banks.ImmutableRange(proof,p->bank,next[p->bank],allocated))return Fail();
   const unsigned page=p->bank*unsigned(canonical.Capacity/4096)+unsigned(next[p->bank]/4096);
   for(unsigned at=0;at<allocated/4096;at++)if(initialized[page+at]!=1)return Fail();
   sides[n++]={i,p->bank,canonical.Base+unsigned(next[p->bank]),logical,allocated};next[p->bank]+=allocated;
  }
  if(n!=1344)return Fail();
  context=c;cookie=fd;hashes=h;ledger=initialized;team=owner;generation=gen;prepared=true;return true;
 }
 bool Accept(unsigned index,const uint32_t* data,unsigned words){
  if(!prepared || sealed || failed || index!=phases || index>=192 || !data || words!=L::Bytes/4
   || !Separate(data,L::Bytes,this,sizeof(*this)))return false;
  const unsigned layer=index/3,role=index%3+2;
  if(!L::Header(data,layer,role))return Fail();
  auto* jobs=new(std::nothrow) PioneerQ38ProjectionJobs;if(!jobs)return Fail();
  bool okay=jobs->Build(canonical,layer,role,0) && jobs->Count()==119 && jobs->Waves()==1;
  for(unsigned w=0;w<119 && okay;w++){
   const auto* part=jobs->Part(w);const auto* d=data+16+w*16;const auto* delta=data+L::LedgerOffset/4+w*4;
   if(!part){okay=false;break;}
   const auto* stripe=canonical.ProjectionStripe(layer,role,part->stripe);unsigned selected=canonical.Count();
   for(unsigned i=0;i<canonical.Count();i++)if(canonical.Part(i)==stripe){selected=i;break;}
   const auto* side=Find(selected);
   if(!stripe || !side || part->offset<stripe->offset || (part->offset-stripe->offset)%2304){okay=false;break;}
   const unsigned tile=unsigned((part->offset-stripe->offset)/2304);
   uint32_t job[16]={},bound[16]={};unsigned reads=0,flits=0;
   job[1]=12;job[2]=2304;job[5]=role==4?17408:5120;job[6]=canonical.Base+unsigned(part->offset);
   job[7]=part->groups;job[8]=PioneerQ38ProjectionRoute(part->bank);job[11]=w+1;job[12]=1;job[14]=1;
   okay=!d[2] && d[8]==side->address && d[9]==side->bytes && d[10]==tile && d[11]==stripe->bytes/2304
    && d[12]==canonical.Base+stripe->offset && d[13]==stripe->bytes
    && L::Bind(data,layer,role,d,delta,job,part->firstRow,part->bank,bound,reads,flits);
  }
  delete jobs;if(!okay)return Fail();
  for(unsigned i=(L::LedgerOffset+119*16)/4;i<L::Bytes/4;i++)if(data[i])return Fail();
  for(unsigned i=0;i<L::Bytes/4;i++)tables[index][i]=data[i];
  phases++;return true;
 }
 bool Seal(){if(!prepared || sealed || failed || phases!=192)return false;sealed=true;return true;}
 bool Matches(const void* c,const void* fd,const void* h,const uint8_t* initialized,int64_t owner,uint64_t gen)const{
  return Ready() && context==c && cookie==fd && hashes==h && ledger==initialized && team==owner && generation==gen;
 }
 bool MatchesPlan(const PioneerQ38CanonicalModelPlan& plan)const{
  if(!Ready() || !plan.Ready() || plan.Count()!=canonical.Count())return false;
  for(unsigned b=0;b<7;b++)if(plan.Used(b)!=canonical.Used(b))return false;
  return true; // both exact immutable one-shot canonical types;no supplied parts/routes
 }
 bool Overlay(unsigned layer,unsigned offset,uint8_t* output,unsigned bytes)const{
  if(!Ready() || layer>=64 || !bytes || ((offset|bytes)&63) || offset>0x80000 || bytes>0x80000-offset || !Output(output,bytes))return false;
  for(unsigned role=2;role<=4;role++){
   const unsigned first=L::PhaseOffset(role),end=first+L::Bytes;
   const unsigned begin=offset>first?offset:first,last=offset+bytes<end?offset+bytes:end;
   if(begin>=last)continue;
   for(unsigned at=begin;at<last;at++)output[at-offset]=uint8_t(tables[layer*3+role-2][(at-first)/4]>>(((at-first)&3)*8));
  }
  return true;
 }
};
static_assert(sizeof(PioneerQ38PreparedNativeSource)>2097152 && sizeof(PioneerQ38PreparedNativeSource)<3145728,
 "complete immutable prepared-source metadata is HEAP ONLY");
#endif
