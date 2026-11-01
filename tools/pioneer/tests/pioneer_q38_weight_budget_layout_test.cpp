/* SPDX-License-Identifier: MIT */
// Independent cross-check through the EXISTING strict C++ GGUF reader and
// actual tiled storage plan. Metadata/layout ONLY;NO SHA authentication,
// payload read,dequantization,numeric reference or accelerator access.
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include <memory>
int main(int argc,char** argv){
 assert(argc==1 || argc==2);
 std::unique_ptr<Fixture> fixture;
 FILE* file=nullptr;
 if(argc==1){fixture=std::make_unique<Fixture>();file=fixture->file;}
 else file=fopen(argv[1],"rb");
 assert(file);
 PioneerQwen38Gguf model;PioneerQwen38PackedPlan plan;
 assert(model.Open(file) && plan.Build(model,8192,true,true));
 uint64_t tiledStorage=0,uploaded=0,raw=0,tiled=0,commands=0,batches=0,copies=0;
 std::set<unsigned> matrices;
 for(const auto& part:plan.Parts()){
  if(part.kind!=PIONEER_Q38_WEIGHT)continue;
  uploaded+=part.allocated;
  const auto& tensor=model.Tensors().at(part.tensor);
  if(tensor.type==0)continue;
  assert(tensor.rank==2 && !(part.rows%16));tiledStorage+=part.bytes;
  if(tensor.name=="token_embd.weight")continue;
  const unsigned tile=PioneerQ38TileBytes(tensor.type),batch=tensor.type==8?4:2;
  const uint64_t count=uint64_t(part.rows/16)*(tensor.inputs/PioneerQ38BlockElements(tensor.type));
  assert(tile && count && !(count%batch) && count*tile==part.bytes);
  matrices.insert(part.tensor);raw+=uint64_t(part.rows)*(tensor.bytes/tensor.outputs);
  tiled+=part.bytes;commands+=count;batches+=count/batch;copies+=count/batch*tile;
 }
 assert(model.Stable() && plan.MatchesSource(model));
 // Independent constants from complete fixed-model shapes,not the Python
 // report or expected data injected into a worker's instruction execution.
 assert(matrices.size()==497 && raw==UINT64_C(18237132800) && tiled==UINT64_C(18671902720));
 assert(tiledStorage==UINT64_C(19387064320) && uploaded==UINT64_C(19399024640));
 assert(commands==UINT64_C(17764480) && batches==UINT64_C(5593920) && copies==UINT64_C(7441879040));
 printf("{\"quantized_matrices\":%u,\"raw_weight_bytes\":%llu,\"tiled_weight_bytes\":%llu,"
        "\"single_tile_commands\":%llu,\"batched_commands\":%llu,\"batch_last_tile_l1_copy_bytes\":%llu,"
        "\"all_quantized_storage_tiled_bytes\":%llu,\"all_learned_page_allocation_bytes\":%llu}\n",
        unsigned(matrices.size()),(unsigned long long)raw,(unsigned long long)tiled,
        (unsigned long long)commands,(unsigned long long)batches,(unsigned long long)copies,
        (unsigned long long)tiledStorage,(unsigned long long)uploaded);
 if(!fixture)assert(!fclose(file));
}
