/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_DENSE_Q8_VERIFIED_MODEL_H
#define PIONEER_Q38_HYBRID_DENSE_Q8_VERIFIED_MODEL_H
#if !defined(PIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION) || PIONEER_Q38_DENSE_Q8_VERIFIED_MODEL_INSPECTION != 1 \
 || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWD verified model is isolated offline preparation only
#endif
#include "PioneerQ38HybridHeadVerifiedModel.h"
#include "PioneerQ38HybridDenseQ8PreparedPages.h"

// Four existing verified caches tied to the SAME live authenticated model,
// packed plan and parent page source. No conversion, device, upload or Work.
class PioneerQ38HybridDenseQ8VerifiedModel {
 using Cache=PioneerModelWeightCache;
 using Parent=PioneerQ38HybridHeadVerifiedModel;
public:
 using Pages=PioneerQ38HybridDenseQ8PreparedPages<PioneerQwen38Gguf,PioneerQwen38PackedPlan,
  PioneerQ38NativeBfpBandSource,Parent::Pages>;
private:
 Parent parent;Cache cacheDense;PioneerQ38NativeBfpBandSource dense{cacheDense};Pages pages;
 bool attempted=false,prepared=false;
 static bool Fail(std::string& error,const char* text){if(error.empty())error=text;return false;}
 bool Current()const{return parent.prepared && parent.Current() && dense.MatchesSource(parent.model);}
public:
 PioneerQ38HybridDenseQ8VerifiedModel()=default;
 PioneerQ38HybridDenseQ8VerifiedModel(const PioneerQ38HybridDenseQ8VerifiedModel&)=delete;
 PioneerQ38HybridDenseQ8VerifiedModel& operator=(const PioneerQ38HybridDenseQ8VerifiedModel&)=delete;
 bool Prepare(const char* four,const char* eight,const char* head,const char* denseDirectory,std::string& error,
  const std::function<void(unsigned,unsigned)>& progress={}){
  if(attempted)return Fail(error,"QGWD model preparation is one-shot.");
  attempted=true;error.clear();
  if(!Parent::PrivateDirectory(denseDirectory))return Fail(error,"Dense BFP8 cache directory must already be private.");
  if(!parent.Prepare(four,eight,head,error,progress))return Fail(error,"QGW8 parent preparation failed.");
  const auto digest=PioneerQ38NativeBfpBandSource::OriginalPin();
  if(!cacheDense.OpenExistingDirectory(denseDirectory,error) || !parent.Current())return Fail(error,"Dense cache refused.");
  auto live=[this]{return parent.prepared && parent.Current();};
  if(!dense.Bind(parent.model,digest,live,error) || !Current()
   || !pages.Bind(parent.model,parent.packed,*parent.canonical,dense,parent.eight,parent.pages,digest,error)
   || !pages.VerifyAll(error) || !pages.AllVerified() || !Current())return Fail(error,"Dense cache/page verification failed.");
  prepared=true;error.clear();return true;
 }
 bool Ready()const{return prepared && Current() && pages.AllVerified();}
 Pages* PreparedPages(){return Ready()?&pages:nullptr;}
 const PioneerQ38CanonicalModelPlan* Canonical()const{return Ready()?parent.canonical.get():nullptr;}
};
#endif
