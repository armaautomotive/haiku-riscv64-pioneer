/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38GgmlBackend.h"
#include "../PioneerQ38GgmlGdnContract.h"
#include "ggml-backend-impl.h"
struct Fake {
	PioneerQwen38Gguf model; bool valid=true,fail=false; unsigned calls=0,ends=0,aborts=0;
	static bool Ready(void* p) { return static_cast<Fake*>(p)->valid; }
	static bool Multiply(void* p,unsigned tensor,const float* input,unsigned k,float* out,unsigned n) {
		auto& f=*static_cast<Fake*>(p); f.calls++;
		assert(tensor<f.model.Tensors().size() && input && k==f.model.Tensors()[tensor].inputs && n==f.model.Tensors()[tensor].outputs);
		if (f.fail) return false;
		for (unsigned r=0;r<n;r++) out[r]=float(r+1); // SYNTHETIC marker,NOT arithmetic reference
		return true;
	}
	static bool Finish(void* p) { static_cast<Fake*>(p)->ends++; return true; }
	static void Abort(void* p) { static_cast<Fake*>(p)->aborts++; }
};
int main(int argc,char** argv)
{
	assert(argc==2); unsigned fault=unsigned(atoi(argv[1])); assert(fault<=4);
	Fixture fixture; Fake fake; assert(fake.model.Open(fixture.file));
	PioneerQ38GgmlDispatch d={&fake,&fake.model,Fake::Ready,Fake::Multiply,Fake::Finish,Fake::Abort};
	auto reg=PioneerQ38GgmlReg(); auto dev=ggml_backend_reg_dev_get(reg,0); assert(dev);
	assert(!ggml_backend_dev_init(dev,nullptr)); // configure/authentication first
	assert(PioneerQ38GgmlConfigure(d) && !PioneerQ38GgmlConfigure(d));
	auto backend=ggml_backend_dev_init(dev,nullptr); assert(backend && !ggml_backend_dev_init(dev,nullptr));
	ggml_init_params params={1024*1024,nullptr,true}; auto ctx=ggml_init(params); assert(ctx);
	// A correct GDN shape is NOT native resident capability. Until owned
	// device storage/transport/CPY routing exists, the matrix-only backend
	// must refuse the computation/state copy. Existing host-only views are
	// metadata no-ops, NOT device buffer residency or execution support.
	auto q=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
	auto k=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
	auto v=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,48,1,1);
	auto g=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
	auto beta=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
	auto state=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,128,48,1);
	auto gdn=ggml_gated_delta_net(ctx,q,k,v,g,beta,state,1);
	auto attention=ggml_view_4d(ctx,gdn,128,48,1,1,512,24576,24576,0);
	auto next=ggml_view_4d(ctx,gdn,128,128,48,1,512,65536,3145728,24576);
	auto copy=ggml_cpy(ctx,next,state);
	PioneerQ38GgmlGdnContract contract;
	assert(PioneerQ38GgmlGdnContract::Describe(gdn,contract) && contract.AttentionView(attention)
		&& contract.StateView(next) && contract.StateCopy(copy));
	for (auto* node:{gdn,copy}) assert(!ggml_backend_dev_supports_op(dev,node));
	for (auto* node:{attention,next}) assert(ggml_backend_dev_supports_op(dev,node));
	assert(ggml_backend_buft_is_host(ggml_backend_dev_buffer_type(dev)));
	assert(!fake.calls);
	for (const char* name:{"blk.0.ssm_alpha.weight","blk.0.ffn_down.weight","blk.3.attn_output.weight"}) {
		const auto* t=fake.model.Find(name); assert(t);
		unsigned outputs=unsigned(t->outputs);
		auto w=ggml_new_tensor_2d(ctx,ggml_type(t->type),int64_t(t->inputs),int64_t(t->outputs));
		ggml_set_name(w,name);
		auto x=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,int64_t(t->inputs)); auto y=ggml_mul_mat(ctx,w,x);
		auto wb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(w)); assert(wb);
		w->buffer=wb; w->data=ggml_backend_buffer_get_base(wb); ggml_backend_buffer_set_usage(wb,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
		auto xb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(x)); assert(xb);
		x->buffer=xb; x->data=ggml_backend_buffer_get_base(xb); memset(x->data,0,ggml_nbytes(x));
		auto yb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(y)); assert(yb);
		y->buffer=yb; y->data=ggml_backend_buffer_get_base(yb);
		for (unsigned r=0;r<t->outputs;r++) static_cast<float*>(y->data)[r]=-99;
		assert(ggml_backend_dev_supports_op(dev,y));
		ggml_set_name(w,"unbound.weight"); assert(!ggml_backend_dev_supports_op(dev,y)); ggml_set_name(w,name);
		w->type=GGML_TYPE_F32; assert(!ggml_backend_dev_supports_op(dev,y)); w->type=ggml_type(t->type);
		w->ne[2]=2; assert(!ggml_backend_dev_supports_op(dev,y)); w->ne[2]=1;
		x->ne[1]=2; assert(!ggml_backend_dev_supports_op(dev,y)); x->ne[1]=1;
		x->nb[0]=8; assert(!ggml_backend_dev_supports_op(dev,y)); x->nb[0]=4;
		ggml_backend_buffer_set_usage(wb,GGML_BACKEND_BUFFER_USAGE_COMPUTE);
		assert(!ggml_backend_dev_supports_op(dev,y)); ggml_backend_buffer_set_usage(wb,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
		auto graph=ggml_new_graph(ctx); ggml_build_forward_expand(graph,y); y->flags|=GGML_TENSOR_FLAG_COMPUTE;
		if (fault==1) fake.fail=true;
		if (fault==2) fake.valid=false;
		if (fault==3) ggml_set_name(w,"unknown.weight");
		if (fault==4) assert(fake.model.Open(fixture.file));
		bool success=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		assert(success==!fault);
		for (unsigned r=0;r<outputs;r++) assert(static_cast<float*>(y->data)[r]==(fault?-99:float(r+1)));
		ggml_backend_buffer_free(yb); ggml_backend_buffer_free(xb); ggml_backend_buffer_free(wb);
		if (fault) break;
	}
	unsigned calls=fake.calls;
	assert(PioneerQ38GgmlSnapshot().recognizedWeights==(fault?1U:3U));
	assert(PioneerQ38GgmlFinish()==!fault);
	auto stats=PioneerQ38GgmlSnapshot(); assert(stats.finished && stats.failed==bool(fault) && stats.clean==!fault);
	assert(fake.ends==unsigned(!fault) && fake.aborts==unsigned(bool(fault)));
	assert(PioneerQ38GgmlFinish()==!fault && !ggml_backend_dev_init(dev,nullptr));
	ggml_backend_free(backend); assert(fake.calls==calls && fake.ends==unsigned(!fault) && fake.aborts==unsigned(bool(fault)));
	ggml_free(ctx);
	printf("PASS OFFLINE ggml27B bridge fault%u:exact named Q8/Q4/Q6 matrices,host weights/batch/stride contracts,GDN/state-CPY refused and views remain host-only metadata,exclusive one-shot context,source/epoch binding,atomic session failure,no CPU products/fallback,repeated Finish/free;SYNTHETIC DISPATCH,NOT native model inference\n",fault);
}
