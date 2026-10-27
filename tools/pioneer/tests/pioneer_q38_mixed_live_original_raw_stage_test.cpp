/* SPDX-License-Identifier: MIT */
// RAW simulated-weight staging ONLY;no native owner/context or RUN grant.
#define PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "pioneer_q38_mixed_live_native_transport.h"
#include "../PioneerQ38OriginalWeightBytes.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
struct StageBytes final:ProjectionExecutionLearnedBytes {
 struct Span{unsigned route,address,bytes;};std::vector<Span> spans;
 unsigned reads=0,fail=0;bool loseReady=false,current=true;
 StageBytes(){auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
  for(unsigned i=0;i<c->Count();i++){const auto* p=c->Part(i);
   if(p->kind==c->WeightKind && p->layer==3 && !p->type)spans.push_back({PioneerQ38ProjectionRoute(p->bank),c->Base+p->offset,p->bytes});
  }assert(spans.size()==4);
 }
 bool Ready()const override{return current;}
 bool Read(unsigned route,uint32_t address,uint8_t* out,unsigned bytes)override{
  assert(current && reads<4);const auto& s=spans[reads++];assert(route==s.route && address==s.address && bytes==s.bytes && out);
  for(unsigned at=0;at<bytes;at+=4){uint32_t value=0x3f800000+reads;std::memcpy(out+at,&value,4);}
  if(reads==fail){if(loseReady){current=false;return true;}return false;}return true;
 }
};
int main(int argc,char** argv){
 assert(argc==5);MixedLiveNativeTransport transport(argv[1],argv[2],argv[3],argv[4]);
 auto before=transport.Fingerprint();unsigned touches=transport.Touches(),cases=0;
 {StageBytes bad;bad.current=false;assert(!transport.AttachOriginalLearned(bad) && !bad.reads);cases++;}
 for(bool lost:{false,true})for(unsigned fail=1;fail<=4;fail++){
  StageBytes bad;bad.fail=fail;bad.loseReady=lost;
  assert(!transport.AttachOriginalLearned(bad) && bad.reads==fail && transport.Touches()==touches && !transport.Resets());cases++;
 }
 StageBytes good;assert(transport.AttachOriginalLearned(good) && good.reads==4);
 assert(!transport.AttachOriginalLearned(good) && good.reads==4 && transport.Fingerprint()==before
  && transport.Touches()==touches && !transport.Resets());cases++;
 printf("PASS original RAW norm staging cases%u/4partial-read+4readiness-loss+not-ready/failed attach does not publish byte source/successful retry+one-shot attach/unchanged L1+ZERO MMIO or launch;NO native/model/source-auth/numeric/cleanup grant\n",cases);
}
