/* SPDX-License-Identifier: MIT */
// Synthetic unit tests only; NO model, device or quality claim.
#include "../PioneerBfp4PromptQuality.h"
#include <cassert>
#include <cstdio>
static uint32_t Bits(float x){uint32_t v;std::memcpy(&v,&x,4);return v;}
int main(){
 std::array<float,1024> weights;weights.fill(1.f);std::array<uint8_t,576> tile={};
 assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,weights.data(),weights.size(),tile.data(),tile.size()));
 std::vector<uint8_t> packed;for(unsigned i=0;i<160;i++)packed.insert(packed.end(),tile.begin(),tile.end());
 std::array<uint32_t,5120> input;input.fill(Bits(1.f));std::array<uint32_t,32> original;original.fill(Bits(5120.f));
 PioneerBfp4PromptQuality::Result result={99,99};
 assert(PioneerBfp4PromptQuality::Compute(packed.data(),packed.size(),input.data(),original.data(),result));
 assert(result.relativeL2==0 && result.maxAbsolute==0);
 original.fill(Bits(2560.f));assert(PioneerBfp4PromptQuality::Compute(packed.data(),packed.size(),input.data(),original.data(),result));
 assert(result.relativeL2==1 && result.maxAbsolute==2560);
 const auto saved=result;
 assert(!PioneerBfp4PromptQuality::Compute(nullptr,packed.size(),input.data(),original.data(),result));
 assert(!PioneerBfp4PromptQuality::Compute(packed.data(),packed.size()-1,input.data(),original.data(),result));
 input[0]=0x7f800000;assert(!PioneerBfp4PromptQuality::Compute(packed.data(),packed.size(),input.data(),original.data(),result));
 assert(result.relativeL2==saved.relativeL2 && result.maxAbsolute==saved.maxAbsolute);
 std::vector<uint8_t> full;for(unsigned i=0;i<544;i++)full.insert(full.end(),tile.begin(),tile.end());
 std::vector<uint32_t> downInput(17408,Bits(1.f));std::array<uint32_t,32> down={};
 assert(PioneerBfp4PromptQuality::Predict(full.data(),full.size(),downInput.data(),17408,down));
 for(auto word:down)assert(word==Bits(17408.f));
 const auto before=down;assert(!PioneerBfp4PromptQuality::Predict(full.data(),full.size(),downInput.data(),17407,down) && down==before);
 puts("PASS BFP4 real-input evaluator: synthetic exact and known 100% error, invalid/atomic refusals; NO MODEL OR DEVICE");
}
