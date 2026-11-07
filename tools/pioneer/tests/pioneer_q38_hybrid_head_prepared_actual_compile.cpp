/* SPDX-License-Identifier: MIT */
// Compile the QGW8 page and cache APIs with real source types only.
// No source open, derived-cache write, accelerator access or Work selection.
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#define PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_BANKS_MOCK_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38NativeBfpBandSource.h"
#include "../PioneerQ38HybridHeadPreparedPages.h"
#include "../PioneerQ38HybridBfp4Preparation.h"
#include "../PioneerQ38HeadBfp8Preparation.h"
#include "../PioneerQ38HybridSpanUpload.h"
using Prepared=PioneerQ38HybridHeadPreparedPages<PioneerQwen38Gguf,PioneerQwen38PackedPlan,PioneerQ38NativeBfpBandSource>;
using Four=PioneerQ38HybridBfp4Preparation<PioneerQwen38Gguf,PioneerQwen38PackedPlan,PioneerQ38Bfp4FragmentCache,UINT64_C(0x1ffc000000000000)>;
using Head=PioneerQ38HeadBfp8Preparation<PioneerQwen38Gguf,PioneerQwen38PackedPlan>;
bool CompileRealHeadSource(PioneerQwen38Gguf& source,const PioneerQwen38PackedPlan& plan,
 const PioneerQ38CanonicalModelPlan& canonical,PioneerQ38NativeBfpBandSource& bfp8,
 const PioneerModelWeightCache::Digest& hash,const char* directory,std::string& error){
 Prepared pages;Four::Statistics four;Head::Statistics head;uint8_t data[4096];
 if(!pages.Bind(source,plan,canonical,bfp8,hash,hash,hash,directory,directory,[]{return true;},error))return false;
 return pages.ReadPage(0,0,data,sizeof(data),error)
  && Four::Run(source,plan,canonical,hash,hash,directory,true,four,error)
  && Head::Run(source,plan,canonical,hash,hash,directory,true,head,error);
}
struct CompileOnlyTransport {bool Call(int,unsigned,void*,size_t){return false;}};
bool CompileRealHeadSpan(Prepared& pages,const PioneerQ38CanonicalModelPlan& canonical,
 CompileOnlyTransport& transport,std::string& error){
 PioneerQ38HybridSpanUpload<Prepared,CompileOnlyTransport,PioneerQ38HybridHeadUploadPlan> uploader;
 return uploader.Run(pages,canonical,transport,42,error);
}
