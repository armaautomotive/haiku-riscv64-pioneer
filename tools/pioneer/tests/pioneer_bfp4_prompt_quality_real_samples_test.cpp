/* SPDX-License-Identifier: MIT */
// Private real-weight fixtures and old synthetic inputs only. NO device IO.
#include "../PioneerBfp4PromptQuality.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
static uint32_t Word(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned sample=0;sample<3;sample++){
  const auto path=std::string(argv[1])+"/band"+std::to_string(sample)+".rbp4";
  std::ifstream f(path,std::ios::binary);assert(f);
  std::vector<uint8_t> bytes(102544);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());assert(f && f.peek()==std::ifstream::traits_type::eof());
  assert(Word(bytes.data())==0x34504252 && Word(bytes.data()+4)==1 && Word(bytes.data()+8)==5120);
  std::array<uint32_t,5120> input={};const unsigned inputAt=16+160*576;
  for(unsigned i=0;i<5120;i++)input[i]=(uint32_t(bytes[inputAt+i*2])|uint32_t(bytes[inputAt+i*2+1])<<8)<<16;
  std::array<uint32_t,32> predicted={};assert(PioneerBfp4PromptQuality::Predict(bytes.data()+16,160*576,input.data(),5120,predicted));
  for(unsigned row=0;row<32;row++)assert(predicted[row]==Word(bytes.data()+inputAt+10240+4*row));
 }
 puts("PASS CPU observer on three private real-weight bands: all 96 words match earlier physical-test predictions; NO DEVICE");
}
