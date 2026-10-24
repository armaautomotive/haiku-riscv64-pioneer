/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphImages.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionGraphImages.h"
static std::vector<uint8_t> Read(const std::string& path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
static uint32_t Word(const std::vector<uint8_t>& bytes,unsigned at){assert(at+4<=bytes.size());return uint32_t(bytes[at])|(uint32_t(bytes[at+1])<<8)|(uint32_t(bytes[at+2])<<16)|(uint32_t(bytes[at+3])<<24);}
int main(int argc,char** argv){
 assert(argc==3);const char* names[]={"boot","prefix","graph","text"};const unsigned base[]={0x6000,0x64000,0x68000,0x72000};unsigned words=0,peerWords=0;
 for(unsigned s=0;s<4;s++){
  auto image=Read(std::string(argv[1])+"/"+names[s]+".bin");assert(!(image.size()&3));
  for(unsigned i=0;i<image.size();i+=4){assert(PioneerQ38NextProjectionGraphProgramWord(0,base[s]+i)==Word(image,i));words++;}
 }
 const char* sections[]={"ffn","affine","text"};const unsigned peerBase[]={0x64000,0x70b00,0x72000};
 for(unsigned s=0;s<3;s++){
  auto image=Read(std::string(argv[2])+"/"+sections[s]+".bin");
  for(unsigned w=1;w<120;w++)for(unsigned i=0;i<image.size();i+=4){assert(PioneerQ38NextProjectionGraphProgramWord(w,peerBase[s]+i)==Word(image,i));peerWords++;}
 }
 for(unsigned w=1;w<120;w++)for(unsigned a=0x6000;a<=0x74000;a+=4)
  assert(PioneerQ38NextProjectionGraphProgramWord(w,a)==PioneerQ38ProjectionGraphProgramWord(w,a));
 for(unsigned w=0;w<120;w++)for(unsigned a:{0U,0x6001U,0x6e000U,0x6fffcU,0x74000U,0xffb20000U,0xffffffffU})
  assert(!PioneerQ38NextProjectionGraphProgramAddress(w,a) && PioneerQ38NextProjectionGraphProgramWord(w,a)==UINT32_MAX);
 unsigned changed=0;for(unsigned a=0x72000;a<0x74000;a+=4)changed+=PioneerQ38NextProjectionGraphProgramWord(0,a)!=PioneerQ38ProjectionGraphProgramWord(0,a);
 assert(!PioneerQ38NextProjectionGraphProgramAddress(120,0x6000) && changed>2);
 printf("PASS independent QPS2 catalog ROOT%uwords/ALL119 byte-exact QFP3%uwords/full old peer windows coexisting/unowned+stack+unaligned rejected;NO new lease/native install grant\n",words,peerWords);
}
