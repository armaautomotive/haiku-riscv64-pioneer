/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphPlan.h"
#include <memory>
#include <type_traits>
static_assert(!std::is_copy_constructible<PioneerQ38ProjectionGraphPlan>::value
 && !std::is_move_constructible<PioneerQ38ProjectionGraphPlan>::value,"borrowed immutable heap plan");
struct ParentPlanFixture {
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38ProjectionJobs> jobs=std::make_unique<PioneerQ38ProjectionJobs>();
 std::unique_ptr<PioneerQ38ProjectionGraphPlan> graph=std::make_unique<PioneerQ38ProjectionGraphPlan>();
 uint32_t root[16]={},sources[119][16]={},sequence[119]={};
 PioneerQ38GdnIdentity id={1,2,3,4};unsigned layer,role,bank=0,phase=100;uint64_t token=123,input=UINT64_C(0x10000002a);bool stop=false;
 ParentPlanFixture(unsigned l,unsigned r,unsigned b=0,bool terminal=false):layer(l),role(r),bank(b),stop(terminal){
  assert(canonical->Build() && jobs->Build(*canonical,layer,role,token));
  for(unsigned i=0;i<16;i++)root[i]=i?0x12340000+i:0x16005;
  for(unsigned w=0;w<119;w++){sequence[w]=w%7;for(unsigned i=0;i<16;i++)sources[w][i]=i?0x23450000+w*16+i:0x16005;}
  if(stop)input=0;
 }
 bool Build(){return graph->Build(*canonical,*jobs,layer,role,token,bank,id,root,sources,sequence,phase,input,stop);}
 bool Match()const{return graph->Matches(*canonical,*jobs,layer,role,token,bank,id,root,sources,sequence,phase,input,stop);}
};
#ifndef PIONEER_Q38_PROJECTION_GRAPH_PLAN_FIXTURE_ONLY
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned selections=0,records=0,refusals=0,mutations=0;
 for(unsigned l=0;l<=64;l++)for(unsigned r=0;r<14;r++){
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());unsigned t=0,k=0,n=0;
  if(!canonical->ProjectionGeometry(l,r,t,k,n))continue;
  for(bool stop:{false,true}){
   ParentPlanFixture f(l,r,l%7,stop);assert(!f.graph->Manifest() && !f.graph->Sources() && f.Build() && f.Match() && !f.Build());
   const auto* m=f.graph->Manifest();assert(PioneerQ38ProjectionGraphManifest(m,f.graph->Sources()));
   for(unsigned i=0;i<f.jobs->Count();i++){
    const auto* a=m+192+i*8;const auto& j=*f.jobs->Part(i);
    assert(a[0]==f.canonical->Base+j.offset && a[1]==j.bank && a[2]==j.groups && a[3]==j.firstRow && a[4]==PioneerQ38ProjectionRoute(j.bank));records++;
   }
   for(unsigned w=0;w<119;w++){
    unsigned next=f.sequence[w]+unsigned(stop);
    if(!stop){next+=unsigned(w<k/256);for(unsigned wave=0;wave<f.jobs->Waves();wave++)next+=unsigned(w<f.jobs->WaveWorkers(wave));}
    assert(PioneerQ38ProjectionGraphNextPeer(m,w)==next);
   }
   assert(!f.graph->Matches(*canonical,*f.jobs,l,r,f.token,f.bank,f.id,f.root,f.sources,f.sequence,f.phase,f.input,f.stop));
   auto saved=f.id;for(unsigned i=0;i<4;i++){
    if(i==0)f.id.session++;else if(i==1)f.id.sequence++;else if(i==2)f.id.modelEpoch++;else f.id.driverGeneration++;
    assert(!f.Match());f.id=saved;refusals++;
   }
   f.phase++;assert(!f.Match());f.phase--;f.input^=1;assert(!f.Match());f.input^=1;f.token++;assert(!f.Match());f.token--;refusals+=3;
   selections++;
  }
 }
 for(const auto selected:{std::pair<unsigned,unsigned>{0,10},{0,2},{3,10},{64,0},{64,1}}){
  ParentPlanFixture f(selected.first,selected.second);assert(f.Build());auto* m=const_cast<uint32_t*>(f.graph->Manifest());
  for(unsigned i=0;i<192;i++){
   m[i]^=1;assert(!f.Match());m[i]^=1;assert(f.Match());mutations++;
  }
  for(unsigned ordinal:{0U,f.jobs->Count()/2,f.jobs->Count()-1})for(unsigned word=0;word<8;word++){
   m[192+ordinal*8+word]^=1;assert(!f.Match());m[192+ordinal*8+word]^=1;mutations++;
  }
  // A whole stripe shifted coherently remains valid DATA geometry. It
  // MUST NOT match the actual canonical kernel-owned learned positions.
  unsigned b=m[193];for(unsigned i=0;i<f.jobs->Count();i++)if(m[192+i*8+1]==b)m[192+i*8]+=64;
  assert(PioneerQ38ProjectionGraphManifest(m,f.graph->Sources()) && !f.Match());mutations++;
  for(unsigned i=0;i<f.jobs->Count();i++)if(m[192+i*8+1]==b)m[192+i*8]-=64;
  assert(f.Match());
 }
 {
  ParentPlanFixture f(0,10);assert(f.Build());auto* source=const_cast<uint32_t*>(f.graph->Sources());
  for(unsigned i=0;i<119*16;i++){source[i]^=1;assert(!f.Match());source[i]^=1;mutations++;}
  auto* m=const_cast<uint32_t*>(f.graph->Manifest());
  for(unsigned ordinal:{f.jobs->Count(),1023U})for(unsigned word=0;word<8;word++){
   m[192+ordinal*8+word]=1;assert(!f.Match());m[192+ordinal*8+word]=0;mutations++;
  }
 }
 for(unsigned invalid=0;invalid<12;invalid++){
  ParentPlanFixture f(0,10);if(!invalid)f.id.session=0;if(invalid==1)f.root[0]=0;if(invalid==2)f.sources[118][0]=0;
  if(invalid==3)f.sequence[118]=101;if(invalid==4)f.phase=0x7fffffff;if(invalid==5)f.bank=7;if(invalid==6)f.input=0;
  if(invalid==7)f.stop=true;if(invalid==8)f.token++;if(invalid==9)f.layer=3;if(invalid==10)f.role=6;if(invalid==11)f.phase=0;
  assert(!f.Build() && !f.graph->Manifest() && !f.graph->Sources() && !f.Build());refusals++;
 }
 printf("PASS private canonical full-projection manifest DATA selections%u/records%u/external refusals%u/mutations%u;ALL498 RUN+STOP/all waves/source+identity+sequence+producer-ticket bindings,coherent foreign learned shift refused;NO_IO_SOURCE_AUTH_OR_NATIVE_GRANT\n",selections,records,refusals,mutations);
}
#endif
