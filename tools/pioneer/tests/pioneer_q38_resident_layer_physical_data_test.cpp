/* SPDX-License-Identifier: MIT */
// Pure grammar only: a copied terminal is NEVER native/cleanup authority.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerProof.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 auto plan=std::make_unique<PioneerQ38ResidentLayerPlan>();
 std::array<uint32_t,1920> sources;std::array<uint32_t,119> sequences={};
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)sources[w*16+i]=0x12340001+w*32+i;
 assert(plan->Build(*canonical,0,0,0,0,248319,19,{1,2,3,4},sources.data(),sequences.data()));
 const auto* m=plan->Manifest();unsigned refused=0;
 uint32_t root[3]={0x7200c,5120,TT_Q38_RESIDENT_LAYER_MARKER},rpc[2]={0x72038,0x7203c};
 assert(PioneerQ38ResidentLayerRootPhysical(root,rpc,0x46800));
 for(unsigned i=0;i<3;i++){root[i]^=1;assert(!PioneerQ38ResidentLayerRootPhysical(root,rpc,0x46800));root[i]^=1;refused++;}
 root[2]=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER;assert(!PioneerQ38ResidentLayerRootPhysical(root,rpc,0x46800));root[2]=TT_Q38_RESIDENT_LAYER_MARKER;refused++;
 for(unsigned reset:{0U,0x47800U,0x87ffffU}){assert(!PioneerQ38ResidentLayerRootPhysical(root,rpc,reset));refused++;}
 for(unsigned i=0;i<2;i++)for(unsigned pc:{0U,0x72000U,0x72040U,0x72060U,0x7203dU}){
  auto old=rpc[i];rpc[i]=pc;assert(!PioneerQ38ResidentLayerRootPhysical(root,rpc,0x46800));rpc[i]=old;refused++;
 }
 assert(!PioneerQ38ResidentLayerRootPhysical(nullptr,rpc,0x46800)
  && !PioneerQ38ResidentLayerRootPhysical(root,nullptr,0x46800));
 for(unsigned w=1;w<120;w++)for(unsigned stop:{0U,1U}){
  const unsigned seq=PioneerQ38ResidentLayerSequence(m,w)+stop;
  std::array<uint32_t,16> source,status={};std::array<uint32_t,8> boot={};
  for(unsigned i=0;i<16;i++)source[i]=PioneerQ38CohortSource(m,w)[i];
  status[0]=status[1]=seq;status[3]=TT_Q38_FFN_PERSISTENT_MARKER;status[4]=stop*2;
  boot[0]=TT_Q38_FFN_PERSISTENT_MARKER;boot[1]=w;boot[2]=PioneerQ38ConvPeer(w);boot[3]=source[0];
  uint32_t pc[2]={stop?unsigned(TT_Q38_PEER_STOP):0x7204cU,stop?unsigned(TT_Q38_PEER_STOP_FETCH):0x72050U};
  auto check=[&](unsigned reset=0x46800,unsigned doorbell=UINT32_MAX){
   return PioneerQ38ResidentLayerPeerPhysical(m,w,source.data(),boot.data(),status.data(),doorbell==UINT32_MAX?seq:doorbell,pc,reset,bool(stop));
  };
  assert(check());
  for(auto* array:{&source,&status})for(unsigned i=0;i<16;i++){
   (*array)[i]^=1;assert(!check());(*array)[i]^=1;refused++;
  }
  for(unsigned i=0;i<8;i++){boot[i]^=1;assert(!check());boot[i]^=1;refused++;}
  for(unsigned reset:{0U,0x47800U,0x87ffffU}){assert(!check(reset));refused++;}
  for(unsigned doorbell:{seq-1,seq+1,PioneerQ38ResidentOutputSequence(m,w)}){assert(!check(0x46800,doorbell));refused++;}
  for(unsigned i=0;i<2;i++){
   auto old=pc[i];pc[i]=stop?0x7204cU:unsigned(TT_Q38_PEER_STOP);assert(!check());pc[i]=old;refused++;
  }
  status[0]=status[1]=PioneerQ38ResidentOutputSequence(m,w)+stop;assert(!check());refused++;
 }
 for(unsigned w:{0U,120U,UINT32_MAX})assert(PioneerQ38ResidentLayerSequence(m,w)==UINT32_MAX);
 assert(PioneerQ38ResidentLayerSequence(nullptr,1)==UINT32_MAX);
 printf("PASS NEW full-layer physical DATA grammar:ROOT QLR1 never oldQCO1,ALL119 WAIT+fresh STOP sequences/%u corrupt-source/boot/status/held/wrong-PC/doorbell/old-sequence refusals;NO native scope/IO/reset/cleanup authority\n",refused);
}
