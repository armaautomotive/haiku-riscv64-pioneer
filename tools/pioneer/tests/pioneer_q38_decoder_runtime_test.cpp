/* SPDX-License-Identifier: MIT */
// Actual production joint Bind/ReserveMutable and private history spans.
// No device transport,initialization,producer or compute authorization.
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_PROJECTION_JOINT_FIXTURE_ONLY
#include "pioneer_q38_projection_mutable_banks_test.cpp"
static_assert(!ProjectionPublicAttach<PioneerQ38ConvHistoryBanks>::value
 && !ProjectionPublicCommit<PioneerQ38ConvHistoryBanks>::value
 && !ProjectionPublicBind<PioneerQ38ConvHistoryBanks>::value,"no public history attach/commit/raw-address bind");
static_assert(!std::is_copy_constructible<PioneerQ38ConvHistoryBanks>::value
 && !std::is_move_constructible<PioneerQ38ConvHistoryBanks>::value,"exact private history owner is retained");
static void Ordinary(ProjectionJointFixture& f,unsigned bank,uint64_t offset,bool allowed){
 uint32_t a=99,r=98,i=97;
 assert(f.banks.Page(f.proof,bank,offset,a,r,i)==allowed);
 if(!allowed)assert(a==99 && r==98 && i==97);
}
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,refusals=0,historyPages=0,runtimePages=0,geometryFaults=0;
 for(unsigned selected=0;selected<7;selected++){
  ProjectionJointFixture f;PioneerResidentMutableRange ranges[7];assert(PioneerQ38RuntimeRanges(*f.plan,ranges));
  // Simulate an authenticated learned-upload ledger,not initialized runtime.
  for(unsigned i=0;i<f.plan->Count();i++){const auto& p=*f.plan->Part(i);if(p.kind!=f.plan->WeightKind)continue;
   for(unsigned page=0;page<p.allocated/4096;page++)f.learned[p.bank*786432+p.offset/4096+page]=1;
  }
  auto before=f.learned;assert(f.Bind(selected));positives++;
  auto* h=f.graph->ConvHistory(f.id);auto* g=f.graph->Recurrent(f.id);auto* p=f.graph->Projection(f.id);assert(h && g && p);
  unsigned progress=~0U;assert(h->InitializationProgress(f.id,progress) && !progress && !h->InitializationPublished(f.id));
  assert(g->InitializationProgress(f.id,progress) && !progress && !g->InitializationPublished(f.id));
  assert(p->InitializationProgress(f.id,progress) && !progress && !p->InitializationPublished(f.id));
  unsigned region=0;
  for(unsigned i=0;i<f.plan->Count();i++){
   const auto& part=*f.plan->Part(i);
   if(part.kind==f.plan->WeightKind){
    assert(f.banks.ImmutableRange(f.proof,part.bank,part.offset,part.allocated));
    Ordinary(f,part.bank,part.offset,true);continue;
   }
   for(unsigned at:{0U,part.allocated/8192,part.allocated/4096-1}){Ordinary(f,part.bank,part.offset+at*4096,false);refusals++;}
   assert(!f.banks.ImmutableRange(f.proof,part.bank,part.offset,part.allocated));
   if(part.kind!=f.plan->ConvKind)continue;
   unsigned count=0;assert(h->InitializationPages(f.id,region,count) && count==5);
   for(unsigned page=0;page<count;page++){
    PioneerQ38ConvHistoryExtent e={};assert(h->InitializeSpan(f.id,region,page,1,e)
     && e.bank==part.bank && e.address==f.plan->Base+part.offset+page*4096
     && e.index==part.bank*786432+part.offset/4096+page && e.route==PioneerQ38ProjectionRoute(part.bank) && e.pages==1 && e.bytes==4096);
    historyPages++;
   }
   PioneerQ38ConvHistoryExtent e={};assert(h->FreshInitializationSpan(f.id,region,0,5,e) && e.bytes==20480);region++;
  }
  assert(region==336 && h->InitializationRegions(f.id,region) && region==336);
  for(unsigned bank=0;bank<7;bank++){
   const auto& r=ranges[bank];assert(r.bank==bank && r.pages && r.firstPage+r.pages==f.plan->Used(bank)/4096);
   Ordinary(f,bank,uint64_t(r.firstPage-1)*4096,true);
   Ordinary(f,bank,f.plan->Used(bank),true);
   assert(!f.banks.ImmutableRange(f.proof,bank,uint64_t(r.firstPage-1)*4096,4097));
   // Exhaust ALL pages in every runtime suffix once,not merely the first
   // page of each logical tensor. No claim spills into learned or unused.
   if(!selected)for(unsigned page=0;page<r.pages;page++){Ordinary(f,bank,uint64_t(r.firstPage+page)*4096,false);runtimePages++;}
  }
  PioneerQ38ConvHistoryExtent sentinel={9,8,7,6,5,4};
  for(unsigned bad=0;bad<10;bad++){
   auto e=sentinel;auto id=f.id;if(bad==9)id.sequence++;
   assert(!h->InitializeSpan(id,bad==0?336:bad==1?UINT32_MAX:0,bad==2?5:bad==3?UINT32_MAX:0,
    bad==4?0:bad==5?6:bad==6?UINT32_MAX:bad==7?256:bad==8?1680:1,e) && !memcmp(&e,&sentinel,sizeof(e)));refusals++;
  }
  auto copy=f.proof;auto foreign=f.id;foreign.modelEpoch++;
  assert(!h->MatchesInitializationOwner(f.banks,copy,f.id) && !f.graph->ConvHistory(foreign));
  auto second=std::make_unique<PioneerQ38ProjectionMutableBanks>();assert(!second->BindCanonical(f.banks,f.proof,f.id,f.learned.data(),unsigned(f.learned.size()),*f.plan,selected));
  assert(!f.Bind(selected) && f.learned==before);refusals+=4;
  if(selected%3==0)h->Fault();else if(selected%3==1)g->Fault();else p->Fault();
  assert(h->Unsafe() && g->Unsafe() && p->Unsafe() && f.graph->Unsafe() && f.banks.Quarantined()
   && !f.graph->ConvHistory(f.id) && !f.graph->Recurrent(f.id) && !f.graph->Projection(f.id));
  auto e=sentinel;progress=~0U;assert(!h->InitializeSpan(f.id,0,0,1,e) && !memcmp(&e,&sentinel,sizeof(e))
   && !h->InitializationProgress(f.id,progress) && progress==~0U && !h->InitializationPublished(f.id));f.Inert();
  printf("PASS decoder joint bank%u all7runtime claims/history EMPTY/learned unchanged/whole-context fault\n",selected);
 }
 // Every nonzero ledger tag at first/middle/LAST runtime page in EVERY
 // bank must refuse atomically,including regions the old owner omitted.
 {ProjectionJointFixture f;PioneerResidentMutableRange ranges[7];assert(PioneerQ38RuntimeRanges(*f.plan,ranges));
  for(unsigned bank=0;bank<7;bank++)for(unsigned at:{0U,ranges[bank].pages/2,ranges[bank].pages-1})for(unsigned tag:{1U,2U,255U}){
   unsigned index=bank*786432+ranges[bank].firstPage+at;f.learned[index]=uint8_t(tag);auto before=f.learned;
   assert(!f.Bind() && !f.graph->Unsafe() && !f.graph->ConvHistory(f.id) && !f.graph->Projection(f.id) && !f.graph->Recurrent(f.id));
   for(const auto& r:ranges){Ordinary(f,r.bank,uint64_t(r.firstPage)*4096,true);Ordinary(f,r.bank,uint64_t(r.firstPage+r.pages-1)*4096,true);}
   assert(f.learned==before);f.learned[index]=0;refusals++;
  }
  assert(f.Bind());f.Inert();positives++;
 }
 // Geometry/output refusal is inert. Walk EVERY canonical part: no
 // shifted/overlapping/holey/out-of-capacity/padded-underclaim suffix.
 {ProjectionJointFixture f;PioneerResidentMutableRange sentinel[7];for(unsigned b=0;b<7;b++)sentinel[b]={9,8,7};
  for(unsigned i=0;i<f.plan->Count();i++){
   auto* p=const_cast<PioneerQ38CanonicalPart*>(f.plan->Part(i));auto saved=*p;
   for(unsigned fault=0;fault<7;fault++){
    if(!fault)p->bank=7;if(fault==1)p->offset+=4096;if(fault==2)p->offset++;
    if(fault==3)p->allocated+=4096;if(fault==4)p->bytes++;if(fault==5)p->rows=0;if(fault==6)p->kind=99;
    PioneerResidentMutableRange out[7];memcpy(out,sentinel,sizeof(out));
    assert(!PioneerQ38RuntimeRanges(*f.plan,out) && !memcmp(out,sentinel,sizeof(out)));*p=saved;geometryFaults++;
   }
  }
  // A same-sized but wrong-layer Conv stripe passes no joint reservation.
  for(unsigned i=0;i<f.plan->Count();i++){
   auto* p=const_cast<PioneerQ38CanonicalPart*>(f.plan->Part(i));if(p->kind!=f.plan->ConvKind)continue;
   auto saved=*p;p->layer=64;assert(!f.Bind() && !f.graph->Unsafe());*p=saved;geometryFaults++;break;
  }
  assert(f.Bind());f.Inert();positives++;
 }
 for(unsigned bad=0;bad<9;bad++){
  ProjectionJointFixture f;auto copy=f.proof;auto id=f.id;if(bad<4){if(!bad)id.session=0;if(bad==1)id.sequence=0;if(bad==2)id.modelEpoch=0;if(bad==3)id.driverGeneration=0;}
  assert(!f.graph->BindCanonical(f.banks,bad==4?copy:f.proof,id,bad==5?nullptr:f.learned.data(),unsigned(f.learned.size())-unsigned(bad==6),*f.plan,bad>=7?bad:0)
   && !f.graph->Unsafe() && !f.graph->ConvHistory(f.id));assert(f.Bind());f.Inert();refusals++;
 }
 printf("PASS actual decoder joint positives%u/runtimePages%u/historyPages%u/refusals%u/geometryFaults%u;SEVEN exact atomic canonical suffixes/ALL mutable kinds protected/learned immutable/EMPTY separate ledgers/no history compute grant;NO DEVICE IO OR SPEED CLAIM\n",positives,runtimePages,historyPages,refusals,geometryFaults);
}
