/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_VALIDATION_FIXTURE_H
#define PIONEER_Q38_PROJECTION_VALIDATION_FIXTURE_H
// Sparse,synthetic original GGUF. This is NOT public-model authentication
// or simulated device arithmetic;numeric tests name their oracle explicitly.
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
struct ProjectionValidationSource {
 PioneerQwen38Gguf model;FILE* file;bool valid=true,gpuOpen=false;
 unsigned reads=0,referenceReads=0,failRead=0,failReference=0;
 explicit ProjectionValidationSource(FILE* f):file(f){assert(model.Open(f));}
 bool Ready()const{return valid && model.Stable();}const auto& Model()const{return model;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  assert(!gpuOpen);return ++referenceReads!=failReference && Ready() && model.ReadPacked(tensor,offset,bytes,out,capacity) && Ready();
 }
 bool ReadWeightPage(const PioneerQwen38PackedPlan& plan,unsigned part,unsigned page,uint8_t* out,unsigned bytes){
  return Ready() && ++reads!=failRead && plan.ReadWeightPage(model,part,page,out,bytes);
 }
};
static void SeedProjectionValidationEmbedding(Fixture& f,unsigned token){
 for(const auto& t:f.expected){
  if(t.name=="token_embd.weight"){
   std::vector<uint8_t> row(2880);unsigned shift=(token*7+token/16)%16;
   for(unsigned block=0;block<20;block++){
    auto* b=row.data()+block*144;b[1]=b[3]=0x3c;
    for(unsigned g=0;g<4;g++){b[4+g]=1;b[8+g]=2;b[12+g]=0x21;}
    for(unsigned g=0;g<4;g++)for(unsigned c=0;c<32;c++)b[16+g*32+c]=uint8_t((c+block+shift)%16|(((c+block+shift+7)%16)<<4));
   }f.Write(t.offset+uint64_t(token)*2880,row);
  }
  if(t.name=="blk.0.attn_norm.weight"){
   std::vector<uint8_t> bytes(20480);
   for(unsigned i=0;i<5120;i++){
    uint32_t value=PioneerAttentionBits(0.5f+float(i%7)*0.125f);
    for(unsigned b=0;b<4;b++)bytes[i*4+b]=uint8_t(value>>(b*8));
   }f.Write(t.offset,bytes);
  }
 }
}
#endif
