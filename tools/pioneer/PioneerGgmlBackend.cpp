/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlBackend.h"
#ifdef PIONEER_GGML_OFFLINE_TEST
#include "tests/PioneerGgmlHostDevice.h"
#else
#include "TenstorrentHaikuResident.h"
#endif
#include "ggml-backend-impl.h"
#include <new>

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
		if (!ctx->cache.Multiply(offset,inputs,outputs,(const float*)op->src[1]->data,(float*)op->data)) {
			ctx->failed=true; return GGML_STATUS_FAILED;
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
