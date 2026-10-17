/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlBackend.h"
#ifdef PIONEER_GGML_OFFLINE_TEST
#include "tests/PioneerGgmlHostDevice.h"
#else
#include "TenstorrentHaikuResident.h"
#endif
#include "ggml-backend-impl.h"
#include "TenstorrentGgufQuant.h"
#include "PioneerBlackholeFma.h"
#include "PioneerGgmlAttention.h"
#include "PioneerAttentionPrefixCache.h"
#include <new>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <string>
#include <time.h>

static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }

static const uint64_t Arena=UINT64_C(2)*1024*1024*1024;
static bool sCreateExecutable=true;
static PioneerGgmlBoundaryCallback sBoundary=NULL;
static void* sBoundaryData=NULL;
static bool Boundary() { return !sBoundary || sBoundary(sBoundaryData); }
struct PioneerGgmlDevice:PioneerResidentDevice {
	bool UploadPages(uint64_t offset,const uint32_t* data,unsigned pages) {
		return Boundary() && PioneerResidentDevice::UploadPages(offset,data,pages) && Boundary();
	}
	bool Upload(uint64_t offset,const uint32_t* data) {
		return Boundary() && PioneerResidentDevice::Upload(offset,data) && Boundary();
	}
	bool Q8Matvec(uint64_t offset,unsigned k,unsigned n,const uint32_t* x,const uint32_t* d,uint32_t* y) {
		return Boundary() && PioneerResidentDevice::Q8Matvec(offset,k,n,x,d,y) && Boundary();
	}
	bool Matvec(uint64_t offset,unsigned k,unsigned n,const uint32_t* x,uint32_t* y) {
		return Boundary() && PioneerResidentDevice::Matvec(offset,k,n,x,y) && Boundary();
	}
	bool AttentionTile(unsigned n,const uint32_t* q,const uint32_t* k,const uint32_t* v,uint32_t* y) {
		return Boundary() && PioneerResidentDevice::AttentionTile(n,q,k,v,y) && Boundary();
	}
	bool AttentionGroups(unsigned n,const uint32_t* q,const uint32_t* k,const uint32_t* v,uint32_t* y) {
		return Boundary() && PioneerResidentDevice::AttentionGroups(n,q,k,v,y) && Boundary();
	}
	bool ResidentAttentionStep(unsigned n,unsigned layer,const uint32_t* q,const uint32_t* k,const uint32_t* v,uint32_t* y) {
		return Boundary() && PioneerResidentDevice::ResidentAttentionStep(n,layer,q,k,v,y) && Boundary();
	}
};
struct PioneerGgmlContext {
	PioneerGgmlDevice device;
	PioneerResidentTensorCache<PioneerGgmlDevice> cache;
	bool opened=false,failed=false;
	const bool fp16;
	const bool q8;
	const bool compact;
	const bool native;
	const bool exact;
	const bool dual;
	const bool reuse;
	const bool attention;
	const unsigned fleet;
	const bool residentKV;
	const bool payloadBlocks;
	PioneerAttentionPrefixCache prefix;
	uint64_t attentionOps=0,attentionTiles=0;
	unsigned attentionLayer=0;
	double weightSeconds=0,inputSeconds=0,matvecSeconds=0,diagnosticSeconds=0,attentionSeconds=0;
	PioneerGgmlContext(bool half,bool quant,bool rawBytes,bool hardwareUnpack,bool accurate,bool twoCores,bool reuseInput,bool deviceAttention,unsigned workers,bool deviceKV,bool blocks):cache(device,Arena-(deviceKV?PIONEER_RESIDENT_KV_BYTES:0),half,rawBytes,hardwareUnpack,accurate,workers!=0),fp16(half),q8(quant),compact(rawBytes),native(hardwareUnpack),exact(accurate),dual(twoCores),reuse(reuseInput),attention(deviceAttention),fleet(workers),residentKV(deviceKV),payloadBlocks(blocks) {}
};
// One exclusive backend instance per process, matching the current device
// session contract. Driver handle and cache are destroyed with the backend.
static PioneerGgmlContext* sActive=NULL;
static bool sDiagnose=false;
static bool sCpuActivationQuantization=false;
static bool sFp16=false;
static bool sQ8=false;
static bool sCompact=false;
static bool sNative=false;
static bool sExact=false;
static bool sDual=false;
static bool sDualReuse=false;
static bool sAttention=false;
static unsigned sBulkUpload=0;
static unsigned sFleet=0;
static bool sResidentKV=false;
static bool sPayloadBlocks=false;
static bool sTrace=false;
static const char* sDiagnosticVector=NULL;
struct ReferenceProjection { std::vector<float> input,output; const ggml_tensor* weights=NULL; };
static std::map<std::string,ReferenceProjection> sReference;
static std::map<std::string,std::vector<float>> sIntermediate;
static std::map<std::string,const ggml_tensor*> sIntermediateNodes;
static std::map<std::string,std::vector<uint8_t>> sRopePositions;
static unsigned sPosition=0;
static std::string TraceKey(const ggml_tensor* t) {
	std::string key=std::string(t->name)+"|"+std::to_string(int(t->op));
	for (unsigned i=0;i<4;i++) key+="|"+std::to_string(t->ne[i]);
	return key;
}
static bool TraceTensor(const ggml_tensor* t) {
	if (!sTrace || t->type!=GGML_TYPE_F32 || !ggml_is_contiguous(t) || ggml_nelements(t)<=0 || ggml_nelements(t)>262144) return false;
	if (t->op==GGML_OP_NONE || t->op==GGML_OP_VIEW || t->op==GGML_OP_RESHAPE || t->op==GGML_OP_TRANSPOSE || t->op==GGML_OP_PERMUTE) return false;
	const char* prefixes[]={"attn_norm-","Qcur-","Kcur-","Qcur_normed-","Kcur_normed-","kq-","kq_soft_max-","kqv-","kqv_out-"};
	for (const char* p:prefixes) if (!strncmp(t->name,p,strlen(p))) return true;
	return false;
}
static const char* Name(ggml_backend_t backend) { return backend->context?"P100A-resident-experimental":"P100A-disabled-for-CPU-context"; }
static const char* DeviceName(ggml_backend_dev_t) { return "P100A-resident"; }
static const char* Description(ggml_backend_dev_t) { return sFleet?"P100A whole-projection prototype (up to120 workers; not a verified CPU speedup)":sDual?"P100A two-core resident matrix prototype (not a verified speedup)":"P100A single-core resident matrix prototype (not a verified speedup)"; }
static bool View(enum ggml_op op) {
	return op==GGML_OP_NONE || op==GGML_OP_RESHAPE || op==GGML_OP_VIEW || op==GGML_OP_PERMUTE || op==GGML_OP_TRANSPOSE;
}
static bool Supports(ggml_backend_dev_t,const ggml_tensor* op) {
	if (View(op->op)) return true;
	if (sAttention && op->op==GGML_OP_FLASH_ATTN_EXT) return PioneerGgmlAttentionContract(op);
	if (op->op!=GGML_OP_MUL_MAT || !op->src[0] || !op->src[1]) return false;
	const auto* w=op->src[0]; const auto* x=op->src[1];
	// Strict immutable learned Q8_0 matrices only, not dynamic attention
	// matrices, quantized activations, batches or higher-rank broadcasts.
	return w->type==GGML_TYPE_Q8_0 && x->type==GGML_TYPE_F32 && op->type==GGML_TYPE_F32
		&& w->ne[0]>0 && w->ne[0]<=TT_RUNTIME_MATVEC_INPUTS_MAX && w->ne[1]>0 && w->ne[1]<=262144
		&& PioneerResidentTensorShape(unsigned(w->ne[0]),unsigned(w->ne[1]))
		&& (!sCompact || PioneerQ8CompactTensorShape(unsigned(w->ne[0]),unsigned(w->ne[1])))
		&& (sFleet || !sDual || (w->ne[1]>=64 && !(w->ne[1]&63)))
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
			if (sFp16) { a=PioneerFloat16Value(PioneerFloat16(independent[k])&~1U); b=PioneerFloat16Value(PioneerFloat16(x[k])); }
			float product=a*b;
			if (!sQ8) { bf16+=product; absolute+=fabsf(product); }
			fp32+=independent[k]*x[k];
			cpuQuantized+=independent[k]*quantizedX[k];
		}
		if (sQ8) for (unsigned block=0;block<inputs/32;block++) {
			const uint8_t* wb=bytes+block*34; const uint8_t* xb=quantized.data()+block*34; int dot=0;
			for (unsigned k=0;k<32;k++) { int a=wb[2+k],b=xb[2+k]; if (a>=128) a-=256; if (b>=128) b-=256; dot+=a*b; }
			float dw=PioneerHalfFloat(uint16_t(wb[0])|(uint16_t(wb[1])<<8)),dx=PioneerHalfFloat(uint16_t(xb[0])|(uint16_t(xb[1])<<8));
			float product=float(dot)*(dw*dx); bf16+=product; absolute+=fabsf(product);
		}
		float actual=y[row],error=fabsf(actual-bf16),normalized=absolute>0?error/absolute:error;
		if (normalized>maximum) maximum=normalized;
		bool bad=!isfinite(actual) || error>1e-6f+absolute*(sQ8?1e-6f:1/1024.0f);
		if (bad && failures<4) fprintf(stderr,"  primitive row%u actual %.9g independent-%s %.9g original-Q8 %.9g error/sumabs %.9g\n",row,actual,sQ8?"integer-Q8-blocks":sFp16?"FP16-phases":"BF16",bf16,fp32,normalized);
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
	fprintf(stderr,"PRIMITIVE %s [%u,%u] samples %u %s cosine/L2 %.9f/%.9f original-Q8 cosine/L2 %.9f/%.9f normalized %.9g failures %u decoder mismatches %u\n",
		w->name,inputs,outputs,samples,sQ8?"integer-Q8-blocks":sFp16?"FP16-phases":"BF16",bfCos,bfRelative,fpCos,fpRelative,maximum,failures,decodeMismatches);
	bool passed=!decodeMismatches && !failures && isfinite(bfRelative) && bfRelative<(sQ8?1e-5:.01);
	if (graph) {
		double inputError=sqrt(graphInputDelta/(graphInputNorm+1e-30)),outputError=sqrt(graphOutputDelta/(graphOutputNorm+1e-30));
		fprintf(stderr,"  GRAPH CPU reference input-L2 %.9f output-L2 %.9f; CPU-Q8-activation emulation vs graph-output L2 %.9f\n",
			inputError,outputError,sqrt(cpuQuantDelta/(graphOutputNorm+1e-30)));
		passed &= isfinite(inputError) && isfinite(outputError) && inputError<.02 && outputError<.02;
		if (sTrace && sQ8) {
			std::vector<uint8_t> referenceQ(inputs/32*34);
			ggml_get_type_traits(GGML_TYPE_Q8_0)->from_float_ref(found->second.input.data(),referenceQ.data(),inputs);
			unsigned scaleDifferences=0,valueDifferences=0;
			for (unsigned b=0;b<inputs/32;b++) {
				scaleDifferences+=memcmp(referenceQ.data()+b*34,quantized.data()+b*34,2)!=0;
				for (unsigned i=2;i<34;i++) valueDifferences+=referenceQ[b*34+i]!=quantized[b*34+i];
			}
			fprintf(stderr,"  TRACE pos%u Q8 input differences: scale blocks %u/%u, values %u/%u; diagnostic only\n",sPosition,scaleDifferences,inputs/32,valueDifferences,inputs);
		}
		if (sQ8 && (strstr(w->name,".attn_v.weight") || strstr(w->name,".attn_k.weight"))) {
			unsigned halfDifferences=0; float maxDifference=0;
			for (unsigned row=0;row<outputs;row++) {
				float reference=found->second.output[row];
				halfDifferences+=ggml_fp32_to_fp16(y[row])!=ggml_fp32_to_fp16(reference);
				float difference=fabsf(y[row]-reference); if (difference>maxDifference) maxDifference=difference;
			}
			fprintf(stderr,"  KV PRECISION pre-cache projection half-rounding disagreements %u/%u, max FP32 difference %.9g; diagnostic only, K normalization/RoPE occur downstream\n",halfDifferences,outputs,maxDifference);
		}
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
static bool ComputeAttention(PioneerGgmlContext& ctx,ggml_tensor* op)
{
	unsigned layer=0;
	if (!ctx.attention || !PioneerGgmlAttentionContract(op,&layer) || layer!=ctx.attentionLayer
		|| !op->src[2]->data || !op->src[3]->data) return false;
	const auto* mask=static_cast<const uint8_t*>(op->src[3]->data);
	unsigned tokens=0;
	for (unsigned i=0;i<256;i++) {
		uint16_t bits; memcpy(&bits,mask+i*2,2);
		if (!bits && tokens==i) tokens++;
		else if (bits!=0xfc00) return false;
	}
	if (!tokens) return false;
	std::vector<uint32_t> query(256),keys(tokens*64),values(tokens*64),result(2048);
	const auto* q=static_cast<const uint8_t*>(op->src[0]->data);
	const auto* k=static_cast<const uint8_t*>(op->src[1]->data);
	const auto* v=static_cast<const uint8_t*>(op->src[2]->data);
	double started=Seconds();
	if (ctx.fleet) {
		std::vector<uint32_t> allKeys(tokens*512),allValues(tokens*512);
		for (unsigned group=0;group<8;group++) for (unsigned t=0;t<tokens;t++) {
			memcpy(allKeys.data()+group*tokens*64+t*64,k+t*2048+group*256,256);
			memcpy(allValues.data()+group*tokens*64+t*64,v+t*2048+group*256,256);
		}
		bool okay=ctx.residentKV?ctx.prefix.Run(ctx.device,layer,tokens,reinterpret_cast<const uint32_t*>(q),allKeys.data(),allValues.data(),result.data())
			:ctx.device.AttentionGroups(tokens,reinterpret_cast<const uint32_t*>(q),allKeys.data(),allValues.data(),result.data());
		if (!okay) return false;
		ctx.attentionTiles+=8;
	} else for (unsigned group=0;group<8;group++) {
		memcpy(query.data(),q+group*1024,1024);
		for (unsigned t=0;t<tokens;t++) {
			memcpy(keys.data()+t*64,k+t*2048+group*256,256);
			memcpy(values.data()+t*64,v+t*2048+group*256,256);
		}
		if (!ctx.device.AttentionTile(tokens,query.data(),keys.data(),values.data(),result.data()+group*256)) return false;
		ctx.attentionTiles++;
	}
	ctx.attentionSeconds+=Seconds()-started;
	// Publish only after all eight grouped-head operations are proved.
	memcpy(op->data,result.data(),8192); ctx.attentionOps++;
	ctx.attentionLayer=(ctx.attentionLayer+1)%28;
	return true;
}
static enum ggml_status Compute(ggml_backend_t backend,ggml_cgraph* graph) {
	auto* ctx=(PioneerGgmlContext*)backend->context;
	if (!ctx || ctx->failed) return GGML_STATUS_FAILED;
	for (int i=0;i<ggml_graph_n_nodes(graph);i++) {
		auto* op=ggml_graph_node(graph,i);
		if (!(op->flags&GGML_TENSOR_FLAG_COMPUTE) || View(op->op)) continue;
		if (!Supports(NULL,op) || !op->data || !op->src[0]->data || !op->src[1]->data) { ctx->failed=true; return GGML_STATUS_FAILED; }
		if (!ctx->opened) {
			ctx->opened=ctx->device.Open(Arena,ctx->fp16,ctx->q8,ctx->compact,ctx->native,ctx->exact,ctx->dual,ctx->reuse,ctx->attention,sBulkUpload,ctx->fleet,ctx->residentKV,ctx->payloadBlocks);
			if (!ctx->opened) { ctx->failed=true; return GGML_STATUS_FAILED; }
		}
		if (op->op==GGML_OP_FLASH_ATTN_EXT) {
			if (!ComputeAttention(*ctx,op)) { ctx->failed=true; return GGML_STATUS_FAILED; }
			continue;
		}
		const auto* w=op->src[0]; unsigned inputs=unsigned(w->ne[0]),outputs=unsigned(w->ne[1]);
		uint64_t offset=0; size_t before=ctx->cache.Tensors();
		double start=Seconds();
		bool weightsOkay=ctx->q8?ctx->cache.Q8Weights(w->data,unsigned(w->type),inputs,outputs,[&](unsigned row,unsigned block,uint8_t* out) {
			memcpy(out,(const uint8_t*)w->data+row*w->nb[1]+block*34,34); return true;
		},offset):ctx->cache.Weights(w->data,unsigned(w->type),inputs,outputs,[&](unsigned row,float* out) {
			ggml_get_type_traits(w->type)->to_float((const char*)w->data+row*w->nb[1],out,inputs); return true;
		},offset);
		ctx->weightSeconds+=Seconds()-start;
		if (!weightsOkay) { fprintf(stderr,"P100A tensor upload failed: %s; no CPU fallback\n",w->name); ctx->failed=true; return GGML_STATUS_FAILED; }
		if (ctx->cache.Tensors()!=before) fprintf(stderr,"P100A retained %s [%u,%u], total %llu MiB / %zu tensors\n",
			w->name,inputs,outputs,(unsigned long long)(ctx->cache.Bytes()/1048576),ctx->cache.Tensors());
		const float* original=(const float*)op->src[1]->data;
		const float* execution=original;
		std::vector<uint8_t> quantized;
		std::vector<float> converted;
		start=Seconds();
		if (sCpuActivationQuantization || ctx->q8) {
			for (unsigned k=0;k<inputs;k++) if (!isfinite(original[k])) { ctx->failed=true; return GGML_STATUS_FAILED; }
			quantized.resize(inputs/32*34);
			const auto* traits=ggml_get_type_traits(GGML_TYPE_Q8_0);
			traits->from_float_ref(original,quantized.data(),inputs);
			if (!ctx->q8) { converted.resize(inputs); traits->to_float(quantized.data(),converted.data(),inputs); execution=converted.data(); }
		}
		ctx->inputSeconds+=Seconds()-start;
		start=Seconds();
		bool mathOkay=ctx->q8?ctx->cache.Q8Multiply(offset,inputs,outputs,quantized.data(),(float*)op->data)
			:ctx->cache.Multiply(offset,inputs,outputs,execution,(float*)op->data);
		ctx->matvecSeconds+=Seconds()-start;
		if (!mathOkay) {
			ctx->failed=true; return GGML_STATUS_FAILED;
		}
		start=Seconds();
		bool diagnosticOkay=!sDiagnose || CheckProjection(w,execution,(const float*)op->data,original);
		ctx->diagnosticSeconds+=Seconds()-start;
		if (!diagnosticOkay) {
			fprintf(stderr,"Primitive numerical diagnostic STOP; no CPU fallback or substituted output\n"); ctx->failed=true; return GGML_STATUS_FAILED;
		}
	}
	return GGML_STATUS_SUCCESS;
}
static bool DisabledSupports(ggml_backend_dev_t,const ggml_tensor*) { return false; }
static bool DisabledBuffer(ggml_backend_dev_t,ggml_backend_buffer_type_t type) {
	// ggml requires every scheduler backend to accept its default storage.
	// This is host storage only; rejecting ALL operations is what prevents
	// scheduling or executing any card work in this inert backend.
	return type==ggml_backend_cpu_buffer_type();
}
static const char* DisabledName(ggml_backend_dev_t) { return "P100A-disabled-for-CPU-context"; }
static void DisabledMemory(ggml_backend_dev_t,size_t* available,size_t* total) { *available=*total=0; }
static void DisabledProps(ggml_backend_dev_t,ggml_backend_dev_props* p) {
	memset(p,0,sizeof(*p)); p->name=p->description="P100A-disabled-for-CPU-context"; p->type=GGML_BACKEND_DEVICE_TYPE_ACCEL;
}
static ggml_backend_t Init(ggml_backend_dev_t dev,const char*) {
	static ggml_guid guid={0xb2,0x6f,0x01,0x04,0x76,0xe1,0x41,0x3b,0x85,0xa2,0x66,0x22,0x91,0x16,0x32,0x08};
	ggml_backend_i iface={}; iface.get_name=Name; iface.free=Free; iface.graph_compute=Compute;
	if (!sCreateExecutable) {
		// Do not claim sActive or allow any graph operation. The scheduler
		// still requires compatibility with our default host storage.
		// CPU contexts remain safe alongside a warm
		// P100A context and after its registration in the global registry.
		static ggml_backend_device disabled={}; disabled=*dev;
		disabled.iface.get_name=DisabledName; disabled.iface.get_description=DisabledName;
		disabled.iface.get_memory=DisabledMemory; disabled.iface.get_props=DisabledProps;
		disabled.iface.supports_op=DisabledSupports; disabled.iface.supports_buft=DisabledBuffer;
		return new(std::nothrow) ggml_backend {&guid,iface,&disabled,NULL};
	}
	if (sActive || (sFp16 && sQ8) || (sCompact && !sQ8) || (sNative && !sCompact) || (sExact && !sNative) || (sDual && !sExact) || (sDualReuse && !sDual) || (sAttention && !sDualReuse) || sBulkUpload>2 || (sBulkUpload && !sDualReuse) || sFleet>120 || (sFleet && (!sDualReuse || !sBulkUpload))) return NULL;
	if (sResidentKV && (!sFleet || !sAttention)) return NULL;
	if (sPayloadBlocks && !sResidentKV) return NULL;
	auto* ctx=new(std::nothrow) PioneerGgmlContext(sFp16,sQ8,sCompact,sNative,sExact,sDual,sDualReuse,sAttention,sFleet,sResidentKV,sPayloadBlocks); if (!ctx) return NULL;
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
void PioneerGgmlContextExecution(bool enabled) { sCreateExecutable=enabled; }
void PioneerGgmlBoundary(PioneerGgmlBoundaryCallback callback,void* data) { sBoundary=callback; sBoundaryData=data; }
bool PioneerGgmlSnapshot(PioneerGgmlStats& stats) {
	stats={}; if (!sActive) return false;
	stats.tensors=sActive->cache.Tensors(); stats.bytes=sActive->cache.Bytes(); stats.uploads=sActive->cache.uploadedPages;
	stats.matvecs=sActive->cache.matvecCalls; stats.failed=sActive->failed || sActive->cache.Failed();
	stats.peakWorkers=sActive->device.peakWorkers;
	stats.attentionOps=sActive->attentionOps; stats.attentionTiles=sActive->attentionTiles;
	stats.attentionSeconds=sActive->attentionSeconds;
	stats.weightSeconds=sActive->weightSeconds; stats.inputSeconds=sActive->inputSeconds;
	stats.matvecSeconds=sActive->matvecSeconds; stats.diagnosticSeconds=sActive->diagnosticSeconds; return true;
}
bool PioneerGgmlFinish() { return !sActive || sActive->device.Finish(); }
void PioneerGgmlDiagnose(bool enabled,const char* vectorPath) { if (!sActive) { sDiagnose=enabled; sDiagnosticVector=vectorPath; sReference.clear(); } }
void PioneerGgmlCpuActivationQuantization(bool enabled) { if (!sActive) sCpuActivationQuantization=enabled; }
void PioneerGgmlFp16Operands(bool enabled) { if (!sActive) sFp16=enabled; }
void PioneerGgmlQ8Operands(bool enabled) { if (!sActive) sQ8=enabled; }
void PioneerGgmlQ8Compact(bool enabled) { if (!sActive) sCompact=enabled; }
void PioneerGgmlQ8Native(bool enabled) { if (!sActive) sNative=enabled; }
void PioneerGgmlQ8Exact(bool enabled) { if (!sActive) sExact=enabled; }
void PioneerGgmlQ8Dual(bool enabled) { if (!sActive) sDual=enabled; }
void PioneerGgmlQ8DualReuse(bool enabled) { if (!sActive) sDualReuse=enabled; }
void PioneerGgmlAttention(bool enabled) { if (!sActive) sAttention=enabled; }
void PioneerGgmlBulkUpload(unsigned mode) { if (!sActive) sBulkUpload=mode; }
void PioneerGgmlFleet(unsigned workers) { if (!sActive) sFleet=workers; }
void PioneerGgmlResidentKV(bool enabled) { if (!sActive) sResidentKV=enabled; }
void PioneerGgmlPayloadBlocks(bool enabled) { if (!sActive) sPayloadBlocks=enabled; }
void PioneerGgmlTrace(bool enabled) { if (!sActive) sTrace=enabled; }
void PioneerGgmlBeginPosition(unsigned position) { sPosition=position; sReference.clear(); sIntermediate.clear(); sIntermediateNodes.clear(); sRopePositions.clear(); }
bool PioneerGgmlReferenceTap(ggml_tensor* t,bool ask,void*) {
	bool wanted=t->op==GGML_OP_MUL_MAT && t->src[0] && t->src[0]->type==GGML_TYPE_Q8_0 && Supports(NULL,t);
	bool trace=TraceTensor(t);
	if (ask) return wanted || trace;
	if (trace) {
		auto key=TraceKey(t);
		if (sIntermediate.find(key)!=sIntermediate.end()) { fprintf(stderr,"TRACE duplicate reference %s; diagnostic stopped\n",key.c_str()); return false; }
		auto& values=sIntermediate[key]; values.resize(size_t(ggml_nelements(t)));
		sIntermediateNodes[key]=t;
		ggml_backend_tensor_get(t,values.data(),0,values.size()*sizeof(float));
		if (t->op==GGML_OP_ROPE && t->src[1] && t->src[1]->type==GGML_TYPE_I32 && ggml_nbytes(t->src[1])<=1024) {
			auto& positions=sRopePositions[key]; positions.resize(ggml_nbytes(t->src[1]));
			ggml_backend_tensor_get(t->src[1],positions.data(),0,positions.size());
		}
	}
	if (!wanted) return true;
	auto& r=sReference[t->src[0]->name];
	r.weights=t->src[0]; // CPU model owns this immutable tensor until replay finishes
	r.input.resize(size_t(t->src[0]->ne[0])); r.output.resize(size_t(t->src[0]->ne[1]));
	ggml_backend_tensor_get(t->src[1],r.input.data(),0,r.input.size()*sizeof(float));
	ggml_backend_tensor_get(t,r.output.data(),0,r.output.size()*sizeof(float));
	if (!(sReference.size()%32)) fprintf(stderr,"CPU graph reference captured: %zu learned matrices\n",sReference.size());
	return true;
}
size_t PioneerGgmlReferenceCount() { return sReference.size(); }
static bool ReplayPostRope(const char* tensorName,const float* actual,size_t elements,const char* source="actual card projection") {
	unsigned layer=0; char which=0;
	if (sscanf(tensorName,"blk.%u.attn_%c.weight",&layer,&which)!=2 || (which!='k' && which!='q')) return true;
	// This observer copies public test data into a private CPU graph. It
	// uses actual card output, never overwrites the independent reference.
	std::string name=std::string(which=='k'?"Kcur-":"Qcur-")+std::to_string(layer);
	const ggml_tensor* rope=NULL; const std::vector<float>* reference=NULL;
	for (const auto& entry:sIntermediateNodes) if (entry.second->op==GGML_OP_ROPE && name==entry.second->name) {
		rope=entry.second; reference=&sIntermediate.find(entry.first)->second; break;
	}
	if (!rope) {
		if (sTrace) fprintf(stderr,"POST-ROPE %s expected CPU reference missing; diagnostic stopped\n",name.c_str());
		return !sTrace; // standalone primitive tests need not capture attention
	}
	const auto* normal=rope->src[0];
	if (!normal || normal->op!=GGML_OP_MUL || !normal->src[0] || normal->src[0]->op!=GGML_OP_RMS_NORM
		|| !normal->src[1] || normal->src[1]->type!=GGML_TYPE_F32 || !rope->src[1] || rope->src[2]
		|| ggml_nelements(normal)!=int64_t(elements) || reference->size()!=elements) return false;
	const auto* rms=normal->src[0]; float epsilon=0; memcpy(&epsilon,rms->op_params,sizeof(epsilon));
	if (!isfinite(epsilon) || epsilon<=0 || ggml_nelements(normal->src[1])>6144 || ggml_nelements(rope->src[1])>256) return false;
	ggml_init_params params={1024*1024,NULL,false}; auto* ctx=ggml_init(params); if (!ctx) return false;
	auto* input=ggml_new_tensor(ctx,GGML_TYPE_F32,4,rms->src[0]->ne); memcpy(input->data,actual,elements*4);
	auto* weight=ggml_dup_tensor(ctx,normal->src[1]); ggml_backend_tensor_get(normal->src[1],weight->data,0,ggml_nbytes(weight));
	auto savedPositions=sRopePositions.find(TraceKey(rope));
	if (savedPositions==sRopePositions.end() || savedPositions->second.size()!=ggml_nbytes(rope->src[1])) { ggml_free(ctx); return false; }
	auto* positions=ggml_dup_tensor(ctx,rope->src[1]); memcpy(positions->data,savedPositions->second.data(),ggml_nbytes(positions));
	auto* norm=ggml_rms_norm(ctx,input,epsilon); auto* scaled=ggml_mul(ctx,norm,weight);
	auto* result=ggml_dup_tensor(ctx,rope); result->op=GGML_OP_ROPE; result->src[0]=scaled; result->src[1]=positions;
	memcpy(result->op_params,rope->op_params,sizeof(result->op_params));
	auto* graph=ggml_new_graph(ctx); ggml_build_forward_expand(graph,result);
	auto device=ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
	auto cpu=device?ggml_backend_dev_init(device,NULL):NULL;
	auto projection=sReference.find(tensorName);
	bool passed=cpu && projection!=sReference.end() && projection->second.output.size()==elements;
	if (passed) {
		memcpy(input->data,projection->second.output.data(),elements*4);
		passed=ggml_backend_graph_compute(cpu,graph)==GGML_STATUS_SUCCESS
			&& !memcmp(result->data,reference->data(),elements*4);
		fprintf(stderr,"POST-ROPE CPU clone control %s: %s; independent reference and device output unchanged\n",name.c_str(),passed?"bit-identical":"STOP mismatch");
		if (!passed) {
			unsigned changed=0; double error=0,norm=0;
			for (size_t i=0;i<elements;i++) { float a=((float*)result->data)[i],r=(*reference)[i]; changed+=memcmp(&a,&r,4)!=0; double d=double(a)-r; error+=d*d; norm+=double(r)*r; }
			fprintf(stderr,"CPU CLONE diagnostic eps %.9g shape %lld/%lld position0 %d changed %u/%zu L2 %.9g\n",epsilon,(long long)input->ne[0],(long long)input->ne[1],((int32_t*)positions->data)[0],changed,elements,sqrt(error/(norm+1e-30)));
		}
	}
	if (passed) {
		memcpy(input->data,actual,elements*4);
		passed=ggml_backend_graph_compute(cpu,graph)==GGML_STATUS_SUCCESS;
	}
	if (passed) {
		unsigned halfChanged=0,changed=0; double error2=0,norm2=0;
		for (size_t i=0;i<elements;i++) {
			float a=((float*)result->data)[i],r=(*reference)[i]; uint16_t ah=ggml_fp32_to_fp16(a),rh=ggml_fp32_to_fp16(r);
			if (ah!=rh && halfChanged<4) fprintf(stderr,"POST-ROPE HALF %s index%zu source[%s] %.9g/%04x reference %.9g/%04x\n",name.c_str(),i,source,a,unsigned(ah),r,unsigned(rh));
			halfChanged+=ah!=rh; changed+=memcmp(&a,&r,4)!=0;
			double difference=double(a)-r; error2+=difference*difference; norm2+=double(r)*r;
			passed &= isfinite(a) && isfinite(r);
		}
		double relative=sqrt(error2/(norm2+1e-30)); passed &= isfinite(relative) && relative<.02;
		fprintf(stderr,"POST-ROPE REPLAY %s %s + private CPU norm/RoPE, changed %u/%zu half changes %u L2 %.9g; observer only, not complete attention/inference\n",name.c_str(),source,changed,elements,halfChanged,relative);
	}
	if (cpu) ggml_backend_free(cpu);
	ggml_free(ctx); return passed;
}
bool PioneerGgmlReplayCpuRope(const char* tensorName) {
	unsigned layer=0; char which=0;
	if (sscanf(tensorName,"blk.%u.attn_%c.weight",&layer,&which)!=2 || (which!='k' && which!='q')) return false;
	auto projection=sReference.find(tensorName);
	return !sActive && projection!=sReference.end() && ReplayPostRope(tensorName,projection->second.output.data(),projection->second.output.size(),"CPU-only control projection");
}
// Offline prediction from public diagnostic data only. Never returns a
// simulated vector to a real model graph, cache or device output buffer.
static bool RoundingReference(const char* name,const float* card) {
	auto found=sReference.find(name);
	if (found==sReference.end() || !found->second.weights) return false;
	const auto& ref=found->second; auto* w=ref.weights;
	unsigned k=unsigned(w->ne[0]),n=unsigned(w->ne[1]);
	if (w->type!=GGML_TYPE_Q8_0 || k%32 || ref.input.size()!=k || ref.output.size()!=n) return false;
	std::vector<uint8_t> input(k/32*34);
	ggml_get_type_traits(GGML_TYPE_Q8_0)->from_float_ref(ref.input.data(),input.data(),k);
	std::vector<float> ieee(n),partial(n),accurate(n); unsigned ieeeDiff=0,partialDiff=0,cardDiff=0,accurateDiff=0;
	for (unsigned r=0;r<n;r++) {
		float a=0,b=0,c=0;
		for (unsigned block=0;block<k/32;block++) {
			const auto* wb=(const uint8_t*)w->data+r*w->nb[1]+block*34; const auto* xb=input.data()+block*34;
			int dot=0;
			for (unsigned i=2;i<34;i++) { int x=wb[i],y=xb[i]; if (x>=128) x-=256; if (y>=128) y-=256; dot+=x*y; }
			float dw=PioneerHalfFloat(uint16_t(wb[0])|(uint16_t(wb[1])<<8)),dx=PioneerHalfFloat(uint16_t(xb[0])|(uint16_t(xb[1])<<8));
			float scale=dw*dx;
			a=fmaf(float(dot),scale,a); b=PioneerBlackholeFma(float(dot),scale,b);
			c=PioneerBlackholeAccurateFma(float(dot),scale,c);
		}
		ieee[r]=a; partial[r]=b; accurate[r]=c;
		ieeeDiff+=PioneerFmaBits(a)!=PioneerFmaBits(ref.output[r]);
		partialDiff+=PioneerFmaBits(b)!=PioneerFmaBits(ref.output[r]);
		accurateDiff+=PioneerFmaBits(c)!=PioneerFmaBits(ref.output[r]);
		if (card && PioneerFmaBits(b)!=PioneerFmaBits(card[r])) {
			if (cardDiff<4) fprintf(stderr,"ROUNDING MODEL card mismatch row%u model %08x actual %08x\n",r,PioneerFmaBits(b),PioneerFmaBits(card[r]));
			cardDiff++;
		}
		if (!isfinite(a) || !isfinite(b) || !isfinite(c)) return false;
	}
	fprintf(stderr,"ROUNDING MODEL %s IEEE vs CPU %u/%u, partial-SFPU vs CPU %u/%u, partial-SFPU vs actual card %s %u; OFFLINE predictions, no replacement/fallback\n",name,ieeeDiff,n,partialDiff,n,card?"mismatches":"NOT MEASURED",cardDiff);
	fprintf(stderr,"ROUNDING CANDIDATE split-product/round-odd partial-SFPU sequence vs CPU mismatches %u/%u; CPU-only prediction, NO device kernel qualification\n",accurateDiff,n);
	return !ieeeDiff && !accurateDiff && (!card || !cardDiff)
		&& ReplayPostRope(name,ieee.data(),n,"CPU-only IEEE simulated projection")
		&& ReplayPostRope(name,partial.data(),n,"CPU-only partial-SFPU simulated projection")
		&& ReplayPostRope(name,accurate.data(),n,"CPU-only accurate-SFPU candidate simulated projection");
}
bool PioneerGgmlReplayRoundingReference(const char* name) { return !sActive && RoundingReference(name,NULL); }
bool PioneerGgmlReplayReference(const char* tensorName,bool roundingModel) {
	auto found=sReference.find(tensorName);
	if (sActive || found==sReference.end() || !found->second.weights) return false;
	const auto& reference=found->second; const ggml_tensor* source=reference.weights;
	if (reference.input.size()!=size_t(source->ne[0]) || reference.output.size()!=size_t(source->ne[1])) return false;
	ggml_init_params params={1024*1024,NULL,true}; ggml_context* ctx=ggml_init(params); if (!ctx) return false;
	auto* weights=ggml_dup_tensor(ctx,source); weights->data=source->data; weights->buffer=source->buffer;
	ggml_set_name(weights,source->name);
	auto* input=ggml_new_tensor_2d(ctx,GGML_TYPE_F32,source->ne[0],1);
	auto* result=ggml_mul_mat(ctx,weights,input); result->flags|=GGML_TENSOR_FLAG_COMPUTE;
	auto inputBuffer=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(input));
	auto outputBuffer=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(result));
	auto backend=ggml_backend_dev_init(ggml_backend_reg_dev_get(PioneerGgmlReg(),0),NULL);
	bool passed=inputBuffer && outputBuffer && backend;
	if (passed) {
		input->buffer=inputBuffer; input->data=ggml_backend_buffer_get_base(inputBuffer);
		result->buffer=outputBuffer; result->data=ggml_backend_buffer_get_base(outputBuffer);
		memcpy(input->data,reference.input.data(),reference.input.size()*sizeof(float));
		for (size_t i=0;i<reference.output.size();i++) ((float*)result->data)[i]=NAN;
		auto* graph=ggml_new_graph(ctx); ggml_build_forward_expand(graph,result);
		passed=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		unsigned changed=0,halfChanged=0; uint32_t maxUlp=0; double error2=0,norm2=0;
		for (size_t i=0;i<reference.output.size();i++) {
			float a=((float*)result->data)[i],r=reference.output[i]; uint32_t ab,rb; memcpy(&ab,&a,4); memcpy(&rb,&r,4);
			changed+=ab!=rb; halfChanged+=ggml_fp32_to_fp16(a)!=ggml_fp32_to_fp16(r);
			ab=(ab&0x80000000)?~ab:ab^0x80000000; rb=(rb&0x80000000)?~rb:rb^0x80000000;
			uint32_t distance=ab>rb?ab-rb:rb-ab; if (distance>maxUlp) maxUlp=distance;
			double difference=double(a)-r; error2+=difference*difference; norm2+=double(r)*r;
			passed &= isfinite(a) && isfinite(r);
		}
		double relative=sqrt(error2/(norm2+1e-30)); passed &= isfinite(relative) && relative<.02;
		fprintf(stderr,"FOCUSED REPLAY %s [%lld,%lld] original CPU input, changed FP32 %u/%zu max ULP %u half-rounding changes %u full-output L2 %.9g; numerical gates unchanged; NOT full inference/timing\n",
			tensorName,(long long)source->ne[0],(long long)source->ne[1],changed,reference.output.size(),maxUlp,halfChanged,relative);
		if (sExact) { passed &= changed==0; fprintf(stderr,"EXACT SFPU focused gate: all output FP32 bits must match independent CPU; %s\n",changed?"FAIL":"PASS"); }
		if (passed) passed=ReplayPostRope(tensorName,(const float*)result->data,reference.output.size());
		if (passed && roundingModel) passed=RoundingReference(tensorName,(const float*)result->data);
	}
	bool clean=PioneerGgmlFinish(); passed &= clean;
	if (backend) ggml_backend_free(backend);
	if (inputBuffer) ggml_backend_buffer_free(inputBuffer);
	if (outputBuffer) ggml_backend_buffer_free(outputBuffer);
	ggml_free(ctx); return passed;
}
bool PioneerGgmlCompareTap(ggml_tensor* t,bool ask,void*) {
	bool wanted=TraceTensor(t);
	if (ask) return wanted;
	if (!wanted) return true;
	auto found=sIntermediate.find(TraceKey(t));
	if (found==sIntermediate.end() || found->second.size()!=size_t(ggml_nelements(t))) {
		fprintf(stderr,"TRACE pos%u %s reference missing/shape changed; stopping diagnostic\n",sPosition,t->name); return false;
	}
	std::vector<float> actual(found->second.size()); ggml_backend_tensor_get(t,actual.data(),0,actual.size()*sizeof(float));
	double norm=0,delta=0; float maximum=0; unsigned changed=0,nonfinite=0;
	for (size_t i=0;i<actual.size();i++) {
		float a=actual[i],r=found->second[i]; changed+=memcmp(&a,&r,4)!=0;
		if (!isfinite(a) || !isfinite(r)) { nonfinite++; continue; }
		double difference=double(a)-r; delta+=difference*difference; norm+=double(r)*r;
		float absolute=fabsf(a-r); if (absolute>maximum) maximum=absolute;
	}
	fprintf(stderr,"TRACE pos%u %s op%d elements %zu changed %u nonfinite %u relative-L2 %.9g max-abs %.9g; observer only, output unchanged\n",sPosition,t->name,int(t->op),actual.size(),changed,nonfinite,sqrt(delta/(norm+1e-30)),maximum);
	return true;
}
