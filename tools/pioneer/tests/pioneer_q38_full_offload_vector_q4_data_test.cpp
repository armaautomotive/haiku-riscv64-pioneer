/* SPDX-License-Identifier: MIT */
// Immutable firmware DATA/address selection only. Not a kernel or card run.
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
#include "full_offload_prepared_q4_role_defines.inc"
#else
#include "full_offload_vector_q4_role_defines.inc"
#endif
#include "../PioneerQ38TokenSelectedPrograms.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
namespace P=PioneerQ38TokenPrograms;
namespace S=PioneerQ38TokenPushedL1;
struct Code {const char* file;unsigned address;};
static auto Bytes(const char* directory,const char* name){
 std::ifstream file(std::string(directory)+"/"+name+".bin",std::ios::binary);assert(file);
 return std::vector<unsigned char>{std::istreambuf_iterator<char>(file),{}};
}
int main(int argc,char** argv){
 assert(argc==3 && P::Family==S::Family && P::EogCount==5);
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
#ifdef PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4
 assert(PioneerQ38FullOffloadFirmwareSelection==10 && P::QueuedQ4Block==1 && P::QueuedQ4Wait==0xa2ff9816U);
 assert(P::QueuedQ4EscapeBase==0x9c610 && P::QueuedQ4EscapeStride==80 && P::QueuedQ4EscapeBytes==1280);
 assert(P::InstructionPrefetch==1 && P::TwoProductQ4==1 && P::AffineReplayWords==13 && P::TokenPhaseProfile==1 && P::CounterUnroll==1);
 assert(P::PhaseProfileAddress==0x75710 && P::PhaseProfileWords==60);
#else
#ifdef PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH
 assert(PioneerQ38FullOffloadFirmwareSelection==9 && P::InstructionPrefetch==1);
 assert(P::PrefetchLimitRegister==203 && P::PrefetchControlRegister==208 && P::PrefetchRequests==4 && P::PrefetchLastByte==0xc0c67);
 assert(P::TwoProductQ4==1 && P::AffineReplayWords==13 && P::TokenPhaseProfile==1 && P::CounterUnroll==1);
 assert(P::PhaseProfileAddress==0x75710 && P::PhaseProfileWords==60);
#elif defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY)
 assert(PioneerQ38FullOffloadFirmwareSelection==8 && P::TwoProductQ4==1 && P::AffineReplayWords==13);
 assert(P::TokenPhaseProfile==1 && P::CounterUnroll==1);
 assert(P::PhaseProfileAddress==0x75710 && P::PhaseProfileWords==60);
#elif defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE)
 assert(PioneerQ38FullOffloadFirmwareSelection==7 && P::TokenPhaseProfile==1 && P::CounterUnroll==1);
 assert(P::PhaseProfileAddress==0x75710 && P::PhaseProfileWords==60);
#elif defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL)
 assert(PioneerQ38FullOffloadFirmwareSelection==6 && P::CounterUnroll==1);
#else
 assert(PioneerQ38FullOffloadFirmwareSelection==5 && P::PreparedQ4==1 && P::NativeQ4Exact==1 && P::RootSections==5);
#endif
#endif
 assert(P::PreparedQ4==1 && P::NativeQ4Exact==1 && P::RootSections==5);
 assert(S::PreparedQ4==1 && S::SavedWords==13480506 && S::SavedBytes==53922024);
#else
 assert(PioneerQ38FullOffloadFirmwareSelection==3);
 assert(P::VectorQ4Metadata==1 && P::MetadataRawUnpacks==1 && P::MetadataScalarProducts==0);
 assert(S::SavedWords==13478458 && S::SavedBytes==53913832);
#endif
 assert(P::BatchedPrefetch==1 && P::TwinBatchPrefetch==1 && P::BatchedOutputClosure==1 && P::WideGdn==1);
 assert(P::WideGdnImports==13 && P::WideGdnDeltaAddress==0x8e600 && P::WideGdnDeltaWords==64);
 const Code root[]={{"boot",0x6000},{"text",0x40000},{"terminal",0x60000},{"conv",0x64000}
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
  ,{"prepared",0x6c000}
#endif
  };
 const Code peer[]={{"boot",0x6000},{"globalworker",0x20000},{"precise",0x30000},{"dense",0x40000},
  {"ffn",0x60000},{"service",0x68000},{"fetch",0x70000},{"affine",0x70b00},{"text",0x72000},{"gdn",0xb8000}};
 unsigned total=0,refusals=0;
 for(unsigned worker=0;worker<120;worker++){
  const auto* codes=worker?peer:root;const unsigned count=worker?10:unsigned(sizeof(root)/sizeof(root[0]));unsigned ordinal=0,last=0;
  assert(P::CodeSpans(worker)==count);
  for(unsigned part=0;part<count;part++){
   const auto span=P::CodeSpan(worker,part);const auto data=Bytes(worker?argv[2]:argv[1],codes[part].file);
   assert(span.address==codes[part].address && span.address>=last && span.words*4==data.size());
   for(unsigned i=0;i<span.words;i++){
    const auto at=span.address+i*4;uint32_t word=0;const auto* b=data.data()+i*4;
    const uint32_t expected=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;
    assert(P::CodeAddress(worker,ordinal++)==at && S::SnapshotContains(worker,at)
     && P::ProgramWord(worker,at,word) && word==expected);
   }
   last=span.address+span.words*4;uint32_t unchanged=0xdeadbeef;
   assert(!P::ProgramWord(worker,last,unchanged) && unchanged==0xdeadbeef);refusals++;
  }
  assert(ordinal==P::CodeWords(worker));total+=ordinal;
#ifndef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
  assert(ordinal==(worker?25946U:40610U));
#endif
  assert(P::CodeAddress(worker,ordinal)==UINT32_MAX && P::CodeAddress(worker,UINT32_MAX)==UINT32_MAX);
  const auto invalid=P::CodeSpan(worker,count);assert(invalid.address==UINT32_MAX && !invalid.words);
  for(unsigned at:{0U,0x6001U,0x6004U,0x6e000U,0x74000U,0x83c00U,0x118000U,0x17ffc0U,0xffb20000U,UINT32_MAX}){
   uint32_t word=0xdeadbeef;assert(!P::ProgramWord(worker,at,word) && word==0xdeadbeef);refusals++;
  }
  if(worker)for(unsigned i=0;i<64;i++){
   const unsigned at=0x8e600+i*4;uint32_t word=0xdeadbeef;
   assert(S::SnapshotContains(worker,at) && !P::ProgramWord(worker,at,word) && word==0xdeadbeef);refusals++;
  }
#ifdef PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4
  if(worker)for(unsigned at=0x9c600;at<0x9cb00;at+=4){
   uint32_t word=0xdeadbeef;assert(S::SnapshotContains(worker,at) && !P::ProgramWord(worker,at,word) && word==0xdeadbeef);refusals++;
  }
#endif
 }
#ifndef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
 assert(total==3128184);
#endif
#ifdef PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4
 assert(total==3466647);
#endif
 for(unsigned worker:{120U,121U,UINT32_MAX}){
  uint32_t word=0xdeadbeef;assert(!P::CodeSpans(worker) && !P::CodeWords(worker)
   && P::CodeAddress(worker,0)==UINT32_MAX && !P::ProgramWord(worker,0x6000,word) && word==0xdeadbeef);refusals++;
 }
 printf("PASS full-card %s immutable DATA:all%u sections/%u actual code words/all120 journal ranges/%u invalid-or-DATA address refusals;NO model,native lifecycle or physical execution\n",
#ifdef PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4
  "prepared Q4",15U,
#else
  "vector Q4",14U,
#endif
  total,refusals);
}
