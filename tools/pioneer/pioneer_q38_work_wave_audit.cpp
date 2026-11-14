/* SPDX-License-Identifier: MIT */
// Metadata-only audit of the actual Work planner. No device access or speed claim.
#include "PioneerQwen38FleetPlan.h"
#include <cstdio>
#include <map>

int main(int argc,char** argv){
 if(argc!=2)return 2;
 FILE* file=fopen(argv[1],"rb");if(!file)return 1;
 PioneerQwen38Gguf model;PioneerQwen38PackedPlan layout;
 if(!model.Open(file) || !layout.Build(model,8192,true)){fclose(file);return 1;}
 std::map<unsigned,unsigned> histogram;unsigned matrices=0,waves=0,batched=0,more=0;
 for(unsigned t=0;t<model.Tensors().size();t++){
  const auto& tensor=model.Tensors()[t];
  if(!tensor.type || tensor.name=="token_embd.weight")continue; // embedding is not a Work matmul
  PioneerQwen38FleetPlan plan;
  if(!plan.Build(model,layout,t,119) || plan.Jobs().empty()){fclose(file);return 1;}
  const unsigned n=plan.Jobs().back().wave+1;
  histogram[n]++;matrices++;waves+=n;batched+=(n+1)/2;more+=n>1;
  if(n>1)printf("MULTIWAVE name=%s inputs=%llu outputs=%llu type=%u waves=%u\n",tensor.name.c_str(),
   (unsigned long long)tensor.inputs,(unsigned long long)tensor.outputs,tensor.type,n);
 }
 if(!model.Stable() || fclose(file))return 1;
 for(const auto& h:histogram)printf("HISTOGRAM waves=%u matrices=%u\n",h.first,h.second);
 printf("TOTAL quantized_matrices=%u multiwave=%u unbatched_calls=%u paired_calls=%u saved_calls=%u;"
  "STATIC_SHAPES_NOT_RUNTIME_FREQUENCY_OR_SPEED\n",matrices,more,waves,batched,waves-batched);
}
