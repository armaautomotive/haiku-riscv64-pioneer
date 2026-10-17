/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlHostDevice.h"
#include "../PioneerGgmlBackend.h"
#include "ggml-backend-impl.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "ggml-impl.h"
#pragma GCC diagnostic pop
PioneerGgmlHostState& PioneerHostDeviceState() { static PioneerGgmlHostState state; return state; }
static std::vector<float> Reference(const ggml_tensor* w,const float* input)
{
	unsigned k=unsigned(w->ne[0]),n=unsigned(w->ne[1]); std::vector<uint8_t> x(k/32*34); std::vector<float> result(n);
	ggml_get_type_traits(GGML_TYPE_Q8_0)->from_float_ref(input,x.data(),k);
	for (unsigned row=0;row<n;row++) {
		float sum=0;
		for (unsigned b=0;b<k/32;b++) {
			const uint8_t* a=static_cast<const uint8_t*>(w->data)+row*w->nb[1]+b*34; const uint8_t* v=x.data()+b*34;
			int dot=0; for (unsigned i=0;i<32;i++) dot+=int(int8_t(a[i+2]))*int(int8_t(v[i+2]));
			uint16_t wa,va; memcpy(&wa,a,2); memcpy(&va,v,2);
			sum=fmaf(float(dot),PioneerHalfFloat(wa)*PioneerHalfFloat(va),sum);
		}
		result[row]=sum;
	}
	return result;
}
int main()
{
	ggml_backend_register(PioneerGgmlReg()); auto* dev=ggml_backend_reg_dev_get(PioneerGgmlReg(),0);
	PioneerGgmlLayerFfn(true); assert(!ggml_backend_dev_init(dev,NULL));
	PioneerGgmlQ8Operands(true); PioneerGgmlQ8Compact(true); PioneerGgmlQ8Native(true); PioneerGgmlQ8Exact(true);
	PioneerGgmlQ8Dual(true); PioneerGgmlQ8DualReuse(true); PioneerGgmlAttention(true); PioneerGgmlBulkUpload(1);
	PioneerGgmlFleet(119); PioneerGgmlResidentKV(true); PioneerGgmlPayloadBlocks(true); PioneerGgmlCoordinated(true);
	PioneerGgmlDiagnose(true); assert(!ggml_backend_dev_init(dev,NULL)); PioneerGgmlDiagnose(false);
	PioneerGgmlTrace(true); assert(!ggml_backend_dev_init(dev,NULL)); PioneerGgmlTrace(false);
	auto* backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	auto& io=PioneerHostDeviceState(); io.bytes.resize(32*1024*1024/4);
	ggml_init_params params={32*1024*1024,NULL,false}; auto* ctx=ggml_init(params); assert(ctx);
	auto* x=ggml_new_tensor_2d(ctx,GGML_TYPE_F32,1024,1);
	ggml_tensor* weights[3]; ggml_backend_buffer_t buffers[3];
	for (unsigned phase=0;phase<3;phase++) {
		unsigned k=phase==2?3072:1024,n=phase==2?1024:3072;
		weights[phase]=ggml_new_tensor_2d(ctx,GGML_TYPE_Q8_0,k,n);
		buffers[phase]=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(weights[phase])); assert(buffers[phase]);
		weights[phase]->buffer=buffers[phase]; weights[phase]->data=ggml_backend_buffer_get_base(buffers[phase]);
		ggml_backend_buffer_set_usage(buffers[phase],GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
		std::vector<float> source(k*n); for (unsigned i=0;i<source.size();i++) source[i]=(int((i*17+phase*13+i/k*3)%31)-15)*.0009765625f;
		ggml_get_type_traits(GGML_TYPE_Q8_0)->from_float_ref(source.data(),weights[phase]->data,source.size());
	}
	auto* gate=ggml_mul_mat(ctx,weights[0],x); auto* up=ggml_mul_mat(ctx,weights[1],x);
	auto* glu=ggml_swiglu_split(ctx,gate,up); auto* down=ggml_mul_mat(ctx,weights[2],glu);
	for (auto* t:{gate,up,glu,down}) t->flags|=GGML_TENSOR_FLAG_COMPUTE;
	assert(ggml_backend_dev_supports_op(dev,glu));
	glu->op_params[1]=1; assert(!ggml_backend_dev_supports_op(dev,glu)); glu->op_params[1]=0;
	glu->op_params[0]=GGML_GLU_OP_SWIGLU_OAI; assert(!ggml_backend_dev_supports_op(dev,glu)); glu->op_params[0]=GGML_GLU_OP_SWIGLU;
	auto* graph=ggml_new_graph(ctx); ggml_build_forward_expand(graph,down); assert(ggml_graph_n_nodes(graph)==4);
	for (unsigned trial=0;trial<2;trial++) {
		for (unsigned i=0;i<1024;i++) static_cast<float*>(x->data)[i]=float(sin(i*.137+trial)*.5);
		for (auto* t:{gate,up,glu,down}) for (int64_t i=0;i<ggml_nelements(t);i++) static_cast<float*>(t->data)[i]=NAN;
		auto a=Reference(weights[0],static_cast<float*>(x->data)),b=Reference(weights[1],static_cast<float*>(x->data));
		std::vector<float> nonlinear(3072); for (unsigned i=0;i<3072;i++) nonlinear[i]=(a[i]/(1.0f+expf(-a[i])))*b[i];
		auto expected=Reference(weights[2],nonlinear.data());
		assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS && io.ffnCalls==trial+1 && !io.gluCalls);
		double difference=0,norm=0;
		for (unsigned i=0;i<1024;i++) { float actual=static_cast<float*>(down->data)[i]; assert(isfinite(actual)); double d=double(actual)-expected[i]; difference+=d*d; norm+=double(expected[i])*expected[i]; }
		assert(sqrt(difference/(norm+1e-30))<.00002);
		for (auto* t:{gate,up,glu}) for (unsigned i=0;i<3072;i++) assert(isnan(static_cast<float*>(t->data)[i]));
	}
	PioneerGgmlStats stats={}; assert(PioneerGgmlSnapshot(stats) && stats.ffnOps==2 && !stats.gluOps && stats.matvecs==6 && stats.tensors==3);
	// A later CPU consumer lies OUTSIDE the scheduler slice. Global counts
	// forbid elision,so normal card matrices+card GLU must materialize gate.
	auto* consumer=ggml_add(ctx,gate,gate); ggml_build_forward_expand(graph,consumer);
	auto slice=ggml_graph_view(graph,0,4);
	assert(ggml_backend_graph_compute(backend,&slice)==GGML_STATUS_SUCCESS && io.ffnCalls==2 && io.gluCalls==1);
	for (unsigned i=0;i<3072;i++) assert(isfinite(static_cast<float*>(gate->data)[i]));
	// Explicit output requests also forbid fusion,even in a fresh graph.
	auto* fresh=ggml_new_graph(ctx); ggml_build_forward_expand(fresh,down); gate->flags|=GGML_TENSOR_FLAG_OUTPUT;
	assert(ggml_backend_graph_compute(backend,fresh)==GGML_STATUS_SUCCESS && io.ffnCalls==2 && io.gluCalls==2);
	gate->flags &= ~GGML_TENSOR_FLAG_OUTPUT;
	io.fail=true; unsigned gluCalls=io.gluCalls;
	for (unsigned i=0;i<1024;i++) static_cast<float*>(down->data)[i]=NAN;
	assert(ggml_backend_graph_compute(backend,fresh)==GGML_STATUS_FAILED && io.ffnCalls==3 && io.gluCalls==gluCalls);
	for (unsigned i=0;i<1024;i++) assert(isnan(static_cast<float*>(down->data)[i]));
	unsigned calls=io.calls; assert(ggml_backend_graph_compute(backend,fresh)==GGML_STATUS_FAILED && io.calls==calls);
	assert(PioneerGgmlSnapshot(stats) && stats.failed && PioneerGgmlFinish()); ggml_backend_free(backend);
	for (unsigned i=0;i<3;i++) { weights[i]->buffer=NULL; ggml_backend_buffer_free(buffers[i]); }
	ggml_free(ctx);
	puts("PASS OFFLINE pinned GGML FFN fusion,independent reference,global outside-slice consumers/outputs preserved,card GLU split path,unchanged warm weights,no CPU fallback or output publication on failure");
}
