/* SPDX-License-Identifier: MIT */
// Instantiate the COMPLETE metadata builder/serializer for native RISC-V
// compile/stack inspection. No device IO,driver ABI,native owner or launch.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenMetadata.h"
#include "../PioneerQ38TokenStartupPlan.h"
extern "C" bool pioneer_token_startup_build(PioneerQ38TokenStartupPlan& p,const PioneerQ38CanonicalModelPlan& c,
 unsigned bank,const PioneerQ38GdnIdentity& identity,const uint32_t* sources,const uint32_t* prompt,unsigned prompts,unsigned limit){
 return p.Build(c,bank,identity,sources,prompt,prompts,limit,false,248318,248317);
}
extern "C" bool pioneer_token_startup_read(PioneerQ38TokenStartupPlan& p,const PioneerQ38CanonicalModelPlan& c,
 unsigned layer,unsigned at,uint8_t* output,unsigned bytes,const uint8_t* ledger,unsigned pages){
 return p.Matches(c) && p.LearnedPagesPresent(ledger,pages) && p.ReadPackage(layer,at,output,bytes)
  && p.ReadSession(0,output,64) && p.ReadPrompt(0,output,64) && p.ReadHead(0,output,64)
  && p.ReadBoot(119,reinterpret_cast<uint32_t*>(output));
} // compile-only instantiation;not a transport transaction or owner proof
extern "C" void pioneer_token_startup_destroy(PioneerQ38TokenStartupPlan* p){delete p;}
extern "C" bool pioneer_token_startup_gdn_routed(uint32_t* record,unsigned layer,
 unsigned physicalWorker,unsigned position){
 if(false){uint32_t data[128]={};
  (void)PioneerQ38TokenMetadata::Outer(data,0,0,0,0,0,data);
  (void)PioneerQ38TokenMetadata::Recipe(data,0,0,0,0);
  (void)PioneerQ38TokenMetadata::Conv(data,0,0,0,0);
  (void)PioneerQ38TokenMetadata::Gdn(data,0,0,0);
  (void)PioneerQ38TokenMetadata::Dense(data,3,0,0);
 }
 return PioneerQ38TokenMetadata::GdnRedistributed(record,layer,physicalWorker,position);
}
