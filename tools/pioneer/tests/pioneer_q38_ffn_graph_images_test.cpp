/* SPDX-License-Identifier: MIT */
// Frozen DATA and footprint tests,NOT native mapping/ownership/launch.
#ifdef PIONEER_Q38_TEST_INPUT_GRAPH_CATALOG
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnInputGraphImages.h"
static uint32_t Qfg1Word(unsigned w,uint32_t a){return PioneerQ38FfnGraphProgramWord(w,a);}
#define PioneerQ38FfnGraphProgramWord PioneerQ38FfnInputGraphProgramWord
#define PioneerQ38FfnGraphExecutableAddress PioneerQ38FfnInputGraphExecutableAddress
#else
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphImages.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
static std::vector<uint8_t> Read(const char* path){FILE* f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0 && !(size&3));
 assert(!fseek(f,0,SEEK_SET));std::vector<uint8_t> data(static_cast<size_t>(size));assert(fread(data.data(),1,data.size(),f)==data.size());assert(!fclose(f));return data;}
static uint32_t Word(const std::vector<uint8_t>& d,unsigned at){assert(at+4<=d.size());return uint32_t(d[at])|(uint32_t(d[at+1])<<8)|(uint32_t(d[at+2])<<16)|(uint32_t(d[at+3])<<24);}
int main(int argc,char** argv){assert(argc==3);unsigned words=0,refusals=0;
 auto root=[&](const char* name){return Read((std::string(argv[1])+"/"+name+".bin").c_str());};
 auto peer=[&](const char* name){return Read((std::string(argv[2])+"/"+name+".bin").c_str());};
 const auto rb=root("boot"),rm=root("text"),rp=root("prefix"),rg=root("graph"),pb=peer("boot"),pm=peer("text"),pa=peer("affine"),ph=peer("ffn");
 assert(rb==pb && rb.size()==4 && Word(rb,0)==TT_Q38_FFN_GRAPH_BOOT);
 for(unsigned w=0;w<120;w++){
  assert(PioneerQ38FfnGraphPrefixBytes(w)==(w?12288U:40960U));
  for(unsigned a=0x64000;a<0x74000;a+=4){bool prefix=a-0x64000<(w?12288U:40960U),panel=a>=0x70000;
   if(!prefix && !panel){assert(!PioneerQ38FfnGraphProgramAddress(w,a) && PioneerQ38FfnGraphProgramWord(w,a)==UINT32_MAX && !PioneerQ38FfnGraphExecutableAddress(w,a));refusals++;continue;}
   unsigned at=0;const std::vector<uint8_t>* section=nullptr;
   if(!w){if(a>=0x64000 && a-0x64000<rp.size()){section=&rp;at=a-0x64000;}if(a>=0x68000 && a-0x68000<rg.size()){section=&rg;at=a-0x68000;}
    if(a>=0x72000 && a-0x72000<rm.size()){section=&rm;at=a-0x72000;}}
   else{if(a>=0x64000 && a-0x64000<ph.size()){section=&ph;at=a-0x64000;}if(a>=0x70b00 && a-0x70b00<pa.size()){section=&pa;at=a-0x70b00;}
    if(a>=0x72000 && a-0x72000<pm.size()){section=&pm;at=a-0x72000;}}
   assert(PioneerQ38FfnGraphProgramAddress(w,a) && PioneerQ38FfnGraphProgramWord(w,a)==(section?Word(*section,at):0));
   assert(PioneerQ38FfnGraphExecutableAddress(w,a)==bool(section));
   assert(PioneerQ38FfnGraphProgramImmutable(w,a)==(!w || a<0x70000 || a>=0x70b00));words++;
   for(unsigned byte=1;byte<4;byte++){assert(!PioneerQ38FfnGraphProgramAddress(w,a+byte) && PioneerQ38FfnGraphProgramWord(w,a+byte)==UINT32_MAX);refusals++;}
  }
  for(uint32_t a:{0U,0x5ffcU,0x6004U,0x63ffcU,0x74000U,0x75800U,0x80000U,0xa0000U,0xffb20000U,UINT32_MAX}){
   assert(!PioneerQ38FfnGraphProgramAddress(w,a) && !PioneerQ38FfnGraphExecutableAddress(w,a) && !PioneerQ38FfnGraphProgramImmutable(w,a) && PioneerQ38FfnGraphProgramWord(w,a)==UINT32_MAX);refusals++;}
  assert(PioneerQ38FfnGraphProgramWord(w,0x6000)==TT_Q38_FFN_GRAPH_BOOT && PioneerQ38FfnGraphProgramImmutable(w,0x6000));
 }
 for(unsigned w:{120U,121U,UINT32_MAX})for(uint32_t a:{0x6000U,0x64000U,0x68000U,0x70000U,0x72000U}){
  assert(!PioneerQ38FfnGraphPrefixBytes(w) && !PioneerQ38FfnGraphProgramAddress(w,a) && !PioneerQ38FfnGraphExecutableAddress(w,a) && !PioneerQ38FfnGraphProgramImmutable(w,a) && PioneerQ38FfnGraphProgramWord(w,a)==UINT32_MAX);refusals++;}
#ifdef PIONEER_Q38_TEST_INPUT_GRAPH_CATALOG
 unsigned changed=0;for(unsigned a=0x64000;a<0x74000;a+=4)if(PioneerQ38FfnGraphProgramAddress(0,a) && Qfg1Word(0,a)!=PioneerQ38FfnGraphProgramWord(0,a))changed++;
 assert(changed);printf("QFG2 distinct root words%u;QFG1 remains separate,ALL119 companions unchanged\n",changed);
 const char* kind="QFG2";
#else
 const char* kind="QFG1";
#endif
 printf("PASS frozen %s+QFP2 code DATA:ALL120roles/words%u/refusals%u/actual image bytes+zero padding/complete40KiB-root12KiB-peer prefixes/excluded stack+mutable scratch;NO_NATIVE_MAPPING_LIFETIME_SELECTOR_OR_SPEED_AUTHORITY\n",kind,words,refusals);
}
