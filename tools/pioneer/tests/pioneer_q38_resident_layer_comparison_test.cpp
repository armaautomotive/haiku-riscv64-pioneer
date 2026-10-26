/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RESIDENT_LAYER_VALIDATION_REFERENCE 1
#define PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE 1
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38ResidentLayerValidationReference.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerReadbackPlan.h" // fixed DATA layout ONLY
#include <cassert>
#include <cstdio>
int main(){
 using C=PioneerQ38ResidentLayerComparison;using namespace PioneerQ38ResidentLayerReferenceDetail;setvbuf(stdout,nullptr,_IOLBF,0);
 static_assert(unsigned(C::Words)==unsigned(PioneerQ38ResidentLayerReadbackPlan::Words),"CPU observer covers NEW full payload,not old prefix");
 std::vector<uint32_t> conv(40960),gated(6144),state(96*8578),actual(C::Words);
 auto expected=std::make_unique<PioneerQ38ResidentLayerReferenceProducts>();
 for(unsigned i=0;i<40960;i++)conv[i]=Bits(.125f+float(i%127)*.00390625f);
 for(unsigned i=0;i<6144;i++)gated[i]=Bits(.25f+float(i%31)*.00390625f);
 for(unsigned i=0;i<5120;i++){expected->projection[i]=Bits(.5f+float(i%63)*.00390625f);expected->finalHidden[i]=Bits(1.f+float(i%63)*.00390625f);
  actual[C::Projection+i]=expected->projection[i];actual[C::Final+i]=expected->finalHidden[i];}
 for(unsigned i=0;i<10240;i++)actual[C::Conv+i]=conv[i];
 for(unsigned i=0;i<30720;i++)actual[C::History+i]=conv[10240+i];
 for(unsigned i=0;i<6144;i++)actual[C::Gated+i]=gated[i];
 for(unsigned half=0;half<96;half++)for(unsigned i=0;i<8192;i++){
  uint32_t v=Bits(.125f+float(i%31)*.00390625f);state[half*8578+322+i]=v;actual[C::Next+half*8192+i]=v;
 }
 C report;assert(PioneerQ38CompareResidentLayer(actual.data(),unsigned(actual.size()),conv,gated,state,*expected,&report)
  && report.passed && report.checked==C::Words && !report.nonfinite && !report.reserved && !report.finalRelativeL2);
 unsigned corruptions=0,refusals=0;
 const unsigned first[]={C::Final,C::Conv,C::History,C::Gated,C::Projection,C::Current,C::Next};
 const unsigned sizes[]={5120,10240,30720,6144,5120,786432,786432};
 for(unsigned region=0;region<7;region++)for(unsigned offset:{0U,sizes[region]/2,sizes[region]-1})for(bool nan:{false,true}){
  unsigned at=first[region]+offset;uint32_t saved=actual[at];actual[at]=nan?0x7fc00000:Bits(Float(saved)+1.f);
  auto before=actual;assert(PioneerQ38CompareResidentLayer(actual.data(),unsigned(actual.size()),conv,gated,state,*expected,&report)
   && !report.passed && report.checked==C::Words && report.nonfinite==unsigned(nan) && actual==before);actual[at]=saved;corruptions++;
 }
 actual[C::Current]=0x80000000;assert(PioneerQ38CompareResidentLayer(actual.data(),unsigned(actual.size()),conv,gated,state,*expected,&report)
  && !report.passed && report.currentErrors==1 && !report.nonfinite);actual[C::Current]=0;corruptions++;
 // The independent instruction-policy observer uses this SAME strict
 // comparison. Even a single-bit/one-ULP change is not a tolerance pass.
 for(unsigned region:{C::Final,C::Projection})for(unsigned i:{0U,2560U,5119U}){
  actual[region+i]^=1;assert(PioneerQ38CompareResidentLayer(actual.data(),unsigned(actual.size()),conv,gated,state,*expected,&report)
   && !report.passed && !report.nonfinite && report.finalDifferences==unsigned(region==C::Final)
   && report.projectionDifferences==unsigned(region==C::Projection));actual[region+i]^=1;corruptions++;
 }
 auto refuse=[&](const uint32_t* values,unsigned words,C* out){std::memset(&report,0xa5,sizeof(report));auto before=report;auto data=actual;
  assert(!PioneerQ38CompareResidentLayer(values,words,conv,gated,state,*expected,out)
   && !std::memcmp(&report,&before,sizeof(report)) && actual==data);refusals++;
 };
 for(unsigned n:{0U,1629120U,unsigned(C::Words)-1,unsigned(C::Words)+1,UINT32_MAX})refuse(actual.data(),n,&report);
 refuse(nullptr,C::Words,&report);refuse(actual.data(),C::Words,nullptr);
 refuse(reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3),C::Words,&report);
 refuse(actual.data(),C::Words,reinterpret_cast<C*>(UINTPTR_MAX-3));
 refuse(reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(actual.data())+1),C::Words,&report);
 refuse(actual.data(),C::Words,reinterpret_cast<C*>(actual.data()));
 for(int mode:{FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}){assert(!std::fesetround(mode));refuse(actual.data(),C::Words,&report);assert(!std::fesetround(FE_TONEAREST));}
 for(auto* v:{&conv[0],&gated.back(),&state[95*8578+322+8191],&expected->projection[0],&expected->finalHidden[5119]}){
  uint32_t saved=*v;*v=0x7fc00000;refuse(actual.data(),C::Words,&report);*v=saved;
 }
 printf("PASS NEW QLV1 ORIGINAL CPU comparison:ALL1630208words/7sections/%ucorruptions/%uatomicRefusals/exactCURRENT+0/finite references+results;strict ideal projection+final bits and prior GDN bounds,report generation is NOT numerical pass or native/Work authority\n",corruptions,refusals);
}
