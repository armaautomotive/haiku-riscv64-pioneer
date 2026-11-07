/* SPDX-License-Identifier: MIT */
// Recompute the fixed public synthetic data from earlier physical runs.
// No model, card access, device ownership, quality or speed claim.
#include "../PioneerBfp4PromptQuality.h"
#include "../PioneerNativeBfpBandFixture.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using Fixture=PioneerNativeBfpBandFixture;
static void Check(unsigned width,const char* path){
 std::vector<uint8_t> weights(size_t(width/32)*1088);
 std::vector<uint32_t> input(width);
 for(unsigned tile=0;tile<width/32;tile++){
  for(unsigned i=0;i<1088;i++)weights[size_t(tile)*1088+i]=Fixture::WeightByte(tile,i);
  for(unsigned i=0;i<32;i++)input[tile*32+i]=uint32_t(Fixture::InputBits(tile,i))<<16;
 }
 std::array<uint32_t,32> calculated={};
 assert(PioneerBfp4PromptQuality::PredictFormat(PioneerBlackholeBlockFloat::Format::Bfp8B,weights.data(),weights.size(),input.data(),width,calculated));
 FILE* file=fopen(path,"r");assert(file);char line[256];bool seen[2][32]={};unsigned count=0;
 while(fgets(line,sizeof(line),file)){
  if(strncmp(line,"NATIVE_BFP_BAND_RETURNED_WORD ",30))continue;
  unsigned k,pass,row,got,expected;int end=0;
  assert(sscanf(line,"NATIVE_BFP_BAND_RETURNED_WORD k%u pass%u row%u got%x expected%x%n",&k,&pass,&row,&got,&expected,&end)==5);
  assert(line[end]=='\n' && line[end+1]=='\0' && k==width && pass<2 && row<32 && !seen[pass][row]);
  assert(got==expected && got==calculated[row]);seen[pass][row]=true;count++;
 }
 assert(!ferror(file) && !fclose(file) && count==64);
 printf("PASS BFP8 CPU emulation K%u matches 64 prior physical synthetic words; NO new device run\n",width);
}
int main(int argc,char** argv){assert(argc==3);Check(5120,argv[1]);Check(17408,argv[2]);}
