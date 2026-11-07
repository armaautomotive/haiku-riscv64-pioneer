/* SPDX-License-Identifier: MIT */
// Synthetic streaming and activation tests, never device or model proof.
#include "../PioneerBfp4FfnQuality.h"
#include <cassert>
#include <cstdio>
struct FakeSource {
 struct Tensor {unsigned type=12,rank=2,inputs=32,outputs=32;};
 std::array<Tensor,1> tensors={};bool okay=true;unsigned reads=0;
 bool Ready()const{return okay;}bool Stable()const{return okay;}uint64_t Epoch()const{return 1;}
 const auto& Tensors()const{return tensors;}
 bool ReadRow(unsigned tensor,unsigned row,float* output,unsigned capacity){
  if(!okay || tensor || row>=32 || capacity!=32)return false;reads++;std::fill(output,output+32,1.f);return true;
 }
};
int main(){
 using namespace PioneerBfp4FfnQuality;FakeSource source;
 std::vector<uint32_t> input(32,Bits(1.f)),output={99};
 assert(Project(source,0,input,output) && output.size()==32 && source.reads==32);
 for(auto word:output)assert(word==Bits(32.f));
 source.reads=0;std::vector<uint32_t> output8;
 assert(ProjectFormat(source,0,input,PioneerBlackholeBlockFloat::Format::Bfp8B,output8) && output8==output && source.reads==32);
 std::vector<uint8_t> packed={7};assert(PackBand(source,0,0,packed) && packed.size()==576);
 std::vector<uint8_t> packed8;assert(PackBandFormat(source,0,0,PioneerBlackholeBlockFloat::Format::Bfp8B,packed8) && packed8.size()==1088);
 const auto saved8=packed8;const auto invalidFormat=static_cast<PioneerBlackholeBlockFloat::Format>(7);
 assert(!PackBandFormat(source,0,0,invalidFormat,packed8) && packed8==saved8);
 auto beforeInvalid=output8;assert(!ProjectFormat(source,0,input,invalidFormat,output8) && output8==beforeInvalid);
 const auto packedBefore=packed;assert(!PackBand(source,0,1,packed) && packed==packedBefore);
 auto saved=output;source.okay=false;assert(!Project(source,0,input,output) && output==saved);source.okay=true;
 source.tensors[0].inputs=31;assert(!Project(source,0,input,output) && output==saved);
 std::vector<uint32_t> gate(17408,Bits(0.f)),up(17408,Bits(2.f)),activation={99};
 assert(Activate(gate,up,activation) && activation.size()==17408);for(auto word:activation)assert(word==Bits(0.f));
 gate[0]=0x7f800000;auto old=activation;assert(!Activate(gate,up,activation) && activation==old);
 PioneerBfp4PromptQuality::Result result={99,99};std::vector<uint32_t> reference(32,Bits(16.f));
 assert(Compare(saved,reference,result) && result.relativeL2==1 && result.maxAbsolute==16);
 reference[0]=0x7fc00000;auto previous=result;assert(!Compare(saved,reference,result)
  && result.relativeL2==previous.relativeL2 && result.maxAbsolute==previous.maxAbsolute);
 puts("PASS synthetic complete FFN helpers: streamed projection, activation, error metrics and atomic refusal; NO MODEL OR DEVICE");
}
