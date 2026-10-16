/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_ATTENTION_H
#define PIONEER_GGML_ATTENTION_H
#include "ggml.h"
#include <stdio.h>
#include <string.h>
// Metadata-only eligibility. Mask contents/finite input/sequence are checked
// again at execution. Unsupported work must fail the caller's coverage gate.
static inline bool PioneerGgmlAttentionContract(const ggml_tensor* op,unsigned* layer=NULL)
{
	if (!op || op->op!=GGML_OP_FLASH_ATTN_EXT || op->type!=GGML_TYPE_F32 || !ggml_is_contiguous(op)
		|| !op->src[0] || !op->src[1] || !op->src[2] || !op->src[3] || op->src[4]) return false;
	const auto* q=op->src[0]; const auto* k=op->src[1]; const auto* v=op->src[2]; const auto* mask=op->src[3];
	const int64_t qShape[]={128,1,16,1},kvShape[]={128,256,8,1},outShape[]={128,16,1,1};
	const size_t qStride[]={4,8192,512,8192},kvStride[]={2,2048,256,524288};
	float p[3]; memcpy(p,op->op_params,sizeof(p));
	if (q->type!=GGML_TYPE_F32 || k->type!=GGML_TYPE_F16 || v->type!=GGML_TYPE_F16 || mask->type!=GGML_TYPE_F16
		|| memcmp(q->ne,qShape,sizeof(qShape)) || memcmp(k->ne,kvShape,sizeof(kvShape)) || memcmp(v->ne,kvShape,sizeof(kvShape))
		|| memcmp(op->ne,outShape,sizeof(outShape)) || memcmp(q->nb,qStride,sizeof(qStride))
		|| memcmp(k->nb,kvStride,sizeof(kvStride)) || memcmp(v->nb,kvStride,sizeof(kvStride))
		|| !ggml_is_contiguous(mask) || ggml_nelements(mask)!=256 || p[0]!=0.0883883461356163f || p[1]!=0 || p[2]!=0
		|| ggml_flash_attn_ext_get_prec(op)!=GGML_PREC_F32 || !q->view_src || !k->view_src || !v->view_src) return false;
	for (unsigned i=0;i<28;i++) {
		char name[32]; snprintf(name,sizeof(name),"Qcur-%u",i);
		if (strcmp(q->view_src->name,name)) continue;
		snprintf(name,sizeof(name),"cache_k_l%u",i); if (strcmp(k->view_src->name,name)) return false;
		snprintf(name,sizeof(name),"cache_v_l%u",i); if (strcmp(v->view_src->name,name)) return false;
		if (layer) *layer=i;
		return true;
	}
	return false;
}
#endif
