/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_prepared_bf16_q4_compare_machine.h"
#include "../PioneerNativeCompactQ4StreamSource.h"
// Bind to the SAME authenticated original FD via the existing source owner.
// It checks EVERY compiled byte and old reference in all9 selections against
// fresh original reads. This inverse PERMUTATION then supplies original rows
// to the NEW independent reference, never to a Work model/context/fallback.
static void OriginalSelection(unsigned sample,PreparedCompareData& data){
 using D=PioneerNativeCompactQ4StreamData;using M=PioneerNativeCompactQ4StreamMetadata;
 const unsigned blocks=D::Width(sample)/256;assert(data.blocks.size()==blocks);
 for(unsigned block=0;block<blocks;block++){
  auto& b=data.blocks[block];std::array<uint8_t,4608> packed{};
  for(unsigned w=0;w<1152;w++){const unsigned value=D::Word(sample,block*1152+w);for(unsigned i=0;i<4;i++)packed[w*4+i]=uint8_t(value>>(i*8));}
  for(unsigned row=0;row<32;row++){
   for(unsigned byte=0;byte<16;byte++)b.original[row*144+byte]=packed[4096+(row/16)*256+byte*16+row%16];
   for(unsigned byte=0;byte<128;byte++)b.original[row*144+16+byte]=packed[(row/16)*2048+byte*16+row%16];
  }
  for(unsigned c=0;c<256;c++)b.input[c]=uint16_t(D::Word(sample,blocks*1152+block*128+c/2)>>(c%2*16));
  std::array<uint8_t,4608> roundtrip{};assert(PioneerQ38Q4FpuCompactData::Prepare(b.original.data(),4608,roundtrip.data(),roundtrip.size()) && roundtrip==packed);
 }
 memcpy(data.expected.data(),M::Golden(sample),128);
}
int main(int argc,char** argv){
 assert(argc==3);static_cast<void>(&NoMore);PioneerNativeCompactQ4StreamSource source;std::string error;
 puts("PREPARED_BF16_PRIVATE_AUTH_SAME_OPEN_ORIGINAL_SHA_ALL9_PAYLOADS_AND_FRESH_OLD_REFS_NO_DEVICE");fflush(stdout);
 assert(source.Prepare(argv[2],error) && source.Ready());unsigned words=0,references=0;
 for(unsigned sample=0;sample<9;sample++){
  const unsigned width=PioneerNativeCompactQ4StreamData::Width(sample);PreparedCompareData data(width,sample);OriginalSelection(sample,data);
  std::array<unsigned,32> expected{};
  for(const auto& block:data.blocks)for(unsigned g=0;g<8;g++)assert(source.Ready() && PioneerQ38PreparedBf16Q4Reference::Group(block.original.data(),block.input.data(),g,expected.data()));
  references+=32;
  if(sample!=0 && sample!=6)continue;
  auto vm=std::make_unique<PreparedCompareRV32>(argv[1],width);data.Stage(vm->bus);const auto original=vm->bus.dram;
  assert(vm->Execute());for(unsigned row=0;row<32;row++){
   assert(vm->ReadL1(SL::Output+row*4)==data.expected[row] && vm->ReadL1(SL::Output+128+row*4)==expected[row]);words+=2;
  }
  assert(source.Ready() && vm->bus.dram==original);
 }
 assert(words==128 && references==288 && source.Ready());
 printf("PASS_PRIVATE_PREPARED_BF16_COMPARE sameOpenFullOriginalSHA_all9Payloads_freshOriginalByteOldAndNewReferences%u_compiledBothWidthsExact%u;NO_DEVICE_NATIVE_OWNER_FULL_MODEL_QUALITY_OR_SPEED\n",references,words);
}
