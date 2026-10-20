/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GGML_GDN_CONTRACT_H
#define PIONEER_Q38_GGML_GDN_CONTRACT_H
#include "ggml.h"
#include <string.h>
// Shape/view contract ONLY. Not registered as backend support:the existing
// matrix backend must NOT claim GDN until native resident ownership AND
// device-buffer/view/CPY routing are implemented. No tensor data reads.
struct PioneerQ38GgmlGdnContract {
 enum { Keys=128,KeyHeads=16,ValueHeads=48,OutputWords=128*48,StateWords=128*128*48,
        OutputBytes=OutputWords*4,StateBytes=StateWords*4 };
 const ggml_tensor* node=nullptr;
 static bool Shape(const ggml_tensor* t,int64_t a,int64_t b,int64_t c,int64_t d) {
  return t && t->type==GGML_TYPE_F32 && t->ne[0]==a && t->ne[1]==b && t->ne[2]==c && t->ne[3]==d;
 }
 static bool Rows(const ggml_tensor* t,unsigned heads,unsigned rowPlane) {
  // Q/K contiguous results,or V's recorded view into10240-channel QKV.
  // Higher planes have one element but their exact recorded strides are
  // still validated so a future larger-batch extension cannot slip through.
  return Shape(t,128,heads,1,1) && t->nb[0]==4 && t->nb[1]==512
   && (t->nb[2]==rowPlane || t->nb[2]==heads*512) && t->nb[3]==t->nb[2];
 }
 static bool Dense(const ggml_tensor* t,int64_t a,int64_t b,int64_t c,int64_t d) {
  return Shape(t,a,b,c,d) && t->nb[0]==4 && t->nb[1]==size_t(a)*4
   && t->nb[2]==size_t(a*b)*4 && t->nb[3]==size_t(a*b*c)*4;
 }
 static bool Describe(const ggml_tensor* op,PioneerQ38GgmlGdnContract& output) {
  if (!op || op->op!=GGML_OP_GATED_DELTA_NET || !Dense(op,OutputWords,129,1,1)
   || !Rows(op->src[0],16,8192) || !Rows(op->src[1],16,8192) || !Rows(op->src[2],48,40960)
   || !Dense(op->src[3],1,48,1,1) || !Dense(op->src[4],1,48,1,1) || !Dense(op->src[5],128,128,48,1)) return false;
  int32_t k=0; memcpy(&k,op->op_params,4); if (k!=1) return false;
  const auto* bytes=reinterpret_cast<const unsigned char*>(op->op_params);
  for (unsigned i=4;i<sizeof(op->op_params);i++) if (bytes[i]) return false;
  for (unsigned i=6;i<GGML_MAX_SRC;i++) if (op->src[i]) return false;
  output.node=op; return true;
 }
 bool AttentionView(const ggml_tensor* view) const {
  return node && view && view->op==GGML_OP_VIEW && view->view_src==node && view->src[0]==node && !view->view_offs
   && Dense(view,128,48,1,1);
 }
 bool StateView(const ggml_tensor* view) const {
  return node && view && view->op==GGML_OP_VIEW && view->view_src==node && view->src[0]==node
   && view->view_offs==OutputBytes && Dense(view,128,128,48,1);
 }
 bool StateCopy(const ggml_tensor* copy) const {
  // Identity/offset matching alone DOES NOT authorize a host memcpy.
  // The eventual backend must bind BOTH ends to its owned device buffers.
  if (!copy || copy->op!=GGML_OP_CPY || !StateView(copy->src[0]) || !Dense(copy->src[1],128,128,48,1)
   || !Dense(copy,128,128,48,1)) return false;
  const auto* destination=copy->src[1];
  const auto* root=destination->view_src?destination->view_src:destination;
  size_t offset=destination->view_src?destination->view_offs:0;
  if (!root || root->type!=GGML_TYPE_F32 || root->view_src || root->nb[0]!=4
   || copy->view_src!=root || copy->view_offs!=offset || offset%4) return false;
  size_t bytes=4;
  for (unsigned i=0;i<4;i++) {
   if (root->ne[i]<=0 || uint64_t(root->ne[i])>SIZE_MAX/bytes || root->nb[i]!=bytes) return false;
   bytes*=size_t(root->ne[i]);
  }
  return offset<=bytes && StateBytes<=bytes-offset;
 }
};
#endif
