/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlHostDevice.h"
#include "../PioneerGgmlBackend.h"
#include "ggml.h"
PioneerGgmlHostState& PioneerHostDeviceState() { static PioneerGgmlHostState state; return state; }
static float Bf16(float f) { uint32_t bits=uint32_t(PioneerResidentBf16(f))<<16; memcpy(&f,&bits,4); return f; }
int main() {
	ggml_backend_register(PioneerGgmlReg());
	auto* dev=ggml_backend_reg_dev_get(PioneerGgmlReg(),0); assert(dev);
	assert(ggml_backend_dev_type(dev)==GGML_BACKEND_DEVICE_TYPE_ACCEL);
	auto* backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	assert(!ggml_backend_dev_init(dev,NULL)); // one exclusive session instance
	ggml_init_params params={2*1024*1024,NULL,false}; auto* ctx=ggml_init(params); assert(ctx);
	auto* weights=ggml_new_tensor_2d(ctx,GGML_TYPE_Q8_0,1024,32);
	auto* input=ggml_new_tensor_2d(ctx,GGML_TYPE_F32,1024,1);
	auto* result=ggml_mul_mat(ctx,weights,input);
	auto* buffer=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(weights)); assert(buffer);
	weights->data=ggml_backend_buffer_get_base(buffer);
	weights->buffer=buffer; ggml_backend_buffer_set_usage(buffer,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
	auto* inputBuffer=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(input)); assert(inputBuffer);
	input->buffer=inputBuffer; input->data=ggml_backend_buffer_get_base(inputBuffer);
	auto* resultBuffer=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(result)); assert(resultBuffer);
	result->buffer=resultBuffer; result->data=ggml_backend_buffer_get_base(resultBuffer);
	assert(ggml_backend_dev_supports_op(dev,result));
	ggml_backend_buffer_set_usage(buffer,GGML_BACKEND_BUFFER_USAGE_COMPUTE);
	assert(!ggml_backend_dev_supports_op(dev,result));
	ggml_backend_buffer_set_usage(buffer,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
	weights->ne[2]=2; assert(!ggml_backend_dev_supports_op(dev,result)); weights->ne[2]=1;
	input->ne[1]=2; assert(!ggml_backend_dev_supports_op(dev,result)); input->ne[1]=1;
	weights->type=GGML_TYPE_F32; assert(!ggml_backend_dev_supports_op(dev,result)); weights->type=GGML_TYPE_Q8_0;
	std::vector<float> original(32*1024),decoded(original.size());
	for (unsigned i=0;i<original.size();i++) original[i]=float(int((i*13)%257)-128)/1024;
	const auto* traits=ggml_get_type_traits(GGML_TYPE_Q8_0);
	traits->from_float_ref(original.data(),weights->data,int64_t(original.size()));
	traits->to_float(weights->data,decoded.data(),int64_t(decoded.size()));
	auto* graph=ggml_new_graph(ctx); ggml_build_forward_expand(graph,result); result->flags|=GGML_TENSOR_FLAG_COMPUTE;
	for (unsigned trial=0;trial<2;trial++) {
		for (unsigned k=0;k<1024;k++) ((float*)input->data)[k]=float(int((k*17+trial*29)%257)-128)/256;
		assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS);
		for (unsigned row=0;row<32;row++) {
			float reference=0;
			for (unsigned k=0;k<1024;k++) reference+=Bf16(decoded[row*1024+k])*Bf16(((float*)input->data)[k]);
			assert(((float*)result->data)[row]==reference);
		}
	}
	PioneerGgmlStats stats={}; assert(PioneerGgmlSnapshot(stats) && stats.tensors==1 && stats.uploads==18 && stats.matvecs==2 && !stats.failed);
	auto& io=PioneerHostDeviceState(); assert(io.opens==1 && io.uploads==18 && io.calls==2);
	io.fail=true;
	for (unsigned i=0;i<32;i++) ((float*)result->data)[i]=NAN;
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_FAILED);
	for (unsigned i=0;i<32;i++) assert(isnan(((float*)result->data)[i])); // no CPU fallback
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_FAILED && io.calls==3);
	assert(PioneerGgmlFinish() && io.finishes==1);
	ggml_backend_free(backend); assert(io.finishes==1 && !PioneerGgmlSnapshot(stats));
	// Independent primitive diagnostics catch a numerically wrong device
	// result even when transport completion says success; never substitute.
	io=PioneerGgmlHostState(); PioneerGgmlDiagnose(true); backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS);
	assert(PioneerGgmlReferenceTap(result,true,NULL) && PioneerGgmlReferenceTap(result,false,NULL) && PioneerGgmlReferenceCount()==1);
	ggml_tensor metadataOnly=*result; metadataOnly.op=GGML_OP_RESHAPE;
	assert(!PioneerGgmlReferenceTap(&metadataOnly,true,NULL));
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS);
	float correct=((float*)result->data)[0]; io.badMath=true;
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_FAILED && ((float*)result->data)[0]==correct+1);
	assert(PioneerGgmlSnapshot(stats) && stats.failed && stats.matvecs==3);
	assert(PioneerGgmlFinish()); ggml_backend_free(backend); assert(io.finishes==1);
	io=PioneerGgmlHostState(); PioneerGgmlDiagnose(false); PioneerGgmlCpuActivationQuantization(true);
	backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS);
	std::vector<uint8_t> qx(1024/32*34); std::vector<float> dx(1024);
	traits->from_float_ref((const float*)input->data,qx.data(),1024); traits->to_float(qx.data(),dx.data(),1024);
	for (unsigned row=0;row<32;row++) {
		float reference=0; for (unsigned k=0;k<1024;k++) reference+=Bf16(decoded[row*1024+k])*Bf16(dx[k]);
		assert(((float*)result->data)[row]==reference);
	}
	assert(PioneerGgmlFinish()); ggml_backend_free(backend);
	io=PioneerGgmlHostState(); PioneerGgmlDiagnose(true); PioneerGgmlFp16Operands(true);
	backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	PioneerGgmlFp16Operands(false); // format cannot change during a live context
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS && io.fp16);
	for (unsigned row=0;row<32;row++) {
		float reference=0;
		for (unsigned k=0;k<1024;k++) reference+=PioneerFloat16Value(PioneerFloat16(decoded[row*1024+k])&~1U)*PioneerFloat16Value(PioneerFloat16(dx[k]));
		assert(((float*)result->data)[row]==reference);
	}
	assert(PioneerGgmlSnapshot(stats) && stats.tensors==1 && stats.uploads==18 && stats.matvecs==1);
	assert(PioneerGgmlFinish()); ggml_backend_free(backend); PioneerGgmlFp16Operands(false);
	for (unsigned format=0;format<4;format++) {
	bool compact=format!=0,native=format>=2,exact=format==3;
	io=PioneerGgmlHostState(); PioneerGgmlDiagnose(true); PioneerGgmlQ8Operands(true); PioneerGgmlQ8Compact(compact); PioneerGgmlQ8Native(native);
	PioneerGgmlQ8Exact(exact);
	backend=ggml_backend_dev_init(dev,NULL); assert(backend); PioneerGgmlQ8Operands(false); // immutable while live
	PioneerGgmlQ8Compact(!compact); // also immutable while live
	PioneerGgmlQ8Native(!native);
	PioneerGgmlQ8Exact(!exact);
	for (unsigned trial=0;trial<2;trial++) {
		for (unsigned k=0;k<1024;k++) ((float*)input->data)[k]=float(int((k*17+trial*29)%257)-128)/257;
		traits->from_float_ref((const float*)input->data,qx.data(),1024);
		assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS && io.q8 && !io.fp16 && io.compact==compact && io.native==native && io.exact==exact);
		for (unsigned row=0;row<32;row++) {
			float reference=0;
			for (unsigned block=0;block<32;block++) {
				const uint8_t* w=(const uint8_t*)weights->data+(row*32+block)*34;
				const uint8_t* x=qx.data()+block*34; int dot=0;
				for (unsigned i=0;i<32;i++) { int a=w[2+i],b=x[2+i]; if (a>=128) a-=256; if (b>=128) b-=256; dot+=a*b; }
				float dw=PioneerHalfFloat(uint16_t(w[0])|(uint16_t(w[1])<<8)),dx=PioneerHalfFloat(uint16_t(x[0])|(uint16_t(x[1])<<8));
				if (exact) reference=fmaf(float(dot),dw*dx,reference); else reference+=float(dot)*(dw*dx);
			}
			assert(((float*)result->data)[row]==reference);
		}
	}
	assert(PioneerGgmlSnapshot(stats) && !stats.failed && stats.tensors==1 && stats.uploads==(compact?9U:18U) && stats.matvecs==2 && io.calls==2);
	io.badMath=true; assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_FAILED);
	assert(PioneerGgmlFinish()); ggml_backend_free(backend); PioneerGgmlQ8Operands(false); PioneerGgmlQ8Compact(false); PioneerGgmlQ8Native(false); PioneerGgmlQ8Exact(false);
	}
	io=PioneerGgmlHostState(); PioneerGgmlDiagnose(true); PioneerGgmlQ8Operands(true); PioneerGgmlQ8Compact(true); PioneerGgmlQ8Native(true);
	backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS);
	assert(PioneerGgmlReferenceTap(result,false,NULL) && PioneerGgmlReferenceCount()==1);
	assert(PioneerGgmlFinish()); ggml_backend_free(backend);
	io=PioneerGgmlHostState(); assert(!PioneerGgmlReplayReference("missing") && !PioneerGgmlReplayCpuRope("missing") && !io.opens);
	assert(PioneerGgmlReplayReference(weights->name) && io.opens==1 && io.calls==1 && io.uploads==9 && io.finishes==1);
	io=PioneerGgmlHostState(); PioneerGgmlQ8Exact(true);
	assert(PioneerGgmlReplayReference(weights->name) && io.exact && io.opens==1 && io.calls==1 && io.uploads==9 && io.finishes==1);
	PioneerGgmlQ8Exact(false);
	io=PioneerGgmlHostState(); io.badMath=true;
	assert(!PioneerGgmlReplayReference(weights->name) && io.finishes==1);
	PioneerGgmlQ8Operands(false); PioneerGgmlQ8Compact(false); PioneerGgmlQ8Native(false);
	// Trace observers are separate from learned-matrix qualification. They
	// detect missing references and leave every actual output byte intact.
	PioneerGgmlTrace(true); PioneerGgmlBeginPosition(17);
	result->op=GGML_OP_RMS_NORM; ggml_set_name(result,"attn_norm-0");
	assert(PioneerGgmlReferenceTap(result,true,NULL) && PioneerGgmlReferenceTap(result,false,NULL));
	assert(PioneerGgmlReferenceCount()==0 && PioneerGgmlCompareTap(result,true,NULL));
	assert(PioneerGgmlCompareTap(result,false,NULL));
	float changed=((float*)result->data)[0]+.125f; ((float*)result->data)[0]=changed;
	assert(PioneerGgmlCompareTap(result,false,NULL) && ((float*)result->data)[0]==changed);
	result->op=GGML_OP_MUL; assert(!PioneerGgmlCompareTap(result,false,NULL)); result->op=GGML_OP_RMS_NORM;
	PioneerGgmlBeginPosition(18); assert(!PioneerGgmlCompareTap(result,false,NULL));
	PioneerGgmlTrace(false); assert(!PioneerGgmlCompareTap(result,true,NULL));
	// New two-core mode is explicit/immutable and requires exact native Q8.
	for (unsigned reuseMode=0;reuseMode<2;reuseMode++) {
	PioneerGgmlQ8DualReuse(reuseMode!=0);
	if (reuseMode) assert(!ggml_backend_dev_init(dev,NULL)); // no reuse without two-core/exact flags
	PioneerGgmlQ8Dual(true); assert(!ggml_backend_dev_init(dev,NULL));
	PioneerGgmlQ8Operands(true); PioneerGgmlQ8Compact(true); PioneerGgmlQ8Native(true); PioneerGgmlQ8Exact(true);
	PioneerGgmlDiagnose(false); io=PioneerGgmlHostState();
	backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	result->op=GGML_OP_MUL_MAT; assert(!ggml_backend_dev_supports_op(dev,result)); // 32 rows cannot split
	auto* dualWeights=ggml_new_tensor_2d(ctx,GGML_TYPE_Q8_0,1024,64);
	auto* dualResult=ggml_mul_mat(ctx,dualWeights,input);
	auto* dualWeightBuffer=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(dualWeights)); assert(dualWeightBuffer);
	dualWeights->buffer=dualWeightBuffer; dualWeights->data=ggml_backend_buffer_get_base(dualWeightBuffer);
	ggml_backend_buffer_set_usage(dualWeightBuffer,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
	auto* dualResultBuffer=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(dualResult)); assert(dualResultBuffer);
	dualResult->buffer=dualResultBuffer; dualResult->data=ggml_backend_buffer_get_base(dualResultBuffer);
	for (unsigned row=0;row<64;row++) memcpy((uint8_t*)dualWeights->data+row*1088,(const uint8_t*)weights->data+(row%32)*1088,1088);
	std::vector<float> dualDecoded(64*1024); traits->to_float(dualWeights->data,dualDecoded.data(),dualDecoded.size());
	auto* dualGraph=ggml_new_graph(ctx); ggml_build_forward_expand(dualGraph,dualResult); dualResult->flags|=GGML_TENSOR_FLAG_COMPUTE;
	PioneerGgmlQ8Dual(false); // ignored during an active backend
	PioneerGgmlQ8DualReuse(reuseMode==0); // likewise immutable
	for (unsigned trial=0;trial<2;trial++) {
		for (unsigned k=0;k<1024;k++) ((float*)input->data)[k]=float(int((k*17+trial*29)%257)-128)/256;
		assert(ggml_backend_graph_compute(backend,dualGraph)==GGML_STATUS_SUCCESS && io.dual && io.exact && io.reuse==bool(reuseMode));
		uint8_t quantized[1088]; traits->from_float_ref((const float*)input->data,quantized,1024);
		float activation[1024]; traits->to_float(quantized,activation,1024);
		for (unsigned row=0;row<64;row++) {
			float reference=0;
			for (unsigned block=0;block<32;block++) {
				int dot=0; const auto* a=(const int8_t*)dualWeights->data+row*1088+block*34+2;
				const auto* b=(const int8_t*)quantized+block*34+2;
				for (unsigned i=0;i<32;i++) dot+=int(a[i])*int(b[i]);
				uint16_t dw,dx; memcpy(&dw,a-2,2); memcpy(&dx,b-2,2);
				reference=fmaf(float(dot),PioneerHalfFloat(dw)*PioneerHalfFloat(dx),reference);
			}
			assert(!memcmp((float*)dualResult->data+row,&reference,4));
		}
	}
	assert(io.opens==1 && io.uploads==18 && io.calls==2);
	io.fail=true; for (unsigned row=0;row<64;row++) ((float*)dualResult->data)[row]=NAN;
	assert(ggml_backend_graph_compute(backend,dualGraph)==GGML_STATUS_FAILED);
	for (unsigned row=0;row<64;row++) assert(isnan(((float*)dualResult->data)[row]));
	assert(PioneerGgmlFinish()); ggml_backend_free(backend);
	PioneerGgmlQ8DualReuse(false); PioneerGgmlQ8Dual(false); PioneerGgmlQ8Exact(false); PioneerGgmlQ8Native(false); PioneerGgmlQ8Compact(false); PioneerGgmlQ8Operands(false);
	dualWeights->buffer=NULL; dualResult->buffer=NULL; ggml_backend_buffer_free(dualWeightBuffer); ggml_backend_buffer_free(dualResultBuffer);
	}
	weights->buffer=NULL; input->buffer=NULL; result->buffer=NULL;
	ggml_backend_buffer_free(buffer); ggml_backend_buffer_free(inputBuffer); ggml_backend_buffer_free(resultBuffer); ggml_free(ctx);
	printf("PASS: pinned ggml registration, strict support policy, two vectors/one upload, graph failure/no fallback, exclusive cleanup; OFFLINE ONLY\n");
}
