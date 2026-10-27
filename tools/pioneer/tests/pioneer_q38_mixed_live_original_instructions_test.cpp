/* SPDX-License-Identifier: MIT */
// Unflagged coordinator: ONLY sealed original bytes enter the separate
// instruction TUs. Expected products remain private in the CPU source TU.
#include "pioneer_q38_mixed_live_original_bytes.h"
#include "pioneer_q38_mixed_live_original_instructions.h"
#include <cassert>
#include <cstdio>
struct FaultOriginalSource final:PioneerQ38MixedLiveOriginalSource {
 PioneerQ38MixedLiveOriginalSource& base;unsigned fail,calls=0;bool failed=false;
 FaultOriginalSource(PioneerQ38MixedLiveOriginalSource& s,unsigned n):base(s),fail(n){}
 bool Prepare()override{return false;}
 bool SealBeforeNativeContext()override{return false;}
 bool Ready()const override{return !failed && base.Ready();}
 const PioneerQwen38Gguf* SealedModel()const override{return Ready()?base.SealedModel():nullptr;}
 bool ReadWeightPage(unsigned part,unsigned page,uint8_t* out,unsigned capacity)override{
  if(calls++==fail){if(out && capacity)out[0]=0xff;failed=true;return false;}
  return Ready() && base.ReadWeightPage(part,page,out,capacity);
 }
 bool CompareFinal(const uint32_t*,unsigned,PioneerQ38MixedLiveReferenceComparison*)const override{return false;}
 bool CompareStage(PioneerQ38MixedLiveReferenceStage,const uint32_t*,unsigned,PioneerQ38MixedLiveReferenceComparison*)const override{return false;}
 uint64_t ReferenceReads()const override{return base.ReferenceReads();}
 uint64_t ReferenceFingerprint()const override{return Ready()?base.ReferenceFingerprint():0;}
};
static void OriginalByteRefusals(PioneerQ38MixedLiveOriginalSource& source,MixedLiveOriginalBytes& bytes){
 std::array<uint8_t,64> output;output.fill(0xa5);unsigned refused=0;
 const unsigned route=PioneerQ38ProjectionRoute(0),address=0x04000000;
 assert(bytes.Read(route,address,output.data(),64));output.fill(0xa5);
 auto unchanged=[&]{for(auto value:output)assert(value==0xa5);refused++;};
 for(unsigned fail:{0U,1U,11U,12U,16U}){
  FaultOriginalSource faulty(source,fail);MixedLiveOriginalBytes stage(faulty);
  assert(!stage.Prepare() && !stage.Ready() && !stage.CachedBytes() && !stage.PageReads()
   && !stage.Prepare() && !stage.Read(route,address,output.data(),64));unchanged();
 }
 for(unsigned ordinal=0;ordinal<12;ordinal++){
  PioneerQ38Tensor expected;assert(PioneerQ38Expected(ordinal?3:64,ordinal?ordinal-1:0,expected));
  auto* tensor=const_cast<PioneerQ38Tensor*>(source.SealedModel()->Find(expected.name.c_str()));assert(tensor);
  for(unsigned field=0;field<7;field++){
   auto saved=*tensor;
   if(field==0)tensor->name+='x';if(field==1)tensor->inputs++;if(field==2)tensor->outputs++;
   if(field==3)tensor->type++;if(field==4)tensor->rank++;if(field==5)tensor->offset++;if(field==6)tensor->bytes++;
   assert(!bytes.Ready() && !bytes.CachedBytes() && !bytes.PageReads() && !bytes.Read(route,address,output.data(),64));unchanged();
   *tensor=saved;assert(bytes.Ready());
  }
 }
 assert(!bytes.Read(route,address,reinterpret_cast<uint8_t*>(UINTPTR_MAX-31),64));refused++;
 // No arbitrary learned tensor, second embedding tile, state or workspace
 // may be read through this fixed standalone-operator byte source.
 assert(!bytes.Read(route,address+46080,output.data(),64));unchanged();
 assert(!bytes.Read(route,address+46080-32,output.data(),64));unchanged();
 PioneerQwen38PackedPlan plan;assert(plan.Build(*source.SealedModel(),8192,true,true));
 for(const auto& p:plan.Parts())if(p.kind!=PIONEER_Q38_WEIGHT || (p.layer!=3 && p.layer!=64)){
  assert(!bytes.Read(PioneerQ38ProjectionRoute(p.bank),p.proposedAddress,output.data(),64));unchanged();
 }
 printf("PASS original-byte source refusals%u/partial-read failure atomically unpublished/ALL12 descriptor mutations+all7fields/overflow+cross-end+unselected tensors+state+workspace;NO device IO\n",refused);
}
int main(int argc,char** argv){
 assert(argc==7 || (argc==8 && !std::strcmp(argv[7],"--bytes-only")));setvbuf(stdout,nullptr,_IOLBF,0);
 puts("START original model SHA/source/reference preparation;NO accelerator context or Work mode");
 auto source=OpenPioneerQ38MixedLiveOriginalSource(argv[1]);assert(source);
 MixedLiveOriginalBytes bytes(*source);std::array<uint8_t,46080> raw;raw.fill(0xa5);
 assert(!bytes.Ready() && !bytes.Prepare() && !bytes.Read(0,0,raw.data(),64));
 assert(source->Prepare() && source->SealBeforeNativeContext() && bytes.Prepare() && !bytes.Prepare() && bytes.Ready());
 printf("READY authenticated original raw bytes%llu/pages%u/referenceReads%llu/private reference fingerprint%016llx;NO expected products exposed\n",
  (unsigned long long)bytes.CachedBytes(),bytes.PageReads(),(unsigned long long)source->ReferenceReads(),(unsigned long long)source->ReferenceFingerprint());
 for(auto value:raw)assert(value==0xa5);
 // Arbitrary routes, wrapping/out-of-range addresses, state/workspaces,
 // zero/oversize/null reads refuse without publishing a partial buffer.
 assert(!bytes.Read(~0U,0,raw.data(),64) && !bytes.Read(0,UINT32_MAX-15,raw.data(),64)
  && !bytes.Read(0,0,raw.data(),0) && !bytes.Read(0,0,raw.data(),46081) && !bytes.Read(0,0,nullptr,64));
 for(auto value:raw)assert(value==0xa5);
 OriginalByteRefusals(*source,bytes);
 if(argc==8)return 0;
 std::array<uint32_t,5120> embedding,final;
 ExecuteMixedOriginalEmbedding(argv[2],bytes,embedding.data(),unsigned(embedding.size()));
 MixedOriginalInstructionTrace trace;
 ExecuteMixedOriginalBlock(argv[3],argv[4],argv[5],argv[6],bytes,embedding.data(),unsigned(embedding.size()),final.data(),trace);
 // These CPU comparisons occur ONLY after actual119STOP+ROOT terminal
 // and return from the instruction TU. Reports cannot feed its arithmetic.
 const char* names[]={"embedding","normalized","qg","key","value","query","gate","halfKey","halfValue","attention",
  "projection","residual","postNormalized","ffnGate","ffnUp","activated","down","final"};
 static_assert(sizeof(names)/sizeof(*names)==PioneerQ38MixedStageCount,"all diagnostic stages");
 for(unsigned i=0;i<PioneerQ38MixedStageCount;i++){
  const auto& actual=trace.actual[i];PioneerQ38MixedLiveReferenceComparison stage;
  assert(source->CompareStage(PioneerQ38MixedLiveReferenceStage(i),actual.data(),unsigned(actual.size()),&stage));
  printf("ORIGINAL_STAGE %s/checked%u/differences%u/nonfinite%u/maxAbsolute%.12g/relativeL2%.12g/exactPassed%u\n",
   names[i],stage.checked,stage.differences,stage.nonfinite,stage.maxAbsolute,stage.relativeL2,stage.passed);
 }
 PioneerQ38MixedLiveReferenceComparison report;assert(source->CompareFinal(final.data(),unsigned(final.size()),&report));
 printf("ORIGINAL_INSTRUCTION_COMPARISON checked%u/differences%u/nonfinite%u/maxAbsolute%.12g/relativeL2%.12g/exactPassed%u;NOT silicon TPS\n",
  report.checked,report.differences,report.nonfinite,report.maxAbsolute,report.relativeL2,report.passed);
 return report.passed?0:1; // An observed mismatch MUST NOT be promoted to PASS.
}
