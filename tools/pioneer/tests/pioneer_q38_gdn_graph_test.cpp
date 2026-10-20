/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38GgmlGdnContract.h"
#include "../PioneerQ38GdnState.h"
#include <cassert>
#include <cstdio>
int main()
{
 ggml_init_params p={2*1024*1024,nullptr,true}; auto* ctx=ggml_init(p); assert(ctx);
 auto* q=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
 auto* k=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
 auto* qkv=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,10240);
 auto* v=ggml_view_4d(ctx,qkv,128,48,1,1,512,40960,40960,16384);
 auto* g=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
 auto* beta=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
 auto* pool=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,2*128*128*48);
 auto* state=ggml_view_4d(ctx,pool,128,128,48,1,512,65536,3145728,3145728);
 auto* op=ggml_gated_delta_net(ctx,q,k,v,g,beta,state,1);
 auto* attention=ggml_view_4d(ctx,op,128,48,1,1,512,24576,24576,0);
 auto* next=ggml_view_4d(ctx,op,128,128,48,1,512,65536,3145728,24576);
 auto* copy=ggml_cpy(ctx,next,state);
 PioneerQ38GgmlGdnContract c;
 assert(PioneerQ38GgmlGdnContract::Describe(op,c) && c.AttentionView(attention) && c.StateView(next) && c.StateCopy(copy));
 assert(!c.StateView(attention) && !c.AttentionView(next) && !q->data && !state->data && !op->data);
 auto unchanged=c;
 for (unsigned src=0;src<6;src++) {
  auto* t=op->src[src];
  auto oldType=t->type; t->type=GGML_TYPE_F16; assert(!PioneerQ38GgmlGdnContract::Describe(op,c) && c.node==unchanged.node); t->type=oldType;
  for (unsigned dim=0;dim<4;dim++) {
   auto ne=t->ne[dim]; t->ne[dim]++; assert(!PioneerQ38GgmlGdnContract::Describe(op,c)); t->ne[dim]=ne;
   auto nb=t->nb[dim]; t->nb[dim]++; assert(!PioneerQ38GgmlGdnContract::Describe(op,c)); t->nb[dim]=nb;
  }
  op->src[src]=nullptr; assert(!PioneerQ38GgmlGdnContract::Describe(op,c)); op->src[src]=t;
 }
 for (unsigned byte=0;byte<sizeof(op->op_params);byte++) {
  auto* b=reinterpret_cast<unsigned char*>(op->op_params); b[byte]^=1;
  assert(!PioneerQ38GgmlGdnContract::Describe(op,c)); b[byte]^=1;
 }
 auto previous=next->view_offs; next->view_offs+=4; assert(!c.StateView(next) && !c.StateCopy(copy)); next->view_offs=previous;
 previous=copy->view_offs; copy->view_offs+=4; assert(!c.StateCopy(copy)); copy->view_offs=previous;
 auto* root=copy->view_src; copy->view_src=qkv; assert(!c.StateCopy(copy)); copy->view_src=root;
 auto ne=pool->ne[0]; pool->ne[0]=INT64_MAX; assert(!c.StateCopy(copy)); pool->ne[0]=ne;
 previous=state->view_offs; state->view_offs=SIZE_MAX-3; copy->view_offs=state->view_offs; assert(!c.StateCopy(copy)); state->view_offs=previous; copy->view_offs=previous;
 assert(c.StateCopy(copy));
 // Larger prompt chunks,snapshots and multiple sequences are deliberately
 // not inferred from the valid one-token/one-sequence contract.
 for (unsigned kind=0;kind<3;kind++) {
  ggml_tensor changed=*op;
  if (!kind) changed.op=GGML_OP_MUL_MAT;
  if (kind==1) changed.ne[1]++;
  if (kind==2) changed.src[6]=q;
  assert(!PioneerQ38GgmlGdnContract::Describe(&changed,c));
 }
 ggml_free(ctx);
 puts("PASS REAL pinned GGML graph:GDN128/16/48 single-token shape,packed-QKV V view,output/state split,flattened nonzero-offset state CPY,ALL source dimensions/strides/types/op-param bytes and alias-bound faults;NO tensor data access/backend claim/native transport");
}
