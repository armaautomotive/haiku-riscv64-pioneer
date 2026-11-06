#include "PioneerQwen38Gguf.h"
#include "PioneerQwen38Panel.h"
#include <cstdio>
#include <cinttypes>
int main(int argc,char** argv){
 if(argc!=2)return 2;
 FILE* file=fopen(argv[1],"rb");if(!file)return 3;
 PioneerQwen38Gguf model;if(!model.Open(file))return 4;
 uint64_t count[15]={},elements[15]={},packed[15]={},stepPacked[15]={},stepTiled[15]={};
 uint64_t total=0,parameters=0,embedding=0;
 for(const auto& t:model.Tensors()){
  if(t.type>=15 || !t.inputs || !t.outputs)return 5;
  count[t.type]++;elements[t.type]+=t.inputs*t.outputs;packed[t.type]+=t.bytes;
  total+=t.bytes;parameters+=t.inputs*t.outputs;
  if(t.name=="token_embd.weight"){embedding=t.bytes;continue;}
  stepPacked[t.type]+=t.bytes;
  const unsigned tile=PioneerQ38PanelTileBytes(t.type);
  if(tile){
   const unsigned block=PioneerQ38BlockElements(t.type);
   if(t.rank!=2 || t.outputs%16 || t.inputs%block)return 6;
   stepTiled[t.type]+=(t.inputs/block)*(t.outputs/16)*tile;
  }else stepTiled[t.type]+=t.bytes;
 }
 if(!model.Stable() || count[0]+count[8]+count[12]+count[14]!=851 || !embedding)return 7;
 printf("HEADER_ONLY_EXACT851_GEOMETRY_NO_FULL_SHA_RECHECK_NO_WEIGHT_READ_NO_INFERENCE_OR_DEVICE\n");
 printf("tensors %zu parameters %" PRIu64 " packed_bytes %" PRIu64 " full_file %" PRIu64 " embedding_table_bytes %" PRIu64 "\n",model.Tensors().size(),parameters,total,PioneerQwen38Gguf::FullBytes,embedding);
 uint64_t step=0,tiled=0;
 for(unsigned type:{0U,8U,12U,14U}){
  printf("type%u tensors%" PRIu64 " parameters%" PRIu64 " packed%" PRIu64 " nonembedding_packed%" PRIu64 " nonembedding_tiled%" PRIu64 "\n",type,count[type],elements[type],packed[type],stepPacked[type],stepTiled[type]);
  step+=stepPacked[type];tiled+=stepTiled[type];
 }
 printf("single_pass_nonembedding_packed_bytes %" PRIu64 " single_pass_nonembedding_card_tiled_bytes %" PRIu64 "\n",step,tiled);
 printf("ASSUMPTIONS_one_GDDR_pass_per_nonembedding_tensor_no_reuse_or_extra_reads_excludes_embedding_lookup_state_activation_control_NoC_protocol_bytes\n");
 if(fclose(file))return 8;
}
