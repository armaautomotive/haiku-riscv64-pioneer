/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchImages.h"
#ifndef PIONEER_Q38_PEER_PHASE_QUALIFICATION
#error Actual separate private timing DATA image registry ONLY
#endif
static std::vector<uint8_t> Read(const char* root,const char* section){
 FILE* f=fopen((std::string(root)+"/"+section+".bin").c_str(),"rb");assert(f&&!fseek(f,0,SEEK_END));
 long size=ftell(f);assert(size>0&&size<=8192&&!(size&3)&&!fseek(f,0,SEEK_SET));
 std::vector<uint8_t> b(size);assert(fread(b.data(),1,b.size(),f)==b.size()&&!fclose(f));return b;
}
static uint32_t Word(const std::vector<uint8_t>& b,unsigned at){
 assert(!(at&3));if(at>=b.size())return 0;assert(at+4<=b.size());
 return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);
}
int main(int argc,char** argv){
 assert(argc==3);auto root=Read(argv[1],"text"),output=Read(argv[1],"output"),peer=Read(argv[2],"text"),affine=Read(argv[2],"affine"),fetch=Read(argv[2],"fetch");
 unsigned cases=0;assert(affine.size()<=0x1500&&fetch.size()<=0x700);
 for(unsigned worker=0;worker<120;worker++){
  for(unsigned word=0;word<2048;word++){
   assert(PioneerQ38DispatchRoleWord(worker,word)==Word(worker?peer:root,word*4));cases++;
   unsigned at=0x70000+word*4;uint32_t expected=worker?(at<0x70700?Word(fetch,at-0x70000):at>=0x70b00?Word(affine,at-0x70b00):0):Word(output,at-0x70000);
   assert(PioneerQ38DispatchPrefixWord(worker,at)==expected);cases++;
  }
  assert(PioneerQ38DispatchRoleWord(worker,2048)==UINT32_MAX);
  for(unsigned at:{0x6fffcU,0x70001U,0x72000U})assert(PioneerQ38DispatchPrefixWord(worker,at)==UINT32_MAX);
 }
 assert(PioneerQ38DispatchRoleWord(120,0)==UINT32_MAX&&PioneerQ38DispatchPrefixWord(120,0x70000)==UINT32_MAX);
 printf("PASS %u ACTUAL private timing DATA image words across120roles:bounded root/119peers,exact fetch/affine/zero gaps/operand holes/limits;NO IO/source/launch/native-speed authority\n",cases);
}
