/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FIXTURE_ONLY
#include "pioneer_qwen38_gguf_test.cpp"
#include "../PioneerQ38GgmlBackend.h"
#include "../PioneerQ38GgmlGdnContract.h"
#include "ggml-backend-impl.h"
#include <cstdio>
#include <vector>
struct Fake {
	PioneerQwen38Gguf model; bool valid=true,fail=false; unsigned calls=0,pairCalls=0,gdnCalls=0,gdnCopies=0,gdnBegins=0,gdnCommits=0,gdnAborts=0,ends=0,aborts=0;
	static bool Ready(void* p) { return static_cast<Fake*>(p)->valid; }
	static bool Multiply(void* p,unsigned tensor,const float* input,unsigned k,float* out,unsigned n) {
		auto& f=*static_cast<Fake*>(p); f.calls++;
		assert(tensor<f.model.Tensors().size() && input && k==f.model.Tensors()[tensor].inputs && n==f.model.Tensors()[tensor].outputs);
		if (f.fail) return false;
		for (unsigned r=0;r<n;r++) out[r]=float(r+1); // SYNTHETIC marker,NOT arithmetic reference
		return true;
	}
	static bool MultiplyPair(void* p,unsigned first,unsigned second,const float* input,unsigned k,
		float* firstOut,unsigned firstN,float* secondOut,unsigned secondN) {
		auto& f=*static_cast<Fake*>(p);f.pairCalls++;
		assert(first<f.model.Tensors().size() && second<f.model.Tensors().size() && first!=second && input);
		assert(k==f.model.Tensors()[first].inputs && k==f.model.Tensors()[second].inputs);
		assert(firstN==f.model.Tensors()[first].outputs && secondN==f.model.Tensors()[second].outputs);
		if(f.fail)return false;
		for(unsigned r=0;r<firstN;r++)firstOut[r]=float(r+1);
		for(unsigned r=0;r<secondN;r++)secondOut[r]=float(r+101);
		return true;
	}
	static bool Finish(void* p) { static_cast<Fake*>(p)->ends++; return true; }
	static void Abort(void* p) { static_cast<Fake*>(p)->aborts++; }
	static bool Gdn(void* p,const ggml_tensor* op) {
		auto& f=*static_cast<Fake*>(p); f.gdnCalls++;
		PioneerQ38GgmlGdnContract c; assert(PioneerQ38GgmlGdnContract::Describe(op,c));
		if(f.fail)return false;
		static_cast<float*>(op->data)[0]=123.0f; // synthetic marker,NOT GDN math
		return true;
	}
	static bool GdnStateCopy(void* p,const ggml_tensor* copy) {
		auto& f=*static_cast<Fake*>(p); f.gdnCopies++;
		PioneerQ38GgmlGdnContract c; assert(PioneerQ38GgmlGdnContract::Describe(copy->src[0]->view_src,c) && c.StateCopy(copy));
		return !f.fail; // deliberately no host memcpy: provider owns resident state
	}
	static bool GdnBegin(void* p) { static_cast<Fake*>(p)->gdnBegins++; return true; }
	static bool GdnCommit(void* p) { static_cast<Fake*>(p)->gdnCommits++; return true; }
	static void GdnAbort(void* p) { static_cast<Fake*>(p)->gdnAborts++; }
};
int main(int argc,char** argv)
{
	assert(argc==2);
	if(strcmp(argv[1],"gdn")==0 || strcmp(argv[1],"gdn-split")==0 || strcmp(argv[1],"gdn-fault")==0 || strcmp(argv[1],"gdn-partial")==0 || strcmp(argv[1],"gdn-order-fault")==0) {
		Fixture fixture; Fake fake; assert(fake.model.Open(fixture.file)); fake.fail=strcmp(argv[1],"gdn-fault")==0;
		const bool orderFault=strcmp(argv[1],"gdn-order-fault")==0;
		PioneerQ38GgmlDispatch d={&fake,&fake.model,Fake::Ready,Fake::Multiply,Fake::Finish,Fake::Abort};
		d.gdn=Fake::Gdn; d.gdn_state_copy=Fake::GdnStateCopy; d.gdn_begin_token=Fake::GdnBegin;
		d.gdn_commit_token=Fake::GdnCommit; d.gdn_abort_token=Fake::GdnAbort;
		auto reg=PioneerQ38GgmlReg(); auto dev=ggml_backend_reg_dev_get(reg,0); assert(dev);
		assert(PioneerQ38GgmlConfigure(d)); auto backend=ggml_backend_dev_init(dev,nullptr); assert(backend);
		ggml_init_params params={16*1024*1024,nullptr,true}; auto ctx=ggml_init(params); assert(ctx);
		auto q=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
		auto k=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,16,1,1);
		auto qkv=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,10240);
		auto v=ggml_view_4d(ctx,qkv,128,48,1,1,512,40960,40960,16384);
		auto g=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
		auto beta=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,1,48,1,1);
		std::vector<float> qData(128*16),kData(128*16),qkvData(10240),gData(48),betaData(48),stateData(128*128*48);
		std::vector<float> outputData(128*48+128*128*48),copyData(128*128*48);
		q->data=qData.data(); k->data=kData.data(); qkv->data=qkvData.data(); v->data=qkvData.data()+4096;
		g->data=gData.data(); beta->data=betaData.data();
		const unsigned layers=strcmp(argv[1],"gdn-partial")==0?1:PioneerQ38GgmlGdnContract::GdnLayers;
		std::vector<ggml_tensor*> copies;
		for(unsigned layer=0;layer<layers;layer++) {
			auto* state=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,128,48,1); char stateName[GGML_MAX_NAME];
			const unsigned expectedLayer=PioneerQ38GgmlGdnContract::ExpectedLayer(layer);
			const unsigned stateLayer=orderFault && layer==1?0:expectedLayer;
			std::snprintf(stateName,sizeof(stateName),"cache_s_l%u",stateLayer); ggml_set_name(state,stateName); state->data=stateData.data();
			auto* op=ggml_gated_delta_net(ctx,q,k,v,g,beta,state,1);
			auto* next=ggml_view_4d(ctx,op,128,128,48,1,512,65536,3145728,24576);
			auto* destination=ggml_new_tensor_4d(ctx,GGML_TYPE_F32,128,128,48,1);
			ggml_set_name(destination,stateName); destination->data=stateData.data();
			auto* copy=ggml_cpy(ctx,next,destination);
			op->data=outputData.data(); copy->data=copyData.data();
			op->flags|=GGML_TENSOR_FLAG_COMPUTE; copy->flags|=GGML_TENSOR_FLAG_COMPUTE; copies.push_back(copy);
		}
		auto graph=ggml_new_graph(ctx); for(auto* copy:copies) ggml_build_forward_expand(graph,copy);
		bool okay=true;
		if(strcmp(argv[1],"gdn-split")==0) {
			for(auto* copy:copies) { auto* split=ggml_new_graph(ctx); ggml_build_forward_expand(split,copy);
				if(ggml_backend_graph_compute(backend,split)!=GGML_STATUS_SUCCESS) { okay=false; break; } }
		} else okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		const bool complete=!fake.fail && !orderFault && layers==PioneerQ38GgmlGdnContract::GdnLayers;
		assert(okay==(!fake.fail && !orderFault) && fake.gdnCalls==((fake.fail||orderFault)?1:layers)
			&& fake.gdnCopies==(fake.fail?0:((orderFault||layers==1)?1:layers)));
		assert(fake.gdnBegins==1 && fake.gdnCommits==unsigned(complete));
		assert(PioneerQ38GgmlSnapshot().gdnOps==(fake.fail?0:(orderFault?1:layers))
			&& PioneerQ38GgmlSnapshot().gdnStateCopies==(fake.fail?0:((orderFault||layers==1)?1:layers))
			&& PioneerQ38GgmlSnapshot().gdnTokensCommitted==fake.gdnCommits);
		if(fake.fail) assert(PioneerQ38GgmlSnapshot().failed && fake.aborts==1 && fake.gdnAborts==1 && !PioneerQ38GgmlFinish());
		else if(!complete) assert(!PioneerQ38GgmlFinish() && fake.gdnAborts==1 && fake.aborts==1);
		else { assert(outputData[0]==123.0f && PioneerQ38GgmlFinish() && fake.ends==1 && !fake.gdnAborts); }
		ggml_backend_free(backend); ggml_free(ctx);
		puts(fake.fail?"PASS GDN provider failure aborts token and backend session;SYNTHETIC DISPATCH,NO DEVICE":
			orderFault?"PASS out-of-order GDN layers abort before dispatch;SYNTHETIC DISPATCH,NO DEVICE":
			!complete?"PASS incomplete GDN token remains uncommitted and Finish aborts it;SYNTHETIC DISPATCH,NO DEVICE":
			strcmp(argv[1],"gdn-split")==0?"PASS 48 GDN layer transactions span backend graph splits and commit once;SYNTHETIC DISPATCH,NO DEVICE":
			"PASS 48 GDN+resident-state-CPY pairs commit once;SYNTHETIC MARKER ONLY,NO GDN MATH/DEVICE/RESIDENCY CLAIM");
		return 0;
	}
	if(strcmp(argv[1],"pair")==0 || strcmp(argv[1],"pair-fault")==0 || strcmp(argv[1],"pair-qkv")==0
		|| strcmp(argv[1],"pair-delta")==0 || strcmp(argv[1],"pair-attention")==0) {
#ifndef PIONEER_Q38_WORK_BATCH_CLIENT
		assert(false && "pair test requires opt-in Work batch client build");
#else
		const bool pairFault=strcmp(argv[1],"pair-fault")==0;
		Fixture fixture;Fake fake;assert(fake.model.Open(fixture.file));
		fake.fail=pairFault;
		PioneerQ38GgmlDispatch d={&fake,&fake.model,Fake::Ready,Fake::Multiply,Fake::Finish,Fake::Abort};
		d.multiply_pair=Fake::MultiplyPair;
		auto reg=PioneerQ38GgmlReg();auto dev=ggml_backend_reg_dev_get(reg,0);assert(dev);
		assert(PioneerQ38GgmlConfigure(d));auto backend=ggml_backend_dev_init(dev,nullptr);assert(backend);
		ggml_init_params params={1024*1024,nullptr,true};auto ctx=ggml_init(params);assert(ctx);
		const char* firstName="blk.0.ffn_up.weight";const char* secondName="blk.0.ffn_gate.weight";
		if(strcmp(argv[1],"pair-qkv")==0){firstName="blk.0.attn_qkv.weight";secondName="blk.0.attn_gate.weight";}
		if(strcmp(argv[1],"pair-delta")==0){firstName="blk.0.ssm_alpha.weight";secondName="blk.0.ssm_beta.weight";}
		if(strcmp(argv[1],"pair-attention")==0){firstName="blk.3.attn_q.weight";secondName="blk.3.attn_k.weight";}
		const auto* first=fake.model.Find(firstName);const auto* second=fake.model.Find(secondName);assert(first&&second);
		ggml_backend_buffer_t upWB=nullptr,gateWB=nullptr,upYB=nullptr,gateYB=nullptr,xB=nullptr;
		auto make=[&](const PioneerQ38Tensor* t,ggml_tensor*& weight,ggml_tensor*& output,ggml_backend_buffer_t& wb,ggml_backend_buffer_t& xb) {
			weight=ggml_new_tensor_2d(ctx,ggml_type(t->type),int64_t(t->inputs),int64_t(t->outputs));ggml_set_name(weight,t->name.c_str());
			auto x=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,int64_t(t->inputs));
			output=ggml_mul_mat(ctx,weight,x);wb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(weight));assert(wb);
			weight->buffer=wb;weight->data=ggml_backend_buffer_get_base(wb);ggml_backend_buffer_set_usage(wb,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
			xb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(x));assert(xb);x->buffer=xb;x->data=ggml_backend_buffer_get_base(xb);memset(x->data,0,ggml_nbytes(x));
			return x;
		};
		ggml_tensor *firstW=nullptr,*secondW=nullptr,*firstY=nullptr,*secondY=nullptr;
		auto* input=make(first,firstW,firstY,upWB,xB);
		secondW=ggml_new_tensor_2d(ctx,ggml_type(second->type),int64_t(second->inputs),int64_t(second->outputs));ggml_set_name(secondW,second->name.c_str());
		secondY=ggml_mul_mat(ctx,secondW,input);gateWB=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(secondW));assert(gateWB);
		secondW->buffer=gateWB;secondW->data=ggml_backend_buffer_get_base(gateWB);ggml_backend_buffer_set_usage(gateWB,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
		gateYB=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(secondY));assert(gateYB);secondY->buffer=gateYB;secondY->data=ggml_backend_buffer_get_base(gateYB);
		upYB=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(firstY));assert(upYB);firstY->buffer=upYB;firstY->data=ggml_backend_buffer_get_base(upYB);
		memset(firstY->data,0xff,ggml_nbytes(firstY));memset(secondY->data,0xff,ggml_nbytes(secondY));
		firstY->flags|=GGML_TENSOR_FLAG_COMPUTE;secondY->flags|=GGML_TENSOR_FLAG_COMPUTE;
		auto graph=ggml_new_graph(ctx);ggml_build_forward_expand(graph,firstY);ggml_build_forward_expand(graph,secondY);
		const bool graphOkay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		assert(graphOkay==!pairFault && fake.pairCalls==1 && !fake.calls);
		if(pairFault){
			for(size_t i=0;i<ggml_nbytes(firstY)/4;i++)assert(static_cast<uint32_t*>(firstY->data)[i]==UINT32_MAX);
			for(size_t i=0;i<ggml_nbytes(secondY)/4;i++)assert(static_cast<uint32_t*>(secondY->data)[i]==UINT32_MAX);
			assert(PioneerQ38GgmlSnapshot().failed && !PioneerQ38GgmlSnapshot().projections);
			assert(!PioneerQ38GgmlFinish() && !fake.ends && fake.aborts==1);
		}else{
			assert(PioneerQ38GgmlSnapshot().projections==2 && PioneerQ38GgmlSnapshot().pairedBatches==1);
			for(unsigned r=0;r<first->outputs;r++)assert(static_cast<float*>(firstY->data)[r]==float(r+1));
			for(unsigned r=0;r<second->outputs;r++)assert(static_cast<float*>(secondY->data)[r]==float(r+101));
			assert(PioneerQ38GgmlFinish());assert(fake.ends==1 && !fake.aborts);
		}
		ggml_backend_buffer_free(gateYB);ggml_backend_buffer_free(upYB);ggml_backend_buffer_free(gateWB);ggml_backend_buffer_free(upWB);
		ggml_backend_buffer_free(xB);ggml_backend_free(backend);ggml_free(ctx);
		puts(pairFault?"PASS opt-in same-input FFN pair failure: both caller outputs untouched, graph fails and owner aborts;SYNTHETIC DISPATCH,NO DEVICE"
			:"PASS opt-in same-input FFN pair dispatch: exact two weights, same activation, both outputs, atomic graph completion;SYNTHETIC DISPATCH,NO DEVICE");
#endif
		return 0;
	}
	unsigned fault=unsigned(atoi(argv[1])); assert(fault<=4);
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
	assert(PioneerQ38GgmlSnapshot().recognizedWeights==(fault?1U:3U) && !PioneerQ38GgmlSnapshot().pairedBatches);
	assert(PioneerQ38GgmlFinish()==!fault);
	auto stats=PioneerQ38GgmlSnapshot(); assert(stats.finished && stats.failed==bool(fault) && stats.clean==!fault);
	assert(fake.ends==unsigned(!fault) && fake.aborts==unsigned(bool(fault)));
	assert(PioneerQ38GgmlFinish()==!fault && !ggml_backend_dev_init(dev,nullptr));
	ggml_backend_free(backend); assert(fake.calls==calls && fake.ends==unsigned(!fault) && fake.aborts==unsigned(bool(fault)));
	ggml_free(ctx);
	printf("PASS OFFLINE ggml27B bridge fault%u:exact named Q8/Q4/Q6 matrices,host weights/batch/stride contracts,GDN/state-CPY refused and views remain host-only metadata,exclusive one-shot context,source/epoch binding,atomic session failure,no CPU products/fallback,repeated Finish/free;SYNTHETIC DISPATCH,NOT native model inference\n",fault);
}
