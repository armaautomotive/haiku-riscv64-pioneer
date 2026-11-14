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
 if(!model.Stable())return 1;
 for(const auto& h:histogram)printf("HISTOGRAM waves=%u matrices=%u\n",h.first,h.second);
 printf("TOTAL quantized_matrices=%u multiwave=%u unbatched_calls=%u paired_calls=%u saved_calls=%u;"
  "STATIC_SHAPES_NOT_RUNTIME_FREQUENCY_OR_SPEED\n",matrices,more,waves,batched,waves-batched);

 // Count source-graph same-input opportunities for a future multi-matrix
 // Work packet. This deliberately checks only model metadata, identical
 // quantization type/input width, and one-wave fit. It does NOT assert that
 // llama's graph scheduler presents the nodes together, that the Work driver
 // supports the request, or that batching improves latency.
 auto find=[&](const std::string& name)->unsigned {
  for(unsigned i=0;i<model.Tensors().size();i++)if(model.Tensors()[i].name==name)return i;
  return UINT32_MAX;
 };
 auto pairable=[&](unsigned a,unsigned b)->bool {
  if(a==UINT32_MAX || b==UINT32_MAX || a==b)return false;
  const auto& x=model.Tensors()[a];const auto& y=model.Tensors()[b];
  if(!x.type || x.type!=y.type || x.inputs!=y.inputs)return false;
  PioneerQwen38FleetPlan px,py;
  return px.Build(model,layout,a,119) && py.Build(model,layout,b,119)
   && !px.Jobs().empty() && !py.Jobs().empty()
   && px.Jobs().back().wave==0 && py.Jobs().back().wave==0;
 };
 unsigned ffnPairs=0,deltaPairs=0,attentionTriples=0;
 for(unsigned layer=0;layer<64;layer++){
  char up[96],gate[96],qkv[96],attnGate[96],alpha[96],beta[96],q[96],k[96],v[96];
  snprintf(up,sizeof(up),"blk.%u.ffn_up.weight",layer);snprintf(gate,sizeof(gate),"blk.%u.ffn_gate.weight",layer);
  if(pairable(find(up),find(gate)))ffnPairs++;
  if(layer%4==3){
   snprintf(q,sizeof(q),"blk.%u.attn_q.weight",layer);snprintf(k,sizeof(k),"blk.%u.attn_k.weight",layer);
   snprintf(v,sizeof(v),"blk.%u.attn_v.weight",layer);
   if(pairable(find(q),find(k)) && pairable(find(q),find(v)) && pairable(find(k),find(v)))attentionTriples++;
  }else{
   snprintf(qkv,sizeof(qkv),"blk.%u.attn_qkv.weight",layer);snprintf(attnGate,sizeof(attnGate),"blk.%u.attn_gate.weight",layer);
   snprintf(alpha,sizeof(alpha),"blk.%u.ssm_alpha.weight",layer);snprintf(beta,sizeof(beta),"blk.%u.ssm_beta.weight",layer);
   if(pairable(find(qkv),find(attnGate)))deltaPairs++;
   if(pairable(find(alpha),find(beta)))deltaPairs++;
  }
 }
 const unsigned potentialSavings=ffnPairs+deltaPairs+attentionTriples;
 printf("SHARED_INPUT_ONE_WAVE_CANDIDATES ffn_gate_up_pairs=%u delta_projection_pairs=%u attention_qkv_triples=%u"
  " pair_calls_saved_upper_bound=%u residual_calls_upper_bound=%u;SHAPES_ONLY_GRAPH_ORDER_AND_DEVICE_UNPROVEN\n",
  ffnPairs,deltaPairs,attentionTriples,potentialSavings,
  waves>=potentialSavings?waves-potentialSavings:0);
 if(!model.Stable() || fclose(file))return 1;
}
