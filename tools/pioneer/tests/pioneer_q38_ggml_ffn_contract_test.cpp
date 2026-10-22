/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38GgmlFfnContract.h"
#include "../PioneerQ38GgmlBackend.h"
#include <sys/mman.h>
using Contract=PioneerQ38GgmlFfnContract;
struct Graph {
 ggml_tensor *x,*weight[3],*gate,*up,*glu,*down;
 ggml_cgraph* graph;
 Graph(ggml_context* ctx,unsigned layer,bool reverse=false) {
  x=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,5120);
  for(unsigned p=0;p<3;p++) {
   weight[p]=ggml_new_tensor_2d(ctx,GGML_TYPE_Q4_K,p==2?17408:5120,p==2?5120:17408);
   ggml_format_name(weight[p],"blk.%u.ffn_%s.weight",layer,p==0?"gate":p==1?"up":"down");
  }
  gate=ggml_mul_mat(ctx,weight[0],x);up=ggml_mul_mat(ctx,weight[1],x);
  glu=ggml_swiglu_split(ctx,gate,up);down=ggml_mul_mat(ctx,weight[2],glu);
  for(auto* t:{gate,up,glu,down})t->flags|=GGML_TENSOR_FLAG_COMPUTE;
  graph=ggml_new_graph_custom(ctx,16,false);
  if(reverse)ggml_build_forward_expand(graph,up);
  ggml_build_forward_expand(graph,down);assert(ggml_graph_n_nodes(graph)==4);
 }
};
static bool Same(const Contract& a,const Contract& b) {
 return a.source==b.source && a.epoch==b.epoch && a.layer==b.layer && a.first==b.first
  && a.input==b.input && a.gate==b.gate && a.up==b.up && a.glu==b.glu && a.down==b.down
  && a.weight[0]==b.weight[0] && a.weight[1]==b.weight[1] && a.weight[2]==b.weight[2];
}
static bool Ready(void*){return true;}
static bool Multiply(void*,unsigned,const float*,unsigned,float*,unsigned){assert(false);return false;}
static bool Finish(void*){assert(false);return false;}
static void Abort(void*){assert(false);}
int main() {
 Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));off_t position=ftello(fixture.file);
 ggml_init_params params={8*1024*1024,nullptr,true};auto* ctx=ggml_init(params);assert(ctx);
 void* unreadable=mmap(nullptr,4096,PROT_NONE,MAP_PRIVATE|MAP_ANON,-1,0);assert(unreadable!=MAP_FAILED);
 PioneerQ38GgmlDispatch dispatch={&model,&model,Ready,Multiply,Finish,Abort};assert(PioneerQ38GgmlConfigure(dispatch));
 auto dev=ggml_backend_reg_dev_get(PioneerQ38GgmlReg(),0);assert(dev);
 unsigned positive=0,negative=0;
 for(unsigned layer=0;layer<64;layer++)for(bool reverse:{false,true}) {
  Graph g(ctx,layer,reverse);
  for(auto* t:{g.x,g.weight[0],g.weight[1],g.weight[2],g.gate,g.up,g.glu,g.down})t->data=unreadable;
  Contract c;assert(Contract::Describe(model,layer,g.graph,0,c));
  assert(c.source==&model && c.epoch==model.Epoch() && c.layer==layer && c.input==g.x
   && c.gate==g.gate && c.up==g.up && c.glu==g.glu && c.down==g.down);
  for(unsigned p=0;p<3;p++)assert(&model.Tensors()[c.weight[p]]==model.Find(g.weight[p]->name));
  assert(!ggml_backend_dev_supports_op(dev,g.glu)); // recognition MUST NOT advertise native support
  auto slice=ggml_graph_view(g.graph,0,4);Contract sliced;
  assert(Contract::Describe(model,layer,&slice,0,sliced) && Same(c,sliced));positive+=2;
 }
 // Only INTERMEDIATE values are elided. A final down output and later CPU
 // consumers of that output remain valid;their normal publication is owed.
 for(bool output:{false,true}) {
  Graph final(ctx,0);if(output)final.down->flags|=GGML_TENSOR_FLAG_OUTPUT;
  auto consumer=ggml_add(ctx,final.down,final.down);ggml_build_forward_expand(final.graph,consumer);
  for(auto* t:{final.x,final.weight[0],final.weight[1],final.weight[2],final.gate,final.up,final.glu,final.down,consumer})t->data=unreadable;
  auto slice=ggml_graph_view(final.graph,0,4);Contract c;
  assert(Contract::Describe(model,0,final.graph,0,c) && c.down==final.down);
  Contract sliced;assert(Contract::Describe(model,0,&slice,0,sliced) && Same(c,sliced));positive+=2;
 }
 Graph g(ctx,0);for(auto* t:{g.x,g.weight[0],g.weight[1],g.weight[2],g.gate,g.up,g.glu,g.down})t->data=unreadable;
 Contract c;assert(Contract::Describe(model,0,g.graph,0,c));
 auto refuse=[&](unsigned layer=0,ggml_cgraph* graph=nullptr,int first=0) {
  Contract out=c;assert(!Contract::Describe(model,layer,graph?graph:g.graph,first,out) && Same(out,c));negative++;
 };
 for(int first:{-1,1,INT32_MAX})refuse(0,g.graph,first);refuse(64);
 {Contract out=c;assert(!Contract::Describe(model,0,nullptr,0,out) && Same(out,c));negative++;}
 for(auto* node:{g.gate,g.up,g.glu,g.down}) {
  auto saved=*node;
  node->flags &= ~GGML_TENSOR_FLAG_COMPUTE;refuse();*node=saved;
  node->type=GGML_TYPE_F16;refuse();*node=saved;
  node->ne[1]=2;refuse();*node=saved;
  node->nb[0]=8;refuse();*node=saved;
  node->view_src=g.x;refuse();*node=saved;
  node->view_offs=4;refuse();*node=saved;
  node->src[2]=g.x;refuse();*node=saved;
  node->op_params[3]=1;refuse();*node=saved;
 }
 for(auto* node:{g.gate,g.up,g.glu}) {
  auto flags=node->flags;node->flags|=GGML_TENSOR_FLAG_OUTPUT;refuse();node->flags=flags;
 }
 for(auto* w:{g.weight[0],g.weight[1],g.weight[2]}) {
  auto saved=*w;
  ggml_set_name(w,"unbound.weight");refuse();*w=saved;
  memset(w->name,'x',sizeof(w->name));refuse();*w=saved;
  w->type=GGML_TYPE_Q8_0;refuse();*w=saved;
  w->ne[0]++;refuse();*w=saved;
  w->nb[1]++;refuse();*w=saved;
  w->view_src=g.x;refuse();*w=saved;
  w->op=GGML_OP_VIEW;refuse();*w=saved;
  w->src[0]=g.x;refuse();*w=saved;
 }
 {auto saved=*g.glu;g.glu->op_params[0]=GGML_GLU_OP_SWIGLU_OAI;refuse();*g.glu=saved;
  g.glu->op_params[1]=1;refuse();*g.glu=saved;
  g.glu->src[0]=g.up;g.glu->src[1]=g.gate;refuse();*g.glu=saved;}
 {auto* old=g.up->src[1];g.up->src[1]=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,5120);refuse();g.up->src[1]=old;}
 {auto* old=g.down->src[1];g.down->src[1]=g.gate;refuse();g.down->src[1]=old;}
 // Outside-slice CPU consumers and explicit outputs must preserve ALL normal
 // intermediates. The slice retains GLOBAL counts and therefore refuses.
 for(auto* value:{g.gate,g.up,g.glu}) {
  Graph branch(ctx,0);auto* t=value==g.gate?branch.gate:value==g.up?branch.up:branch.glu;
  auto consumer=ggml_add(ctx,t,t);ggml_build_forward_expand(branch.graph,consumer);
  auto slice=ggml_graph_view(branch.graph,0,4);refuse(0,&slice);
 }
 for(unsigned field=0;field<5;field++) {
  auto copy=*g.graph;
  if(field==0)copy.use_counts=nullptr;if(field==1)copy.visited_hash_set.size=0;
  if(field==2)copy.visited_hash_set.keys=nullptr;if(field==3)copy.visited_hash_set.used=nullptr;
  if(field==4)copy.n_nodes=3;refuse(0,&copy);
 }
 // Source descriptors are immutable during a live graph. Re-open changes the
 // epoch:the OLD description is not reusable;fresh Describe records the new.
 assert(ftello(fixture.file)==position);
 assert(model.Open(fixture.file));Contract fresh;assert(Contract::Describe(model,0,g.graph,0,fresh));
 assert(fresh.epoch!=c.epoch);position=ftello(fixture.file);
 assert(!ftruncate(fileno(fixture.file),off_t(PioneerQwen38Gguf::FullBytes)-1));refuse();
 assert(ftello(fixture.file)==position);
 assert(!munmap(unreadable,4096));ggml_free(ctx);
 printf("PASS OFFLINE Q38 FFN graph contract positive%u negative%u;ALL64layers/twographorders/globaloutside-slice consumers/OUTPUT/shape/type/stride/name/params/sourceepoch atomic refusals;PROT_NONE tensorvalues untouched/normalbackendGLU unsupported;NO_BUFFER_OWNER_DEVICE_EXECUTION_OR_SPEED_AUTHORITY\n",positive,negative);
}
