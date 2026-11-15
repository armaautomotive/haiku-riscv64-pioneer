/* SPDX-License-Identifier: MIT */
#include "full_offload_spill_q4_role_defines.inc"
#include "../PioneerQ38TokenSelectedPrograms.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
namespace P=PioneerQ38TokenPrograms;
namespace L=PioneerQ38TokenPushedL1;
struct SpanFile {const char* file;uint32_t address;};
static std::vector<unsigned char> Bytes(const char* directory,const char* file){
 std::ifstream in(std::string(directory)+"/"+file,std::ios::binary);assert(in);
 return {std::istreambuf_iterator<char>(in),{}};
}
static void CheckWorker(unsigned worker,const char* directory,const SpanFile* files,unsigned count){
 assert(P::CodeSpans(worker)==count);unsigned ordinal=0,last=0;
 for(unsigned part=0;part<count;part++){
  const auto span=P::CodeSpan(worker,part);const auto bytes=Bytes(directory,files[part].file);
  assert(span.address==files[part].address && span.address>=last && span.words*4==bytes.size());
  for(unsigned i=0;i<span.words;i++){
   const uint32_t address=span.address+i*4;
   const uint32_t expected=uint32_t(bytes[i*4])|uint32_t(bytes[i*4+1])<<8
    |uint32_t(bytes[i*4+2])<<16|uint32_t(bytes[i*4+3])<<24;
   uint32_t observed=0xdeadbeef;
   assert(P::CodeAddress(worker,ordinal++)==address && L::SnapshotContains(worker,address)
    && P::ProgramWord(worker,address,observed) && observed==expected);
  }
  last=span.address+span.words*4;uint32_t unchanged=0xdeadbeef;
  if(part+1==count || P::CodeSpan(worker,part+1).address!=last)
   assert(!P::ProgramWord(worker,last,unchanged) && unchanged==0xdeadbeef);
 }
 assert(ordinal==P::CodeWords(worker) && P::CodeAddress(worker,ordinal)==UINT32_MAX);
 const auto invalid=P::CodeSpan(worker,count);assert(invalid.address==UINT32_MAX && !invalid.words);
}
int main(int argc,char** argv){
 assert(argc==3 && PioneerQ38FullOffloadFirmwareSelection==13);
 assert(P::Family==L::Family && P::EogCount==5 && P::SpillRoot==1 && P::RootSections==5);
 assert(P::MacroQ4Decode==1 && P::MacroQ4PrivateJobConfig==1 && P::MacroQ4InertReturn==1);
 assert(P::WideGdn==1 && P::WideGdnImports==13 && P::WideGdnDeltaAddress==0x8e600 && P::WideGdnDeltaWords==64);
 assert(L::SpillQ4==1 && L::RootSpillWords==4094 && L::RootSavedWords==321033 && L::SavedBytes==53930208);
 const SpanFile root[]={{"boot.bin",0x6000},{"text.bin",0x40000},{"terminal.bin",0x60000},
  {"rootspill.bin",0x60008},{"conv.bin",0x64000}};
 const SpanFile peer[]={{"boot.bin",0x6000},{"globalworker.bin",0x20000},{"precise.bin",0x30000},
  {"dense.bin",0x40000},{"ffn.bin",0x60000},{"service.bin",0x68000},{"fetch.bin",0x70000},
  {"affine.bin",0x70b00},{"text.bin",0x72000},{"gdn.bin",0xb8000}};
 CheckWorker(0,argv[1],root,5);assert(P::CodeWords(0)==43760);
 CheckWorker(1,argv[2],peer,10);assert(P::CodeWords(1)==26118);
 uint32_t word=0xdeadbeef;
 assert(L::SnapshotContains(0,0x60008) && L::SnapshotContains(0,0x63ffc)
  && !P::ProgramWord(0,0x63ffc,word) && word==0xdeadbeef);
 for(unsigned address:{0x6004U,0x63ffcU,0x6bd4cU,0x6e000U,0x74000U,0x17ffc0U,0xffb20000U,UINT32_MAX})
  if(address!=0x6004U){word=0xdeadbeef;if(P::ProgramWord(0,address,word)){std::fprintf(stderr,"unexpected ROOT code address %x\n",address);assert(false);}}
 for(unsigned worker=2;worker<120;worker++)assert(P::CodeSpans(worker)==10 && P::CodeWords(worker)==26118);
 for(unsigned worker:{120U,121U,UINT32_MAX})
  assert(!P::CodeSpans(worker)&&!P::CodeWords(worker)&&P::CodeAddress(worker,0)==UINT32_MAX);
 assert(L::SnapshotBase(1)==L::RootSavedWords && L::SnapshotWords(120)==0);
}
