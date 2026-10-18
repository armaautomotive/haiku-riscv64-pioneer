/* SPDX-License-Identifier: MIT */
#include "PioneerQ38GgmlBackend.h"
#include "ggml-backend-impl.h"
#include "ggml-cpu.h"
#include <new>
namespace {
PioneerQ38GgmlDispatch dispatch;
PioneerQ38GgmlStats stats;
uint64_t epoch=0;
bool configured=false,claimed=false;
bool recognized[851]={};
ggml_backend_t active=nullptr;
bool Ready() {
	return configured && !stats.failed && !stats.finished && dispatch.model->Epoch()==epoch
		&& dispatch.model->Stable() && dispatch.ready(dispatch.owner);
}
void Abort() {
	if (!stats.failed && configured) dispatch.abort(dispatch.owner);
	stats.failed=true; stats.finished=true; stats.clean=false;
}
bool View(ggml_op op) {
	return op==GGML_OP_NONE || op==GGML_OP_RESHAPE || op==GGML_OP_VIEW || op==GGML_OP_PERMUTE || op==GGML_OP_TRANSPOSE;
}
bool Contract(const ggml_tensor* op,unsigned* index=nullptr) {
	if (!op || !Ready()) return false;
	if (View(op->op)) return true;
	if (op->op!=GGML_OP_MUL_MAT || !op->src[0] || !op->src[1]) return false;
	const auto* w=op->src[0]; const auto* x=op->src[1];
	// Only exact named learned tensors from the SAME loaded public source.
	// No dynamic attention products, arbitrary equal-sized tensors, batches,
	// broadcasts, CPU-weight mutation or mixed-runtime precision conversion.
	const auto* t=dispatch.model->Find(w->name);
	if (!t || !t->type || t->rank!=2 || unsigned(w->type)!=t->type || w->ne[0]!=int64_t(t->inputs)
		|| w->ne[1]!=int64_t(t->outputs) || w->ne[2]!=1 || w->ne[3]!=1
		|| x->type!=GGML_TYPE_F32 || x->ne[0]!=w->ne[0] || x->ne[1]!=1 || x->ne[2]!=1 || x->ne[3]!=1
		|| op->type!=GGML_TYPE_F32 || op->ne[0]!=w->ne[1] || op->ne[1]!=1 || op->ne[2]!=1 || op->ne[3]!=1
		|| !ggml_is_contiguous(w) || !ggml_is_contiguous(x) || !ggml_is_contiguous(op)
		|| !w->buffer || ggml_backend_buffer_get_usage(w->buffer)!=GGML_BACKEND_BUFFER_USAGE_WEIGHTS) return false;
	unsigned selected=unsigned(t-dispatch.model->Tensors().data());
	if (selected>=851) return false;
	if (!recognized[selected]) { recognized[selected]=true; stats.recognizedWeights++; }
	if (index) *index=selected;
	return true;
}
bool Supports(ggml_backend_dev_t,const ggml_tensor* t) { return Contract(t); }
const char* Name(ggml_backend_t) { return "P100A-public27B-cohort52-experimental"; }
const char* DeviceName(ggml_backend_dev_t) { return "P100A-public27B-cohort52"; }
const char* Description(ggml_backend_dev_t) { return "Experimental authenticated mixed-quant matrices;CPU non-matrix ops;not a speed qualification"; }
ggml_status Compute(ggml_backend_t backend,ggml_cgraph* graph) {
	if (backend!=active || !Ready()) { Abort(); return GGML_STATUS_FAILED; }
	for (int i=0;i<ggml_graph_n_nodes(graph);i++) {
		auto* op=ggml_graph_node(graph,i); unsigned tensor=0;
		if (!(op->flags&GGML_TENSOR_FLAG_COMPUTE) || View(op->op)) continue;
		if (!Contract(op,&tensor) || !op->data || !op->src[0]->data || !op->src[1]->data
			|| !dispatch.multiply(dispatch.owner,tensor,static_cast<const float*>(op->src[1]->data),unsigned(op->src[1]->ne[0]),
				static_cast<float*>(op->data),unsigned(op->ne[0])) || !Ready()) {
			Abort(); return GGML_STATUS_FAILED; // NEVER return success/substitute CPU values
		}
		stats.projections++;
	}
	return GGML_STATUS_SUCCESS;
}
void Free(ggml_backend_t backend) {
	if (backend==active) { PioneerQ38GgmlFinish(); active=nullptr; }
	delete backend;
}
ggml_backend_t Init(ggml_backend_dev_t dev,const char*) {
	if (claimed || !Ready()) return nullptr;
	static ggml_guid guid={0x67,0x30,0x38,0x52,0xe2,0x34,0x4e,0x56,0xaa,0x13,0x91,0x07,0xde,0x01,0xba,0x51};
	ggml_backend_i iface={}; iface.get_name=Name; iface.free=Free; iface.graph_compute=Compute;
	auto* backend=new(std::nothrow) ggml_backend{&guid,iface,dev,nullptr};
	if (backend) { claimed=true; active=backend; }
	return backend;
}
void Memory(ggml_backend_dev_t,size_t* available,size_t* total) { *available=*total=UINT64_C(21)*1024*1024*1024; }
enum ggml_backend_dev_type Type(ggml_backend_dev_t) { return GGML_BACKEND_DEVICE_TYPE_ACCEL; }
void Props(ggml_backend_dev_t dev,ggml_backend_dev_props* p) {
	*p={}; p->name=DeviceName(dev); p->description=Description(dev); p->type=Type(dev);
	Memory(dev,&p->memory_free,&p->memory_total); p->caps.buffer_from_host_ptr=p->caps.mmap_support=true;
}
ggml_backend_buffer_type_t BufferType(ggml_backend_dev_t) { return ggml_backend_cpu_buffer_type(); }
ggml_backend_buffer_t FromHost(ggml_backend_dev_t,void* data,size_t bytes,size_t) { return ggml_backend_cpu_buffer_from_ptr(data,bytes); }
bool BufferSupported(ggml_backend_dev_t,ggml_backend_buffer_type_t type) { return ggml_backend_buft_is_host(type); }
const char* RegName(ggml_backend_reg_t) { return "Pioneer public27B cohort52 experimental"; }
size_t Count(ggml_backend_reg_t) { return 1; }
ggml_backend_dev_t Device(ggml_backend_reg_t reg,size_t index) {
	if (index) return nullptr;
	static ggml_backend_device dev={}; dev.reg=reg;
	dev.iface.get_name=DeviceName; dev.iface.get_description=Description; dev.iface.get_memory=Memory;
	dev.iface.get_type=Type; dev.iface.get_props=Props; dev.iface.init_backend=Init;
	dev.iface.get_buffer_type=BufferType; dev.iface.buffer_from_host_ptr=FromHost;
	dev.iface.supports_op=Supports; dev.iface.supports_buft=BufferSupported;
	return &dev;
}
}
bool PioneerQ38GgmlConfigure(const PioneerQ38GgmlDispatch& d) {
	if (configured || !d.owner || !d.model || !d.ready || !d.multiply || !d.finish || !d.abort
		|| !d.model->Stable() || !d.ready(d.owner)) return false;
	dispatch=d; epoch=d.model->Epoch(); configured=true; return true;
}
ggml_backend_reg_t PioneerQ38GgmlReg() {
	static ggml_backend_reg reg={GGML_BACKEND_API_VERSION,{RegName,Count,Device,nullptr},nullptr}; return &reg;
}
bool PioneerQ38GgmlFinish() {
	if (stats.finished) return stats.clean && !stats.failed;
	if (!configured) return false;
	if (!Ready() || !dispatch.finish(dispatch.owner)) { Abort(); return false; }
	stats.finished=true; stats.clean=true; return true;
}
PioneerQ38GgmlStats PioneerQ38GgmlSnapshot() { return stats; }
