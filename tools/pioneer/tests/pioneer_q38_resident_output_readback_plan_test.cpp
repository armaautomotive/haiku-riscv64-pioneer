/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputReadbackPlan.h"
#include <memory>
#include <set>
#include <type_traits>
using Readback=PioneerQ38ResidentOutputReadbackPlan;
using OutputPlan=PioneerQ38ResidentOutputPlan;
using Canonical=PioneerQ38CanonicalModelPlan;
using Page=PioneerQ38ResidentOutputReadbackPage;
static_assert(sizeof(Page)==28 && !std::is_copy_constructible<Readback>::value
 && !std::is_move_constructible<Readback>::value,"fixed DATA recipe,not a transferable native owner");
static unsigned examples=0,observedPages=0,refusals=0;
static const PioneerQ38CanonicalPart& StatePart(const Canonical& c,unsigned layer,unsigned slot,unsigned head){
 const PioneerQ38CanonicalPart* found=nullptr;
 for(unsigned i=0;i<c.Count();i++){
  const auto& p=*c.Part(i);
  if(p.kind!=c.RecurrentKind || p.layer!=layer || p.role!=slot || head<p.firstRow || head-p.firstRow>=p.rows)continue;
  assert(!found && p.rowBytes==65536);found=&p;
 }
 assert(found);return *found;
}
static void Compare(const Canonical& c,unsigned layer,unsigned position,unsigned bank,unsigned slot){
 const PioneerQ38GdnIdentity id={1,UINT64_C(0x100000002),3,UINT64_C(0x200000004)};
 std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequence;
 for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*17;
 for(unsigned i=0;i<sequence.size();i++)sequence[i]=i%5;
 auto operation=std::make_unique<OutputPlan>();auto data=std::make_unique<Readback>();
 auto build=[&](Readback& r){return r.Build(c,*operation,layer,position,bank,slot,248319,17,id,sources.data(),sequence.data());};
 Page actual={11,22,33,44,55,66,77};const Page untouched=actual;
 assert(!data->Page(0,actual) && !memcmp(&actual,&untouched,sizeof(actual)) && !data->WordCount() && !data->PageCount());
 assert(operation->Build(c,layer,position,bank,slot,248319,17,id,sources.data(),sequence.data()));
 assert(build(*data) && !build(*data) && data->WordCount()==1629120 && data->PageCount()==1592);
 // Independent oracle uses public canonical PARTS,not the production
 // RecurrentRegions table,operation descriptors or readback region cache.
 const unsigned offsets[]={0x120000,0x130000,0x150000,0x20000,0x30000,0x34000,0x100000};
 const unsigned lengths[]={10240,30720,6144,3072,768,192,5120};
 std::set<uint64_t> physical;unsigned ordinal=0,word=0;
 auto check=[&](unsigned kind,unsigned index,unsigned b,uint32_t address,unsigned length){
  for(unsigned within=0;within<length;within+=1024){
   assert(data->Page(ordinal++,actual));unsigned n=length-within<1024?length-within:1024;
   assert(actual.kind==kind && actual.index==index && actual.bank==b && actual.address==address+within*4
    && actual.route==PioneerQ38ProjectionRoute(b) && actual.firstWord==word && actual.words==n);
   assert(!(actual.address&4095) && actual.address>=c.Base
    && uint64_t(actual.address)-c.Base+4096<=c.Capacity);
   assert(physical.insert((uint64_t(b)<<32)|actual.address).second);word+=n;
  }
 };
 for(unsigned kind=0;kind<7;kind++)check(kind,0,bank,c.Base+c.Workspace(bank)->offset+offsets[kind],lengths[kind]);
 assert(ordinal==56 && word==56256);
 for(unsigned side=0;side<2;side++)for(unsigned half=0;half<96;half++){
  const auto& part=StatePart(c,layer,slot^side,half/2);
  check(side?8:7,half,part.bank,c.Base+part.offset+(half/2-part.firstRow)*65536+(half&1)*32768,8192);
 }
 assert(ordinal==1592 && word==1629120 && physical.size()==1592);
 for(unsigned bad:{1592U,1593U,UINT32_MAX}){
  actual=untouched;assert(!data->Page(bad,actual) && !memcmp(&actual,&untouched,sizeof(actual)));refusals++;
 }
 if(!examples){
  // Different identity/model/source/sequence/slot/position/layer cannot
  // select another valid-looking canonical span for this SAME operation.
  auto foreign=std::make_unique<Canonical>();assert(foreign->Build());
  auto empty=std::make_unique<OutputPlan>();
  for(unsigned bad=0;bad<13;bad++){
   auto refused=std::make_unique<Readback>();auto otherId=id;if(bad==2)otherId.sequence++;
   const auto originalSource=sources[119*16],originalSequence=sequence[118];
   if(bad==3)sources[119*16]^=1;if(bad==4)sequence[118]++;
   assert(!refused->Build(bad==0?*foreign:c,bad==1?*empty:*operation,
    bad==5?1:layer,bad==6?8191:position,bad==7?1:bank,bad==8?1:slot,bad==9?0:248319,
    bad==10?18:17,otherId,bad==11?nullptr:sources.data(),bad==12?nullptr:sequence.data()));
   sources[119*16]=originalSource;sequence[118]=originalSequence;
   actual=untouched;assert(!refused->Page(0,actual) && !memcmp(&actual,&untouched,sizeof(actual))
    && !refused->WordCount() && !refused->PageCount() && !build(*refused));refusals++;
  }
  for(unsigned field:{3U,4U,5U,7U,8U,9U,14U,32U,TT_Q38_COHORT_GDN_WORD+69U}){
   auto* changed=const_cast<uint32_t*>(operation->Manifest())+field;uint32_t original=*changed;*changed^=1;
   auto refused=std::make_unique<Readback>();assert(!build(*refused) && !refused->PageCount());*changed=original;refusals++;
  }
 }
 observedPages+=ordinal;examples++;
}
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<Canonical>();assert(canonical->Build());
 for(unsigned layer=0;layer<64;layer++)if(layer%4!=3)Compare(*canonical,layer,(layer&1)?8191:0,layer%7,layer&1);
 for(unsigned bank=0;bank<7;bank++)Compare(*canonical,62,8191,bank,bank&1);
 assert(examples==55 && observedPages==55*1592);
 printf("PASS fixed resident-output readback DATA55cases/ALL48 recurrent layers+ALL7 banks/BOTH slots+position boundaries/ALL%u unique pages/contiguous1629120 words per case/refusals%u;independent canonical PART oracle,NO local a0000/old156000 output/IO/native or cleanup grant\n",observedPages,refusals);
}
