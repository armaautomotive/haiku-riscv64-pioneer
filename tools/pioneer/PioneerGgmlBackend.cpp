/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlBackend.h"
#ifdef PIONEER_GGML_OFFLINE_TEST
#include "tests/PioneerGgmlHostDevice.h"
#else
#include "TenstorrentHaikuResident.h"
#endif
#include "ggml-backend-impl.h"
#include "TenstorrentGgufQuant.h"
#include <new>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <string>

static const uint64_t Arena=UINT64_C(2)*1024*1024*1024;
struct PioneerGgmlContext {
	PioneerResidentDevice device;
	PioneerResidentTensorCache<PioneerResidentDevice> cache;
	bool opened=false,failed=false;
	PioneerGgmlContext():cache(device,Arena) {}
};
// One exclusive backend instance per process, matching the current device
// session contract. Driver handle and cache are destroyed with the backend.
static PioneerGgmlContext* sActive=NULL;
static bool sDiagnose=false;
static bool sCpuActivationQuantization=false;
static const char* sDiagnosticVector=NULL;
struct ReferenceProjection { std::vector<float> input,output; };
static std::map<std::string,ReferenceProjection> sReference;
static const char* Name(ggml_backend_t) { return "P100A-resident-experimental"; }
static const char* DeviceName(ggml_backend_dev_t) { return "P100A-resident"; }
static const char* Description(ggml_backend_dev_t) { return "P100A single-core resident BF16 prototype (not a verified speedup)"; }
static bool View(enum ggml_op op) {
	return op==GGML_OP_NONE || op==GGML_OP_RESHAPE || op==GGML_OP_VIEW || op==GGML_OP_PERMUTE || op==GGML_OP_TRANSPOSE;
}
static bool Supports(ggml_backend_dev_t,const ggml_tensor* op) {
	if (View(op->op)) return true;
	if (op->op!=GGML_OP_MUL_MAT || !op->src[0] || !op->src[1]) return false;
	const auto* w=op->src[0]; const auto* x=op->src[1];
	// Strict immutable learned Q8_0 matrices only, not dynamic attention
	// matrices, quantized activations, batches or higher-rank broadcasts.
	return w->type==GGML_TYPE_Q8_0 && x->type==GGML_TYPE_F32 && op->type==GGML_TYPE_F32
		&& w->ne[0]>0 && w->ne[0]<=TT_RUNTIME_MATVEC_INPUTS_MAX && w->ne[1]>0 && w->ne[1]<=262144
		&& PioneerResidentTensorShape(unsigned(w->ne[0]),unsigned(w->ne[1]))
		&& w->ne[2]==1 && w->ne[3]==1 && x->ne[0]==w->ne[0] && x->ne[1]==1 && x->ne[2]==1 && x->ne[3]==1
		&& op->ne[0]==w->ne[1] && op->ne[1]==1 && op->ne[2]==1 && op->ne[3]==1
		&& ggml_is_contiguous(w) && ggml_is_contiguous(x) && ggml_is_contiguous(op)
		&& w->buffer && ggml_backend_buffer_get_usage(w->buffer)==GGML_BACKEND_BUFFER_USAGE_WEIGHTS;
}
static void Free(ggml_backend_t backend) {
	auto* ctx=(PioneerGgmlContext*)backend->context;
	if (sActive==ctx) sActive=NULL;
	delete ctx; delete backend;
}
static bool CheckProjection(const ggml_tensor* w,const float* x,const float* y,const float* graphX) {
	unsigned inputs=unsigned(w->ne[0]),outputs=unsigned(w->ne[1]),samples=outputs<64?outputs:64;
	std::vector<float> decoded(inputs),independent(inputs);
	double bfDot=0,bfA2=0,bfB2=0,bfDelta2=0,fpDot=0,fpB2=0,fpDelta2=0;
	float maximum=0; unsigned failures=0,decodeMismatches=0;
	std::vector<uint8_t> quantized(inputs/32*34); std::vector<float> quantizedX(inputs);
	ggml_get_type_traits(GGML_TYPE_Q8_0)->from_float_ref(x,quantized.data(),inputs);
	for (unsigned k=0;k<inputs;k+=32) if (!PioneerDecodeQ8_0(quantized.data()+(k/32)*34,quantizedX.data()+k)) return false;
	auto found=sReference.find(w->name);
	bool graph=found!=sReference.end() && found->second.input.size()==inputs && found->second.output.size()==outputs;
	double graphInputDelta=0,graphInputNorm=0,graphOutputDelta=0,graphOutputNorm=0,cpuQuantDelta=0;
	if (graph) for (unsigned k=0;k<inputs;k++) {
		double reference=found->second.input[k],difference=double(graphX[k])-reference;
		graphInputDelta+=difference*difference; graphInputNorm+=reference*reference;
	}
	for (unsigned sample=0;sample<samples;sample++) {
		unsigned row=sample*(outputs-1)/(samples-1);
		const uint8_t* bytes=(const uint8_t*)w->data+row*w->nb[1];
		ggml_get_type_traits(w->type)->to_float(bytes,decoded.data(),inputs);
		for (unsigned k=0;k<inputs;k+=32) if (!PioneerDecodeQ8_0(bytes+(k/32)*34,independent.data()+k)) return false;
		float bf16=0,fp32=0,absolute=0,cpuQuantized=0;
		for (unsigned k=0;k<inputs;k++) {
			decodeMismatches+=decoded[k]!=independent[k];
			uint32_t wb=uint32_t(PioneerResidentBf16(independent[k]))<<16,xb=uint32_t(PioneerResidentBf16(x[k]))<<16;
			float a,b; memcpy(&a,&wb,4); memcpy(&b,&xb,4);
			float product=a*b; bf16+=product; absolute+=fabsf(product); fp32+=independent[k]*x[k];
			cpuQuantized+=independent[k]*quantizedX[k];
		}
		float actual=y[row],error=fabsf(actual-bf16),normalized=absolute>0?error/absolute:error;
		if (normalized>maximum) maximum=normalized;
		bool bad=!isfinite(actual) || error>1e-6f+absolute/1024.0f;
		if (bad && failures<4) fprintf(stderr,"  primitive row%u actual %.9g independent-BF16 %.9g original-Q8 %.9g error/sumabs %.9g\n",row,actual,bf16,fp32,normalized);
		failures+=bad;
		bfDot+=double(actual)*bf16; bfA2+=double(actual)*actual; bfB2+=double(bf16)*bf16;
		double d=double(actual)-bf16; bfDelta2+=d*d;
		fpDot+=double(actual)*fp32; fpB2+=double(fp32)*fp32; d=double(actual)-fp32; fpDelta2+=d*d;
		if (graph) {
			double reference=found->second.output[row]; graphOutputNorm+=reference*reference;
			d=double(actual)-reference; graphOutputDelta+=d*d;
			d=double(cpuQuantized)-reference; cpuQuantDelta+=d*d;
		}
	}
	double bfRelative=bfB2>0?sqrt(bfDelta2/bfB2):sqrt(bfDelta2),fpRelative=fpB2>0?sqrt(fpDelta2/fpB2):sqrt(fpDelta2);
	double bfCos=bfB2>0 && bfA2>0?bfDot/sqrt(bfA2*bfB2):1,fpCos=fpB2>0 && bfA2>0?fpDot/sqrt(bfA2*fpB2):1;
	fprintf(stderr,"PRIMITIVE %s [%u,%u] samples %u BF16 cosine/L2 %.9f/%.9f original-Q8 cosine/L2 %.9f/%.9f normalized %.9g failures %u decoder mismatches %u\n",
		w->name,inputs,outputs,samples,bfCos,bfRelative,fpCos,fpRelative,maximum,failures,decodeMismatches);
	bool passed=!decodeMismatches && !failures && isfinite(bfRelative) && bfRelative<.01;
	if (graph) {
		double inputError=sqrt(graphInputDelta/(graphInputNorm+1e-30)),outputError=sqrt(graphOutputDelta/(graphOutputNorm+1e-30));
		fprintf(stderr,"  GRAPH CPU reference input-L2 %.9f output-L2 %.9f; CPU-Q8-activation emulation vs graph-output L2 %.9f\n",
			inputError,outputError,sqrt(cpuQuantDelta/(graphOutputNorm+1e-30)));
		passed &= isfinite(inputError) && isfinite(outputError) && inputError<.02 && outputError<.02;
	}
	if (!passed && sDiagnosticVector) {
		int fd=open(sDiagnosticVector,O_WRONLY|O_CREAT|O_EXCL,0600);
		if (fd<0) perror("exclusive diagnostic vector");
		else {
			const char* bytes=(const char*)x; size_t remaining=inputs*sizeof(float);
			while (remaining) {
				ssize_t written=write(fd,bytes,remaining);
				if (written<0 && errno==EINTR) continue;
				if (written<=0) break;
				bytes+=written; remaining-=size_t(written);
			}
			close(fd);
			fprintf(stderr,"Diagnostic FP32 input %s: %s, tensor %s K%u\n",remaining?"INCOMPLETE":"saved",sDiagnosticVector,w->name,inputs);
		}
	}
	return passed;
}
static enum ggml_status Compute(ggml_backend_t backend,ggml_cgraph* graph) {
	auto* ctx=(PioneerGgmlContext*)backend->context;
	if (ctx->failed) return GGML_STATUS_FAILED;
	for (int i=0;i<ggml_graph_n_nodes(graph);i++) {
		auto* op=ggml_graph_node(graph,i);
		if (!(op->flags&GGML_TENSOR_FLAG_COMPUTE) || View(op->op)) continue;
		if (!Supports(NULL,op) || !op->data || !op->src[0]->data || !op->src[1]->data) { ctx->failed=true; return GGML_STATUS_FAILED; }
		if (!ctx->opened) {
			ctx->opened=ctx->device.Open(Arena);
			if (!ctx->opened) { ctx->failed=true; return GGML_STATUS_FAILED; }
		}
		const auto* w=op->src[0]; unsigned inputs=unsigned(w->ne[0]),outputs=unsigned(w->ne[1]);
		uint64_t offset=0; size_t before=ctx->cache.Tensors();
		if (!ctx->cache.Weights(w->data,unsigned(w->type),inputs,outputs,[&](unsigned row,float* out) {
			ggml_get_type_traits(w->type)->to_float((const char*)w->data+row*w->nb[1],out,inputs); return true;
		},offset)) { fprintf(stderr,"P100A tensor upload failed: %s; no CPU fallback\n",w->name); ctx->failed=true; return GGML_STATUS_FAILED; }
		if (ctx->cache.Tensors()!=before) fprintf(stderr,"P100A retained %s [%u,%u], total %llu MiB / %zu tensors\n",
			w->name,inputs,outputs,(unsigned long long)(ctx->cache.Bytes()/1048576),ctx->cache.Tensors());
		const float* original=(const float*)op->src[1]->data;
		const float* execution=original;
		std::vector<uint8_t> quantized;
		std::vector<float> converted;
		if (sCpuActivationQuantization) {
			quantized.resize(inputs/32*34); converted.resize(inputs);
			const auto* traits=ggml_get_type_traits(GGML_TYPE_Q8_0);
			traits->from_float_ref(original,quantized.data(),inputs);
			traits->to_float(quantized.data(),converted.data(),inputs); execution=converted.data();
		}
		if (!ctx->cache.Multiply(offset,inputs,outputs,execution,(float*)op->data)) {
			ctx->failed=true; return GGML_STATUS_FAILED;
		}
		if (sDiagnose && !CheckProjection(w,execution,(const float*)op->data,original)) {
			fprintf(stderr,"Primitive numerical diagnostic STOP; no CPU fallback or substituted output\n"); ctx->failed=true; return GGML_STATUS_FAILED;
		}
	}
	return GGML_STATUS_SUCCESS;
}
static ggml_backend_t Init(ggml_backend_dev_t dev,const char*) {
	if (sActive) return NULL;
	auto* ctx=new(std::nothrow) PioneerGgmlContext; if (!ctx) return NULL;
	ggml_backend_i iface={}; iface.get_name=Name; iface.free=Free; iface.graph_compute=Compute;
	static ggml_guid guid={0xb2,0x6f,0x01,0x04,0x76,0xe1,0x41,0x3b,0x85,0xa2,0x66,0x22,0x91,0x16,0x32,0x08};
	auto* backend=new(std::nothrow) ggml_backend {&guid,iface,dev,ctx};
	if (!backend) { delete ctx; return NULL; }
	sActive=ctx; return backend;
}
static void Memory(ggml_backend_dev_t,size_t* available,size_t* total) { *available=*total=size_t(Arena); }
static enum ggml_backend_dev_type Type(ggml_backend_dev_t) { return GGML_BACKEND_DEVICE_TYPE_ACCEL; }
static void Props(ggml_backend_dev_t dev,ggml_backend_dev_props* props) {
	memset(props,0,sizeof(*props)); props->name=DeviceName(dev); props->description=Description(dev); props->type=Type(dev);
	Memory(dev,&props->memory_free,&props->memory_total); props->caps.buffer_from_host_ptr=true; props->caps.mmap_support=true;
}
static ggml_backend_buffer_type_t BufferType(ggml_backend_dev_t) { return ggml_backend_cpu_buffer_type(); }
static ggml_backend_buffer_t FromHost(ggml_backend_dev_t,void* ptr,size_t bytes,size_t) { return ggml_backend_cpu_buffer_from_ptr(ptr,bytes); }
static bool BufferSupported(ggml_backend_dev_t,ggml_backend_buffer_type_t type) { return ggml_backend_buft_is_host(type); }
static const char* RegName(ggml_backend_reg_t) { return "Pioneer P100A resident prototype"; }
static size_t Count(ggml_backend_reg_t) { return 1; }
static ggml_backend_dev_t Device(ggml_backend_reg_t reg,size_t index) {
	if (index) return NULL;
	static ggml_backend_device dev={};
	dev.reg=reg; dev.iface.get_name=DeviceName; dev.iface.get_description=Description;
	dev.iface.get_memory=Memory; dev.iface.get_type=Type; dev.iface.get_props=Props;
	dev.iface.init_backend=Init; dev.iface.get_buffer_type=BufferType; dev.iface.buffer_from_host_ptr=FromHost;
	dev.iface.supports_op=Supports; dev.iface.supports_buft=BufferSupported;
	return &dev;
}
ggml_backend_reg_t PioneerGgmlReg() {
	static ggml_backend_reg reg={GGML_BACKEND_API_VERSION,{RegName,Count,Device,NULL},NULL}; return &reg;
}
bool PioneerGgmlSnapshot(PioneerGgmlStats& stats) {
	stats={}; if (!sActive) return false;
	stats.tensors=sActive->cache.Tensors(); stats.bytes=sActive->cache.Bytes(); stats.uploads=sActive->cache.uploadedPages;
	stats.matvecs=sActive->cache.matvecCalls; stats.failed=sActive->failed || sActive->cache.Failed(); return true;
}
bool PioneerGgmlFinish() { return !sActive || sActive->device.Finish(); }
void PioneerGgmlDiagnose(bool enabled,const char* vectorPath) { if (!sActive) { sDiagnose=enabled; sDiagnosticVector=vectorPath; sReference.clear(); } }
void PioneerGgmlCpuActivationQuantization(bool enabled) { if (!sActive) sCpuActivationQuantization=enabled; }
bool PioneerGgmlReferenceTap(ggml_tensor* t,bool ask,void*) {
	bool wanted=t->op==GGML_OP_MUL_MAT && t->src[0] && t->src[0]->type==GGML_TYPE_Q8_0 && Supports(NULL,t);
	if (ask) return wanted;
	if (!wanted) return true;
	auto& r=sReference[t->src[0]->name];
	r.input.resize(size_t(t->src[0]->ne[0])); r.output.resize(size_t(t->src[0]->ne[1]));
	ggml_backend_tensor_get(t->src[1],r.input.data(),0,r.input.size()*sizeof(float));
	ggml_backend_tensor_get(t,r.output.data(),0,r.output.size()*sizeof(float));
	if (!(sReference.size()%32)) fprintf(stderr,"CPU graph reference captured: %zu learned matrices\n",sReference.size());
	return true;
}
size_t PioneerGgmlReferenceCount() { return sReference.size(); }
