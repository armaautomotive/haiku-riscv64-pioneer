/* SPDX-License-Identifier: MIT */
// Compile the same adapter against the real GGUF, packed-plan and BFP8
// source types. This does not open files, select Work mode or touch a card.
#define PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38NativeBfpBandSource.h"
#include "../PioneerQ38HybridPreparedPages.h"
using Prepared=PioneerQ38HybridPreparedPages<PioneerQwen38Gguf,PioneerQwen38PackedPlan,PioneerQ38NativeBfpBandSource>;
bool CompileRealHybridSource(PioneerQwen38Gguf& source,const PioneerQwen38PackedPlan& plan,
 const PioneerQ38CanonicalModelPlan& canonical,PioneerQ38NativeBfpBandSource& bfp8,
 const PioneerModelWeightCache::Digest& hash,const char* directory,std::string& error){
 Prepared pages;uint8_t data[4096];
 if(!pages.Bind(source,plan,canonical,bfp8,hash,hash,directory,[]{return true;},error))return false;
 return pages.ReadPage(0,0,data,sizeof(data),error);
}
