/* SPDX-License-Identifier: MIT */
// Canonical DATA only,not source/initialized pages/all64/launch authority.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38GlobalChainPackage.h"
#include "../PioneerQ38GlobalRecurrentDispatch.h"
#include "../PioneerQ38GlobalRedistributedGdnPackage.h"
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
#define PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4 1
#define PIONEER_Q38_FULL_OFFLOAD_BATCHED 1
#define PIONEER_Q38_FULL_OFFLOAD_METRICS 1
#define PIONEER_Q38_FULL_OFFLOAD_CLIENT 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnL1PageAllocator.h"
#include "../PioneerQ38GlobalRedistributedTokenPackage.h"
#include "../PioneerQ38TokenMetadata.h"
#endif
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
namespace C=PioneerQ38GlobalChainLayout;namespace H=PioneerQ38GlobalHeadLayout;namespace G=H::G;
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
static void BuildStatePageMap(const PioneerQ38GdnL1PageAllocator& allocator,unsigned layer,uint32_t* map)
{
	for(unsigned i=0;i<PioneerQ38GdnL1PageAllocator::Workers*9;i++)map[i]=0;
	unsigned ordinal=0;assert(PioneerQ38GdnTaskPlacement::LayerOrdinal(layer,ordinal));
	for(unsigned logical=0;logical<PioneerQ38GdnTaskPlacement::LogicalWorkers;logical++){
		PioneerQ38GdnTaskPlacement::Route route={};PioneerQ38GdnL1PageAllocator::PageTable table={};
		assert(PioneerQ38GdnTaskPlacement::Map(ordinal,logical,route) && allocator.Resolve(route,table));
		uint32_t* row=map+(route.physicalWorker-1)*9;row[0]=table.resident?0:2;
		if(table.resident)for(unsigned page=0;page<PioneerQ38GdnL1PageAllocator::PagesPerSlot;page++)row[1+page]=table.pages[page];
	}
}
#endif
int main(){
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	if(false){uint32_t data[192]={};(void)PioneerQ38TokenMetadata::Outer(data,0,0,0,0,0,data);
		(void)PioneerQ38TokenMetadata::Recipe(data,0,0,0,0);(void)PioneerQ38TokenMetadata::Conv(data,0,0,0,0);
		(void)PioneerQ38TokenMetadata::Gdn(data,0,0,0);(void)PioneerQ38TokenMetadata::Dense(data,3,0,0);}
#endif
 static_assert(unsigned(C::HeadMagic)!=unsigned(TT_Q38_GDN_GATED_MARKER) && unsigned(C::CandidateMagic)!=unsigned(TT_Q38_GDN_RENDEZVOUS_MARKER),"new header/result records must not reuse older GDN tags");
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources={};for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540000+i+1;
 PioneerQ38GdnIdentity id={1,2,3,4};unsigned jobs=0,mutations=0,dispatch=0;
	unsigned redistributedLayers=0,redistributedRoutes=0;
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	PioneerQ38GdnL1PageAllocator pageAllocator;assert(pageAllocator.Build());
	std::array<uint32_t,PioneerQ38GdnL1PageAllocator::Workers*9> pageMap={};
	bool spillTransitionTested=false;
#endif
	for(unsigned layer=0;layer<64;layer++)if(layer%4!=3){
	#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		BuildStatePageMap(pageAllocator,layer,pageMap.data());
	#endif
  auto plan=std::make_unique<PioneerQ38RedistributedGdnPlan>();
	  const unsigned phase=200+layer,request=201+layer;
	  assert(plan->Build(*canonical,layer,37,0,0,phase,request,id,sources.data()
	#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		,pageMap.data()
	#endif
		));
	  assert(plan->Matches(*canonical,layer,37,0,0,phase,request,id,sources.data()));
  unsigned active=0;
  for(unsigned physical=1;physical<=119;physical++)active+=plan->Active(physical);
  assert(active==96);
  for(unsigned logical=0;logical<96;logical++){
   uint32_t ordinal=0;assert(PioneerQ38GdnTaskPlacement::LayerOrdinal(layer,ordinal));
   PioneerQ38GdnTaskPlacement::Route route={},sibling={};
   assert(PioneerQ38GdnTaskPlacement::Map(ordinal,logical,route)
    && PioneerQ38GdnTaskPlacement::Map(ordinal,logical^1,sibling));
   const uint32_t* record=plan->Worker(route.physicalWorker);assert(record);
   const uint32_t* input=record;const uint32_t* state=record+64;const uint32_t* gated=record+128;
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
			const uint32_t* stateMap=pageMap.data()+(route.physicalWorker-1)*9;
			assert(state[50]==stateMap[0]);
			for(unsigned page=0;page<8;page++)assert(state[42+page]==stateMap[1+page]);
#endif
   assert(input[7]==logical && input[9]==route.peer && state[9]==logical
    && state[10]==layer && state[15]==route.peer && gated[7]==logical
    && gated[9]==route.peer && gated[14]==(logical^1)+1 && gated[15]==sibling.peer);
   assert(PioneerQ38GdnRedistributedPairing(input,state)
    && PioneerQ38GdnRedistributedRendezvousShape(gated,input,state));
   uint32_t boot[64]={},output[64]={};boot[0]=PioneerQ38GlobalWorkerLayout::Magic;
   boot[1]=1;boot[2]=64;boot[3]=route.physicalWorker;boot[4]=route.peer;
   boot[5]=input[16];boot[6]=8192;boot[7]=64;
   for(unsigned i=8;i<16;i++)boot[i]=input[24+i];
   for(unsigned i=16;i<32;i++)boot[i]=input[i];
   boot[40]=PioneerQ38GlobalWorkerLayout::JobWitness;boot[41]=1;
   boot[42]=5120;boot[43]=248320;boot[44]=12;boot[45]=1;
   assert(PioneerQ38GlobalRecurrentDispatch::Binding(boot,gated));
   uint32_t internal[16]={};PioneerQ38MixedRecurrentDispatch::Internal(8,gated[6],boot[3],1,internal);
   assert(PioneerQ38ResidentGdnRedistributedServiceJob(internal,gated,input,state,
    boot+16,boot[3],boot[4]));
   assert(PioneerQ38GlobalRecurrentDispatch::GdnRedistributed(boot,input,state,gated,0,output));
   assert(output[0]==PioneerQ38GlobalWorkerLayout::Magic && output[3]==route.physicalWorker);
   redistributedRoutes++;
  }
  uint32_t* changed=const_cast<uint32_t*>(plan->Worker(1));assert(changed);
  changed[128+15]^=1;
	  assert(!plan->Matches(*canonical,layer,37,0,0,phase,request,id,sources.data()));
  changed[128+15]^=1;
	  assert(plan->Matches(*canonical,layer,37,0,0,phase,request,id,sources.data()));
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	  bool hasSpill=false;uint32_t ordinal=0;assert(PioneerQ38GdnTaskPlacement::LayerOrdinal(layer,ordinal));
	  for(unsigned logical=0;logical<PioneerQ38GdnTaskPlacement::LogicalWorkers;logical++){
		PioneerQ38GdnTaskPlacement::Route route={};assert(PioneerQ38GdnTaskPlacement::Map(ordinal,logical,route));
		hasSpill|=pageMap[(route.physicalWorker-1)*9]==2;
	  }
	  if(layer==0 || (hasSpill && !spillTransitionTested)){
		auto tokenPackage=std::make_unique<PioneerQ38GlobalRedistributedTokenPackage>();
		const unsigned tokenWorkspace=canonical->Base+canonical->Workspace(0)->offset;
		assert(tokenPackage->Build(*canonical,layer,0,0,id,sources.data(),0,tokenWorkspace+0x600000,pageMap.data()));
		unsigned residentChecks=0,spillChecks=0;
		for(unsigned logical=0;logical<PioneerQ38GdnTaskPlacement::LogicalWorkers;logical++){
			PioneerQ38GdnTaskPlacement::Route route={};assert(PioneerQ38GdnTaskPlacement::Map(ordinal,logical,route));
			const uint32_t* source=tokenPackage->GdnWorker(route.physicalWorker);assert(source);
			std::array<uint32_t,192> coldRecord={},nextRecord={};
			for(unsigned i=0;i<192;i++)coldRecord[i]=nextRecord[i]=source[i];
			assert(PioneerQ38TokenMetadata::GdnRedistributed(coldRecord.data(),layer,route.physicalWorker,0,0));
			assert(PioneerQ38TokenMetadata::GdnRedistributed(nextRecord.data(),layer,route.physicalWorker,1,1));
			const unsigned mode=pageMap[(route.physicalWorker-1)*9];
			assert(coldRecord[64+50]==mode && nextRecord[64+50]==(mode==0?1:2));
			if(mode==0)residentChecks++;else spillChecks++;
		}
		assert(residentChecks && (layer==0 || spillChecks));
		if(spillChecks)spillTransitionTested=true;
	  }
#endif
  redistributedLayers++;
 }
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
	assert(spillTransitionTested);
#endif
 for(unsigned bank=0;bank<7;bank++){
  unsigned workspace=canonical->Base+canonical->Workspace(bank)->offset;
  auto package=std::make_unique<PioneerQ38GlobalChainPackage>();assert(package->Build(*canonical,0,bank,id,sources.data(),bank,workspace+0x600000));
  std::array<uint32_t,64> boot;for(unsigned i=0;i<64;i++)boot[i]=package->Boot()[i];
  assert(C::BootShape(boot.data()) && !C::Old::A::BootShape(boot.data()) && C::LayerFrame(package->Outer(),0,bank,workspace,boot.data()));
  if(bank==0){
   auto packed=std::make_unique<PioneerQ38GlobalRedistributedGdnPackage>();
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		BuildStatePageMap(pageAllocator,0,pageMap.data());
   assert(packed->Build(*canonical,0,bank,id,sources.data(),bank,workspace+0x600000,pageMap.data()));
#else
		assert(packed->Build(*canonical,0,bank,id,sources.data(),bank,workspace+0x600000));
#endif
   const unsigned baseBytes=package->Bytes();
   assert(packed->Bytes()==baseBytes+(32+119*192)*4 && packed->Matches(*canonical));
   std::array<uint8_t,128> headerBytes={};assert(packed->Read(baseBytes,headerBytes.data(),headerBytes.size()));
   for(unsigned i=0;i<32;i++){
    uint32_t value=uint32_t(headerBytes[i*4])|(uint32_t(headerBytes[i*4+1])<<8)
     |(uint32_t(headerBytes[i*4+2])<<16)|(uint32_t(headerBytes[i*4+3])<<24);
    assert(value==packed->GdnHeader()[i]);
   }
   std::array<uint8_t,64> lastWorker={};
   const unsigned lastOffset=baseBytes+(32+119*192-16)*4;
   assert(packed->Read(lastOffset,lastWorker.data(),lastWorker.size()));
   for(unsigned i=0;i<16;i++){
    uint32_t value=uint32_t(lastWorker[i*4])|(uint32_t(lastWorker[i*4+1])<<8)
     |(uint32_t(lastWorker[i*4+2])<<16)|(uint32_t(lastWorker[i*4+3])<<24);
    assert(value==packed->GdnWorker(119)[176+i]);
   }
#ifdef PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT
		const uint32_t* firstRoute=packed->GdnWorker(1)+64;
		assert(firstRoute[50]==pageMap[0]);
		for(unsigned page=0;page<8;page++)assert(firstRoute[42+page]==pageMap[1+page]);
#endif
   assert(!packed->Read(baseBytes+1,headerBytes.data(),64));
   uint32_t* changed=const_cast<uint32_t*>(packed->GdnWorker(1));changed[128+15]^=1;
   assert(!packed->Matches(*canonical));changed[128+15]^=1;
   assert(packed->Matches(*canonical));
  }
  auto plan=std::make_unique<PioneerQ38GlobalHeadPlan>();assert(plan->Build(*canonical,bank,boot.data()) && plan->Matches(*canonical) && !plan->Build(*canonical,bank,boot.data()));
  assert(H::Shape(plan->Data(),boot.data(),bank,workspace));
  auto global=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(global->Build(*canonical,0,id,sources.data()));
  assert(plan->Data()[6]==global->NormBank() && plan->Data()[7]==global->NormAddress());
  std::vector<unsigned> covered(248320,0);
  for(unsigned ordinal=0;ordinal<973;ordinal++){
   unsigned wave=ordinal/119,worker=ordinal%119+1,previous=worker*17+wave;
   std::array<uint32_t,64> b,j,expected;assert(global->WorkerBoot(worker,b.data()) && global->Job(wave,worker,previous,expected.data()));
   assert(H::Job(plan->Data(),ordinal,previous,b.data(),j.data()) && j==expected);
   for(unsigned i=0;i<j[23]*16;i++)assert(covered.at(j[28]+i)++==0);jobs++;
  }
  for(unsigned count:covered)assert(count==1);
  auto* raw=const_cast<uint32_t*>(plan->Data());
  for(unsigned word=0;word<128;word++){
   raw[word]^=1;assert(!plan->Matches(*canonical));raw[word]^=1;assert(plan->Matches(*canonical));mutations++;
  }
  std::array<uint32_t,64> b,out;assert(global->WorkerBoot(1,b.data()));out.fill(0xa55aa55a);const auto saved=out;
  assert(!H::Job(plan->Data(),973,0,b.data(),out.data()) && out==saved);
  assert(!H::Job(plan->Data(),0,0x7ffffffe,b.data(),out.data()) && out==saved);
  assert(!H::Job(plan->Data(),0,0,b.data(),nullptr));
  assert(!H::Job(plan->Data(),0,0,b.data(),raw) && plan->Matches(*canonical));
  assert(!H::Job(plan->Data(),0,0,b.data(),b.data()) && G::BootShape(b.data()));
  // NEW G envelope preserves exact existing numeric Conv/GDN descriptors.
  auto recurrent=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
  assert(recurrent->Build(*canonical,0,0,0,bank,0,1001,1,id,sources.data(),bank,workspace+0x600000));
  for(unsigned worker=1;worker<=119;worker++){
   assert(global->WorkerBoot(worker,b.data()));const auto* table=recurrent->Conv()+worker*384;
   assert(PioneerQ38GlobalRecurrentDispatch::Binding(b.data(),table) && !PioneerQ38MixedRecurrentDispatch::Binding(b.data(),table));
   assert(PioneerQ38GlobalRecurrentDispatch::Conv(b.data(),table,7,out.data()) && out[0]==G::Magic && G::JobShape(out.data(),b.data(),7,0));dispatch++;
   if(worker<=96){const auto* gdn=recurrent->Gdn()+(worker-1)*192;
    assert(PioneerQ38GlobalRecurrentDispatch::Gdn(b.data(),gdn,gdn+64,gdn+128,8,out.data()) && out[0]==G::Magic && G::JobShape(out.data(),b.data(),8,0));dispatch++;
   }
  }
 }
 for(unsigned layer=0;layer<64;layer++){
  unsigned workspace=canonical->Base+canonical->Workspace(0)->offset;
  auto package=std::make_unique<PioneerQ38GlobalChainPackage>();assert(package->Build(*canonical,layer,0,id,sources.data(),0,workspace+0x600000) && package->Matches(*canonical));
  assert(C::LayerFrame(package->Outer(),layer,0,workspace,package->Boot()) && !C::Old::A::BootShape(package->Boot()));
  std::array<uint8_t,512> first;assert(package->Read(0,first.data(),512) && !memcmp(first.data(),package->Outer(),512));
  std::array<uint8_t,64> last;assert(package->Read(package->Bytes()-64,last.data(),64));
  assert(!package->Read(0,reinterpret_cast<uint8_t*>(package.get()),64) && package->Matches(*canonical));
 }
 printf("PASS QGC1 DATA/7 banks/%u jobs EXACT vs independent canonical scheduler/ALL248320 rows once x7/%u header mutations/%u NEW G Conv+GDN envelopes/redistributed GDN plans%u/physical routes%u/appended package serialization/oldboot+envelope rejection/noalias+no-partial-output;NO instructions/all64/source/native/physical/token/Work/TPS grant\n",jobs,mutations,dispatch,redistributedLayers,redistributedRoutes);
}
