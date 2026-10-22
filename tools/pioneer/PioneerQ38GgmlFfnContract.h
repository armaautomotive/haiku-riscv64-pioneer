/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GGML_FFN_CONTRACT_H
#define PIONEER_Q38_GGML_FFN_CONTRACT_H
#include "PioneerQwen38Gguf.h"
#include "ggml.h"
// The pinned scheduler's GLOBAL use counts are private metadata. A graph
// slice preserves these counts:local-only consumer counting is insufficient.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "ggml-impl.h"
#pragma GCC diagnostic pop
// OFFLINE/UNREGISTERED graph-shape prerequisite ONLY. No tensor value reads,
// device handles,buffers,execution,source authentication or cleanup authority.
// Describe must be repeated against a live unchanged source/graph before any
// future owner independently authorizes a fused operation. This is NOT that
// owner and MUST NOT make the existing matrix backend claim GLU support.
struct PioneerQ38GgmlFfnContract {
 const PioneerQwen38Gguf* source=nullptr;
 uint64_t epoch=0;
 unsigned layer=0,weight[3]={};
 int first=0;
 const ggml_tensor *input=nullptr,*gate=nullptr,*up=nullptr,*glu=nullptr,*down=nullptr;
 static bool FloatVector(const ggml_tensor* t,unsigned n) {
  return t && t->type==GGML_TYPE_F32 && t->ne[0]==n && t->ne[1]==1 && t->ne[2]==1 && t->ne[3]==1
   && t->nb[0]==4 && t->nb[1]==size_t(n)*4 && t->nb[2]==t->nb[1] && t->nb[3]==t->nb[2];
 }
 static bool NoExtraSources(const ggml_tensor* t,unsigned used) {
  if(!t || used>GGML_MAX_SRC)return false;
  for(unsigned i=used;i<GGML_MAX_SRC;i++)if(t->src[i])return false;
  return true;
 }
 static bool Parameters(const ggml_tensor* t,unsigned allowedFirstWord=0) {
  if(!t || unsigned(t->op_params[0])!=allowedFirstWord)return false;
  const auto* bytes=reinterpret_cast<const unsigned char*>(t->op_params);
  for(unsigned i=4;i<sizeof(t->op_params);i++)if(bytes[i])return false;
  return true;
 }
 static bool Weight(const PioneerQwen38Gguf& model,const ggml_tensor* w,unsigned layer,const char* role,
  unsigned k,unsigned n,unsigned& index) {
  if(!w || w->op!=GGML_OP_NONE || w->type!=GGML_TYPE_Q4_K || w->view_src || w->view_offs
   || !NoExtraSources(w,0) || w->ne[0]!=k || w->ne[1]!=n || w->ne[2]!=1 || w->ne[3]!=1
   || w->nb[0]!=144 || w->nb[1]!=size_t(k/256)*144 || w->nb[2]!=w->nb[1]*n || w->nb[3]!=w->nb[2]
   || !memchr(w->name,0,sizeof(w->name)))return false;
  char name[64];int length=snprintf(name,sizeof(name),"blk.%u.ffn_%s.weight",layer,role);
  if(length<=0 || unsigned(length)>=sizeof(name) || strcmp(w->name,name))return false;
  const auto* t=model.Find(name);
  if(!t || t->type!=12 || t->rank!=2 || t->inputs!=k || t->outputs!=n)return false;
  size_t at=size_t(t-model.Tensors().data());if(at>=851)return false;
  index=unsigned(at);return true;
 }
 static bool Matrix(const ggml_tensor* t,unsigned k,unsigned n) {
  return t && t->op==GGML_OP_MUL_MAT && FloatVector(t,n) && FloatVector(t->src[1],k)
   && t->src[0] && !t->view_src && !t->view_offs && NoExtraSources(t,2) && Parameters(t);
 }
 static bool Describe(const PioneerQwen38Gguf& model,unsigned layer,ggml_cgraph* graph,int first,
  PioneerQ38GgmlFfnContract& output) {
  if(layer>=64 || !model.Stable() || !model.Epoch() || !graph || !graph->nodes || graph->n_nodes<4
   || first<0 || first>graph->n_nodes-4 || !graph->use_counts || !graph->visited_hash_set.size
   || !graph->visited_hash_set.keys || !graph->visited_hash_set.used)return false;
  const uint64_t epoch=model.Epoch();
  auto* a=graph->nodes[first];auto* b=graph->nodes[first+1];
  auto* glu=graph->nodes[first+2];auto* down=graph->nodes[first+3];
  if(!Matrix(a,5120,17408) || !Matrix(b,5120,17408) || !Matrix(down,17408,5120)
   || a==b || a->src[1]!=b->src[1] || !FloatVector(glu,17408) || glu->op!=GGML_OP_GLU
   || !Parameters(glu,unsigned(GGML_GLU_OP_SWIGLU)) || !NoExtraSources(glu,2)
   || glu->view_src || glu->view_offs || down->src[1]!=glu)return false;
  const ggml_tensor* gate=nullptr;const ggml_tensor* up=nullptr;
  if(glu->src[0]==a && glu->src[1]==b){gate=a;up=b;}
  else if(glu->src[0]==b && glu->src[1]==a){gate=b;up=a;}else return false;
  for(unsigned i=0;i<4;i++) {
   auto* node=graph->nodes[first+i];
   if(!(node->flags&GGML_TENSOR_FLAG_COMPUTE) || !ggml_hash_contains(&graph->visited_hash_set,node))return false;
   // Includes explicit OUTPUT and view-source refusal,not just a count.
   if(i<3 && !ggml_node_has_n_uses(graph,first+int(i),1))return false;
  }
  PioneerQ38GgmlFfnContract stage={};stage.source=&model;stage.epoch=epoch;stage.layer=layer;stage.first=first;
  stage.input=gate->src[1];stage.gate=gate;stage.up=up;stage.glu=glu;stage.down=down;
  if(!Weight(model,gate->src[0],layer,"gate",5120,17408,stage.weight[0])
   || !Weight(model,up->src[0],layer,"up",5120,17408,stage.weight[1])
   || !Weight(model,down->src[0],layer,"down",17408,5120,stage.weight[2])
   || model.Epoch()!=epoch || !model.Stable())return false;
  output=stage;return true;
 }
};
#endif
