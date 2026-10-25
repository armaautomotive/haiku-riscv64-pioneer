/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputPlan.h"
#include <array>
#include <memory>
#include <type_traits>
using OutputPlan=PioneerQ38ResidentOutputPlan;
using Canonical=PioneerQ38CanonicalModelPlan;
static_assert(!std::is_copy_constructible<OutputPlan>::value && !std::is_move_constructible<OutputPlan>::value,"immutable borrowed HEAP recipe");
static unsigned examples=0,convGroups=0,gdnHalves=0,rows=0,refusals=0,pageRefusals=0;
static const PioneerQ38CanonicalPart& Part(const Canonical& c,unsigned kind,unsigned layer,unsigned role,unsigned first){
 const PioneerQ38CanonicalPart* result=nullptr;
 for(unsigned i=0;i<c.Count();i++){
  const auto* p=c.Part(i);if(p->kind!=kind || p->layer!=layer || p->role!=role || first<p->firstRow || first-p->firstRow>=p->rows)continue;
  assert(!result);result=p;
 }
 assert(result);return *result;
}
static void Compare(const Canonical& c,unsigned layer,unsigned position,unsigned bank,unsigned slot,unsigned token){
 std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequence;
 for(unsigned i=0;i<sources.size();i++)sources[i]=0x10001+i*17;
 for(unsigned i=0;i<sequence.size();i++)sequence[i]=i%5;
 const PioneerQ38GdnIdentity id={1,UINT64_C(0x100000002),3,UINT64_C(0x200000004)};
 auto p=std::make_unique<OutputPlan>();assert(!p->Manifest() && !p->PostManifest() && !p->LearnedPagesPresent(nullptr,0));
 auto match=[&](){return p->Matches(c,layer,position,bank,slot,token,17,id,sources.data(),sequence.data());};
 assert(p->Build(c,layer,position,bank,slot,token,17,id,sources.data(),sequence.data()) && match());
 assert(!p->Build(c,layer,position,bank,slot,token,17,id,sources.data(),sequence.data()) && match());
 const auto* m=p->Manifest();const auto* q=p->PostManifest();assert(m && q && m[35]==1 && m[39]==2);
 std::array<unsigned,10240> cover={};
 for(unsigned w=0;w<120;w++){
  const auto* batch=PioneerQ38CohortConv(m,w);
  for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){
   const auto* d=batch+j*64;unsigned first=(w+j*120)*16;
   const auto& weight=Part(c,c.WeightKind,layer,12,first);const auto& history=Part(c,c.ConvKind,layer,0,first);
   assert(d[7]==first && d[11]==weight.bank && d[12]==c.Base+weight.offset+(first-weight.firstRow)*16
    && d[14]==history.bank && d[15]==c.Base+history.offset+(first-history.firstRow)*12
    && d[4]==c.Base+c.Workspace(bank)->offset && d[10]==w);
   for(unsigned i=0;i<16;i++){assert(d[16+i]==sources[w*16+i] && !cover[first+i]++);}convGroups++;
  }
 }
 for(unsigned value:cover)assert(value==1);
 for(unsigned h=0;h<96;h++){
  const auto* in=PioneerQ38CohortGdn(m,h+1);const auto* state=in+64;const auto* gated=state+64;
  const unsigned head=h/2,block=(head/16)*64;assert(in[7]==h && in[40]==position && state[11]==slot);
  for(unsigned scalar=0;scalar<2;scalar++){
   const auto& weight=Part(c,c.WeightKind,layer,7+scalar,0);assert(in[scalar?14:11]==weight.bank && in[scalar?15:12]==c.Base+weight.offset+block);
  }
  for(unsigned side=0;side<2;side++){
   const auto& r=Part(c,c.RecurrentKind,layer,slot^side,head);
   assert(state[4+side*2]==r.bank && state[5+side*2]==c.Base+r.offset+(head-r.firstRow)*65536+(h&1)*32768
    && state[12+side]==PioneerQ38ProjectionRoute(r.bank));
  }
  const auto& norm=Part(c,c.WeightKind,layer,9,0);
  assert(gated[11]==norm.bank && gated[12]==c.Base+norm.offset && gated[40]==position && gated[41]==slot);
  assert(PioneerQ38GdnRendezvousShape(gated,in,state));gdnHalves++;
 }
 std::array<unsigned,5120> outputCover={};
 for(unsigned w=0;w<119;w++){
  const auto* r=q+64+w*8;const unsigned first=q[1016+w];const auto& weight=Part(c,c.WeightKind,layer,13,first);
  assert(weight.type==8 && weight.rowBytes==6912 && r[0]==c.Base+weight.offset+(first-weight.firstRow)*6912
   && r[1]==weight.bank && r[2]>=2 && r[2]<=3 && r[5]==sources[(w+1)*16]);
  for(unsigned i=0;i<r[2]*16;i++){assert(first+i<5120 && !outputCover[first+i]++);rows++;}
 }
 for(unsigned value:outputCover)assert(value==1);
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,0);
 for(unsigned i=0;i<c.Count();i++){
  const auto& r=*c.Part(i);if(r.kind!=c.WeightKind || r.layer!=layer || (r.role!=7 && r.role!=8 && r.role!=9 && r.role!=12 && r.role!=13))continue;
  unsigned first=r.bank*unsigned(c.Capacity/4096)+r.offset/4096;std::fill(ledger.begin()+first,ledger.begin()+first+r.allocated/4096,1);
 }
 const auto original=ledger;assert(p->LearnedPagesPresent(ledger.data(),unsigned(ledger.size()))
  && !p->LearnedPagesPresent(nullptr,unsigned(ledger.size())) && !p->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())-1));
 if(!examples){
  for(unsigned i=0;i<c.Count();i++){
   const auto& r=*c.Part(i);if(r.kind!=c.WeightKind || r.layer!=layer || (r.role!=7 && r.role!=8 && r.role!=9 && r.role!=12 && r.role!=13))continue;
   unsigned first=r.bank*unsigned(c.Capacity/4096)+r.offset/4096,n=r.allocated/4096;
   for(unsigned relative:{0U,n/2,n-1})for(uint8_t invalid:{uint8_t(0),uint8_t(2)}){
    ledger[first+relative]=invalid;assert(!p->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));ledger[first+relative]=1;pageRefusals++;
   }
  }
  auto foreign=std::make_unique<Canonical>();assert(foreign->Build());
  assert(!p->Matches(*foreign,layer,position,bank,slot,token,17,id,sources.data(),sequence.data()));refusals++;
  for(unsigned i=0;i<sources.size();i+=15){const auto old=sources[i];sources[i]^=1;assert(!match());sources[i]=old;assert(match());refusals++;}
  for(unsigned i=0;i<sequence.size();i++){sequence[i]++;assert(!match());sequence[i]--;assert(match());refusals++;}
  for(unsigned field:{3U,4U,5U,7U,14U,32U,33U,38U,39U,TT_Q38_COHORT_CONV_WORD+12U,TT_Q38_COHORT_GDN_WORD+69U}){
   auto* mutableWord=const_cast<uint32_t*>(m)+field;const auto saved=*mutableWord;*mutableWord^=1;
   assert(!match() && !p->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));*mutableWord=saved;assert(match());refusals++;
  }
  for(unsigned field:{3U,4U,5U,7U,11U,64U,65U,66U,68U,69U,1016U}){
   auto* mutableWord=const_cast<uint32_t*>(q)+field;const auto saved=*mutableWord;*mutableWord^=1;assert(!match());
   if(field!=11)assert(!p->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));
   *mutableWord=saved;assert(match());refusals++;
  }
 }
 assert(ledger==original && match());examples++;
}
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 auto c=std::make_unique<Canonical>();auto empty=std::make_unique<Canonical>();assert(c->Build());
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)Compare(*c,layer,(layer&1)?8191:0,layer%7,layer&1,layer?248319:0);
 for(unsigned bank=0;bank<7;bank++)Compare(*c,62,8191,bank,bank&1,248319);
 {auto p=std::make_unique<OutputPlan>();std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequence;
  sources.fill(0x16005);sequence.fill(0x7ffffff8);const PioneerQ38GdnIdentity id={1,2,3,4};
  assert(p->Build(*c,62,8191,6,1,248319,0x7ffffff9,id,sources.data(),sequence.data())
   && p->Matches(*c,62,8191,6,1,248319,0x7ffffff9,id,sources.data(),sequence.data())
   && p->PostManifest()[7]==0x7ffffffd);}
 for(unsigned bad=0;bad<15;bad++){
  auto p=std::make_unique<OutputPlan>();std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequence={};sources.fill(0x16005);
  PioneerQ38GdnIdentity id={1,2,3,4};unsigned layer=0,position=0,bank=0,slot=0,token=0,phase=17;
  if(bad==1)layer=3;if(bad==2)layer=64;if(bad==3)position=8192;if(bad==4)bank=7;if(bad==5)slot=2;
  if(bad==6)token=248320;if(bad==7)phase=0;if(bad==8)phase=0x7ffffffa;if(bad==11)id.sequence=0;
  if(bad==12)sources[119*16]=0;if(bad==13)sequence[118]=17;if(bad==14)sequence[118]=UINT32_MAX;
  assert(!p->Build(bad?*c:*empty,layer,position,bank,slot,token,phase,id,bad==9?nullptr:sources.data(),bad==10?nullptr:sequence.data())
   && !p->Manifest() && !p->PostManifest() && !p->LearnedPagesPresent(nullptr,0));
  assert(!p->Build(*c,0,0,0,0,0,17,id,sources.data(),sequence.data()));refusals++;
 }
 assert(examples==55 && convGroups==55*640 && gdnHalves==55*96 && rows==55*5120);
 printf("PASS canonical resident-output recipe cases%u/ALL48 recurrent layers+ALL7 banks/BOTH slots/positions0+8191/phaseBoundary1/Conv groups%u/GDN halves%u/output rows%u/refusals%u/pageRefusals%u;independent canonical part addresses/complete nonoverlapping cover/all sources+sequences+identity/ledgers unchanged;DATA ONLY,NO native source/init/producer/current-slot/IO/speed grant\n",examples,convGroups,gdnHalves,rows,refusals,pageRefusals);
}
