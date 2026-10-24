/* SPDX-License-Identifier: MIT */
// Standalone metadata state machine ONLY;NO native owner/ready/IO claim.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnLayerProgress.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <type_traits>
static_assert(!std::is_copy_constructible<PioneerQ38GdnLayerProgress>::value
 && !std::is_move_constructible<PioneerQ38GdnLayerProgress>::value,"owner-bound metadata cannot be copied/moved");
static const PioneerQ38GdnIdentity id={1,2,3,4};
static unsigned refusals=0;
static void Refused(bool value){assert(!value);refusals++;}
static void Snapshot(const PioneerQ38GdnLayerProgress& p,const void* owner,uint64_t mask,unsigned next,bool pending){
 uint64_t actual=UINT64_MAX;unsigned layer=~0U;bool active=!pending;
 assert(p.Snapshot(owner,id,actual,layer,active) && actual==mask && layer==next && active==pending);
}
int main(){
 int owner=0,foreign=0;
 {
  PioneerQ38GdnLayerProgress p;uint64_t mask=99;unsigned next=99;bool active=true;
  Refused(p.Snapshot(&owner,id,mask,next,active));assert(mask==99 && next==99 && active);
  Refused(p.ClaimNext(&owner,id,1,18));Refused(p.PublishNext(&owner,id,1,18));
  Refused(p.Published(&owner,id,0,17));Refused(p.OwnsPending(&owner,id,1,18));
  Refused(p.BindAfterFirstLayer(nullptr,id,17));
  for(unsigned phase:{0U,0x7fffffffU,0x80000000U,~0U})Refused(p.BindAfterFirstLayer(&owner,id,phase));
  for(unsigned field=0;field<4;field++){
   auto bad=id;if(!field)bad.session=0;if(field==1)bad.sequence=0;if(field==2)bad.modelEpoch=0;if(field==3)bad.driverGeneration=0;
   Refused(p.BindAfterFirstLayer(&owner,bad,17));
  }
  assert(p.BindAfterFirstLayer(&owner,id,17));Refused(p.BindAfterFirstLayer(&owner,id,18));Snapshot(p,&owner,1,1,false);
  uint64_t published=1;unsigned phase=17,ordinal=1,completed=1;
  for(unsigned layer=1;layer<64;layer++)if(layer%4!=3){
   for(unsigned badLayer:{0U,layer+1,64U,~0U})Refused(p.ClaimNext(&owner,id,badLayer,phase+7));
   for(unsigned badPhase:{0U,phase,0x7fffffffU,~0U})Refused(p.ClaimNext(&owner,id,layer,badPhase));
   Refused(p.ClaimNext(&foreign,id,layer,phase+7));Refused(p.ClaimNext(nullptr,id,layer,phase+7));
   for(unsigned field=0;field<4;field++){
    auto bad=id;if(!field)bad.session++;if(field==1)bad.sequence++;if(field==2)bad.modelEpoch++;if(field==3)bad.driverGeneration++;
    Refused(p.ClaimNext(&owner,bad,layer,phase+7));
    uint64_t mask=99;unsigned next=99;bool active=true;Refused(p.Snapshot(&owner,bad,mask,next,active));assert(mask==99 && next==99 && active);
   }
   Refused(p.PublishNext(&owner,id,layer,phase+7));Snapshot(p,&owner,published,layer,false);
   assert(p.ClaimNext(&owner,id,layer,phase+7));Snapshot(p,&owner,published,layer,true);
   assert(p.OwnsPending(&owner,id,layer,phase+7) && p.Published(&owner,id,0,17));
   Refused(p.Published(&owner,id,layer,phase+7));Refused(p.ClaimNext(&owner,id,layer,phase+8));
   Refused(p.PublishNext(&foreign,id,layer,phase+7));Refused(p.PublishNext(&owner,id,layer,phase+8));
   Refused(p.PublishNext(&owner,id,layer+1,phase+7));Snapshot(p,&owner,published,layer,true);
   Refused(p.OwnsPending(&foreign,id,layer,phase+7));Refused(p.Published(&foreign,id,0,17));
   for(unsigned field=0;field<4;field++){
    auto bad=id;if(!field)bad.session++;if(field==1)bad.sequence++;if(field==2)bad.modelEpoch++;if(field==3)bad.driverGeneration++;
    Refused(p.PublishNext(&owner,bad,layer,phase+7));Refused(p.OwnsPending(&owner,bad,layer,phase+7));Refused(p.Published(&owner,bad,0,17));
   }
   Snapshot(p,&owner,published,layer,true);
   phase+=7;assert(p.PublishNext(&owner,id,layer,phase));published|=UINT64_C(1)<<ordinal++;completed++;
   unsigned nextLayer=layer+1;if(nextLayer%4==3)nextLayer++;
   Snapshot(p,&owner,published,nextLayer,false);assert(p.Published(&owner,id,layer,phase));
   Refused(p.OwnsPending(&owner,id,layer,phase));Refused(p.PublishNext(&owner,id,layer,phase));
  }
  assert(completed==48 && ordinal==48 && published==UINT64_C(0xffffffffffff));
  Refused(p.ClaimNext(&owner,id,64,phase+1));Refused(p.BindAfterFirstLayer(&owner,id,phase+1));
  Refused(p.ClaimNext(&owner,id,1,phase+1));
  for(unsigned layer=3;layer<64;layer+=4)Refused(p.Published(&owner,id,layer,17));
 }
 for(unsigned stage=0;stage<3;stage++){
  PioneerQ38GdnLayerProgress p;
  if(stage)assert(p.BindAfterFirstLayer(&owner,id,17));if(stage==2)assert(p.ClaimNext(&owner,id,1,24));
  p.Fault();p.Fault();uint64_t mask=99;unsigned next=99;bool active=true;
  Refused(p.Snapshot(&owner,id,mask,next,active));assert(mask==99 && next==99 && active);
  Refused(p.BindAfterFirstLayer(&owner,id,17));Refused(p.ClaimNext(&owner,id,1,24));Refused(p.PublishNext(&owner,id,1,24));
  Refused(p.Published(&owner,id,0,17));Refused(p.OwnsPending(&owner,id,1,24));
 }
 printf("PASS CPU_METADATA_ONLY GDN per-layer progress:48ordered recurrent layers/fresh monotonic phases/foreign owner+identity/gap+repeat+pending+stale+overflow/atomic outputs/sticky quarantine/noncopy;refusals%u;NO native producer/initialized state/full-attention/token-commit/IO authority\n",refusals);
}
