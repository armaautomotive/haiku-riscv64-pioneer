/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlHostDevice.h"
#include "../PioneerGgmlBackend.h"
#include "../PioneerAttentionFixture.h"
#include "ggml.h"
#include "ggml-backend-impl.h"
PioneerGgmlHostState& PioneerHostDeviceState() { static PioneerGgmlHostState state; return state; }
static float Bf16(float f) { uint32_t bits=uint32_t(PioneerResidentBf16(f))<<16; memcpy(&f,&bits,4); return f; }
int main(int argc,char** argv) {
	ggml_backend_register(PioneerGgmlReg());
	auto* dev=ggml_backend_reg_dev_get(PioneerGgmlReg(),0); assert(dev);
	assert(ggml_backend_dev_type(dev)==GGML_BACKEND_DEVICE_TYPE_ACCEL);
	auto* backend=ggml_backend_dev_init(dev,NULL); assert(backend);
	assert(!ggml_backend_dev_init(dev,NULL)); // one exclusive session instance
	PioneerGgmlContextExecution(false);
	auto* disabled=ggml_backend_dev_init(dev,NULL); assert(disabled);
	auto* disabledDevice=ggml_backend_get_device(disabled);
	assert(disabledDevice!=dev && ggml_backend_supports_buft(disabled,ggml_backend_cpu_buffer_type()));
	PioneerGgmlContextExecution(true);
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
	assert(!ggml_backend_dev_supports_op(disabledDevice,result));
	assert(ggml_backend_graph_compute(disabled,graph)==GGML_STATUS_FAILED);
	assert(PioneerHostDeviceState().opens==0 && PioneerHostDeviceState().calls==0);
	// Model the scheduler's required final CPU backend without compiling a
	// CPU arithmetic implementation: reserve/assignment only, no execution.
	ggml_backend_device cpuDevice=*disabledDevice;
	cpuDevice.iface.get_type=[](ggml_backend_dev_t) { return GGML_BACKEND_DEVICE_TYPE_CPU; };
	cpuDevice.iface.supports_op=[](ggml_backend_dev_t,const ggml_tensor*) { return true; };
	ggml_backend cpuBackend=*disabled; cpuBackend.device=&cpuDevice;
	ggml_backend_t schedBackends[]={disabled,&cpuBackend};
	auto* scheduler=ggml_backend_sched_new(schedBackends,NULL,2,2048,false,false); assert(scheduler);
	assert(ggml_backend_sched_reserve(scheduler,graph));
	assert(ggml_backend_sched_alloc_graph(scheduler,graph));
	assert(ggml_backend_sched_get_tensor_backend(scheduler,result)==&cpuBackend);
	ggml_backend_sched_free(scheduler);
	assert(PioneerHostDeviceState().opens==0 && PioneerHostDeviceState().calls==0);
	ggml_backend_free(disabled);
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
	PioneerGgmlContextExecution(false);
	disabled=ggml_backend_dev_init(dev,NULL); assert(disabled);
	ggml_backend_free(disabled);
	assert(PioneerGgmlSnapshot(stats) && stats.matvecs==2); // inert free never frees active card owner
	PioneerGgmlContextExecution(true);
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
	// Fixed fused-attention contract and 28-layer coverage. The transport is
	// explicitly software-only here; native shared handoff is tested separately.
	for (unsigned fleetMode=0;fleetMode<4;fleetMode++) {
		PioneerGgmlAttention(true); assert(!ggml_backend_dev_init(dev,NULL));
		PioneerGgmlQ8Operands(true); PioneerGgmlQ8Compact(true); PioneerGgmlQ8Native(true);
		PioneerGgmlQ8Exact(true); PioneerGgmlQ8Dual(true); PioneerGgmlQ8DualReuse(true);
		PioneerGgmlBulkUpload(fleetMode?1:0); PioneerGgmlFleet(fleetMode?120:0);
		PioneerGgmlResidentKV(fleetMode>=2); PioneerGgmlPayloadBlocks(fleetMode==3);
		io=PioneerGgmlHostState(); backend=ggml_backend_dev_init(dev,NULL); assert(backend);
		ggml_init_params ap={8*1024*1024,NULL,false}; auto* ac=ggml_init(ap); assert(ac);
		auto* qr=ggml_new_tensor_3d(ac,GGML_TYPE_F32,128,16,1);
		auto* kr=ggml_new_tensor_3d(ac,GGML_TYPE_F16,128,8,256);
		auto* vr=ggml_new_tensor_3d(ac,GGML_TYPE_F16,128,8,256);
		auto* mask=ggml_new_tensor_2d(ac,GGML_TYPE_F16,256,1);
		for (unsigned i=0;i<2048;i++) static_cast<float*>(qr->data)[i]=1;
		memset(kr->data,0,ggml_nbytes(kr));
		for (unsigned t=0;t<256;t++) for (unsigned h=0;h<8;h++) for (unsigned d=0;d<128;d++) {
			uint16_t value=PioneerFloat16(float(t+h+d%3)/8); memcpy(static_cast<uint8_t*>(vr->data)+(t*1024+h*128+d)*2,&value,2);
		}
		for (unsigned t=0;t<256;t++) { uint16_t value=t<2?0:0xfc00; memcpy(static_cast<uint8_t*>(mask->data)+t*2,&value,2); }
		auto* q=ggml_permute(ac,qr,0,2,1,3); auto* k=ggml_permute(ac,kr,0,2,1,3); auto* v=ggml_permute(ac,vr,0,2,1,3);
		auto* attention=ggml_flash_attn_ext(ac,q,k,v,mask,0.0883883461356163f,0,0); ggml_flash_attn_ext_set_prec(attention,GGML_PREC_F32);
		auto* ag=ggml_new_graph(ac); ggml_build_forward_expand(ag,attention); attention->flags|=GGML_TENSOR_FLAG_COMPUTE;
		auto names=[&](unsigned layer) {
			char name[32]; snprintf(name,sizeof(name),"Qcur-%u",layer); ggml_set_name(qr,name);
			snprintf(name,sizeof(name),"cache_k_l%u",layer); ggml_set_name(kr,name);
			snprintf(name,sizeof(name),"cache_v_l%u",layer); ggml_set_name(vr,name);
		};
		names(0); assert(ggml_backend_dev_supports_op(dev,attention));
		attention->src[4]=mask; assert(!ggml_backend_dev_supports_op(dev,attention)); attention->src[4]=NULL;
		ggml_flash_attn_ext_set_prec(attention,GGML_PREC_DEFAULT); assert(!ggml_backend_dev_supports_op(dev,attention)); ggml_flash_attn_ext_set_prec(attention,GGML_PREC_F32);
		ggml_set_name(kr,"cache_k_l1"); assert(!ggml_backend_dev_supports_op(dev,attention)); names(0);
		PioneerGgmlAttention(false); // immutable while backend exists
		for (unsigned layer=0;layer<28;layer++) {
			names(layer); assert(ggml_backend_graph_compute(backend,ag)==GGML_STATUS_SUCCESS);
			for (unsigned h=0;h<16;h++) for (unsigned d=0;d<128;d++) assert(static_cast<float*>(attention->data)[h*128+d]==float(h/2+d%3+.5)/8);
		}
		assert(PioneerGgmlSnapshot(stats) && stats.attentionOps==28 && stats.attentionTiles==224 && !stats.matvecs && io.attentionCalls==(fleetMode>=2?448U:224U) && io.opens==1);
		// Optional public captures exercise nonuniform Q/K/V, every head
		// and layer, and position-major cache packing through the real
		// GGML integration. Goldens come from an independent native CPU
		// nonflash graph, not this software-only device model.
		for (int argument=1;argument<argc;argument++) {
			FILE* file=fopen(argv[argument],"rb"); assert(file);
			PioneerAttentionFixture h={};
			assert(fread(&h,sizeof(h),1,file)==1 && PioneerAttentionFixtureValid(h) && h.layer==unsigned(argument-1)%28);
			std::vector<float> expected(h.resultCount);
			memset(kr->data,0,ggml_nbytes(kr)); memset(vr->data,0,ggml_nbytes(vr));
			assert(fread(qr->data,4,h.queryCount,file)==h.queryCount && fread(kr->data,2,h.kvCount,file)==h.kvCount
				&& fread(vr->data,2,h.kvCount,file)==h.kvCount && fread(expected.data(),4,h.resultCount,file)==h.resultCount
				&& fgetc(file)==EOF && fclose(file)==0);
			for (unsigned t=0;t<256;t++) static_cast<uint16_t*>(mask->data)[t]=t<h.tokens?0:0xfc00;
			names(h.layer); unsigned calls=io.attentionCalls;
			assert(ggml_backend_graph_compute(backend,ag)==GGML_STATUS_SUCCESS && io.attentionCalls-calls>=8
				&& io.attentionCalls-calls<=(fleetMode>=2?8*h.tokens:8U));
			assert(!memcmp(attention->data,expected.data(),8192));
			printf("PUBLIC GGML attention layer=%u n=%u all-head exact CPU-nonflash output PASS (OFFLINE)\n",h.layer,h.tokens);
		}
		assert((argc-1)%28==0);
		for (unsigned i=0;i<2048;i++) static_cast<float*>(qr->data)[i]=1;
		memset(kr->data,0,ggml_nbytes(kr));
		for (unsigned t=0;t<256;t++) for (unsigned h=0;h<8;h++) for (unsigned d=0;d<128;d++) {
			uint16_t value=PioneerFloat16(float(t+h+d%3)/8); memcpy(static_cast<uint8_t*>(vr->data)+(t*1024+h*128+d)*2,&value,2);
		}
		// Same fixed shape, now the entire 256-key prefix; uniform keys have
		// an independent analytic mean, no local numerical-model dependency.
		unsigned boundary=fleetMode>=2?4:256;
		for (unsigned t=0;t<256;t++) static_cast<uint16_t*>(mask->data)[t]=t<boundary?0:0xfc00;
		names(0);
		assert(ggml_backend_graph_compute(backend,ag)==GGML_STATUS_SUCCESS);
		for (unsigned h=0;h<16;h++) for (unsigned d=0;d<128;d++) assert(fabsf(static_cast<float*>(attention->data)[h*128+d]-float((boundary-1)*.5+h/2+d%3)/8)<.00001f);
		// Non-causal hole refuses before any partial output is published.
		names(1); uint16_t hidden=0xfc00; memcpy(static_cast<uint8_t*>(mask->data)+4,&hidden,2);
		std::vector<float> before(2048); memcpy(before.data(),attention->data,8192); unsigned calls=io.attentionCalls;
		assert(ggml_backend_graph_compute(backend,ag)==GGML_STATUS_FAILED && io.attentionCalls==calls && !memcmp(before.data(),attention->data,8192));
		assert(ggml_backend_graph_compute(backend,ag)==GGML_STATUS_FAILED && io.attentionCalls==calls);
		assert(PioneerGgmlFinish()); ggml_backend_free(backend); ggml_free(ac);
		PioneerGgmlAttention(false); PioneerGgmlQ8DualReuse(false); PioneerGgmlQ8Dual(false); PioneerGgmlQ8Exact(false);
		PioneerGgmlFleet(0); PioneerGgmlBulkUpload(0);
		PioneerGgmlResidentKV(false); PioneerGgmlPayloadBlocks(false);
		PioneerGgmlQ8Native(false); PioneerGgmlQ8Compact(false); PioneerGgmlQ8Operands(false);
	}
	for (unsigned reject:{1U,2U,36U,37U,38U}) {
		io=PioneerGgmlHostState(); PioneerGgmlDiagnose(false);
		backend=ggml_backend_dev_init(dev,NULL); assert(backend);
		struct StopAt { unsigned calls=0,limit; } stop; stop.limit=reject;
		PioneerGgmlBoundary([](void* data) { auto& s=*static_cast<StopAt*>(data); return ++s.calls!=s.limit; },&stop);
		assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_FAILED && stop.calls==reject);
		unsigned stopped=stop.calls;
		assert(ggml_backend_graph_compute(backend,graph)==GGML_STATUS_FAILED && stop.calls==stopped);
		assert(io.opens==1 && io.uploads<=18 && io.calls<=1 && PioneerGgmlSnapshot(stats) && stats.failed);
		PioneerGgmlBoundary(NULL,NULL); assert(PioneerGgmlFinish()); ggml_backend_free(backend);
	}
	weights->buffer=NULL; input->buffer=NULL; result->buffer=NULL;
	ggml_backend_buffer_free(buffer); ggml_backend_buffer_free(inputBuffer); ggml_backend_buffer_free(resultBuffer); ggml_free(ctx);
	printf("PASS: pinned ggml registration, strict support policy, two vectors/one upload, graph failure/no fallback, exclusive cleanup; OFFLINE ONLY\n");
}
