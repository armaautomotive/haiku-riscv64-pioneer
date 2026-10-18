/* SPDX-License-Identifier: MIT */
// Separate PUBLIC prompt smoke only. Never installs/updates Assistant.
#include "TenstorrentHaikuQwen38.h"
#include "PioneerQ38GgmlBackend.h"
#include "llama.h"
#include <time.h>
static double Seconds() { timespec t={}; if (clock_gettime(CLOCK_MONOTONIC,&t)) return -1; return double(t.tv_sec)+double(t.tv_nsec)*1e-9; }
static void Log(ggml_log_level level,const char* text,void*) { if (level>=GGML_LOG_LEVEL_WARN) fputs(text,stderr); }
struct State {
	PioneerQ38PublicModelSource source;
	PioneerQ38NativeTransport io;
	PioneerQ38NativeSession session;
	bool opened=false;
	State():session(io,source) {}
	static bool Ready(void* p) { auto& s=*static_cast<State*>(p); return s.source.Ready() && !s.session.Failed(); }
	static bool Multiply(void* p,unsigned tensor,const float* x,unsigned k,float* y,unsigned n) {
		auto& s=*static_cast<State*>(p);
		if (!s.opened) { if (!s.session.Open()) return false; s.opened=true; }
		uint64_t before=s.session.pagesUploaded;
		uint64_t readBefore=s.session.sourceReadNanos,bulkBefore=s.session.bulkUploadNanos,runBefore=s.session.runNanos;
		double start=Seconds(); bool okay=s.session.Multiply(tensor,x,k,y,n);
		if (before!=s.session.pagesUploaded || !okay) printf("PUBLIC_MATRIX %s K%u N%u seconds%.6f pages%llu calls%llu success%u;NOT_TOKEN_RATE\n",
			s.source.Model().Tensors()[tensor].name.c_str(),k,n,Seconds()-start,(unsigned long long)s.session.pagesUploaded,
			(unsigned long long)s.session.calls,unsigned(okay));
		if (before!=s.session.pagesUploaded || !okay) printf(" PUBLIC_MATRIX_PHASES source_repack_seconds%.6f checked_bulk_seconds%.6f checked_run_seconds%.6f;DIAGNOSTIC_HOST_TIMES_NOT_PURE_GPU\n",
			double(s.session.sourceReadNanos-readBefore)*1e-9,double(s.session.bulkUploadNanos-bulkBefore)*1e-9,double(s.session.runNanos-runBefore)*1e-9);
		return okay;
	}
	static bool Finish(void* p) { return static_cast<State*>(p)->session.Finish(); }
	static void Abort(void* p) { static_cast<State*>(p)->session.Abort(); }
};
static int GraphSmoke(State& state,const char* output)
{
	const auto* t=state.source.Model().Find("blk.0.ffn_down.weight");
	if (!t || t->type!=12 || t->inputs!=17408 || t->outputs!=5120) return 1;
	PioneerQ38GgmlDispatch d={&state,&state.source.Model(),State::Ready,State::Multiply,State::Finish,State::Abort};
	if (!PioneerQ38GgmlConfigure(d)) return 1;
	auto dev=ggml_backend_reg_dev_get(PioneerQ38GgmlReg(),0); auto backend=ggml_backend_dev_init(dev,nullptr);
	ggml_init_params p={1024*1024,nullptr,true}; auto ctx=ggml_init(p);
	if (!backend || !ctx) { if (backend) ggml_backend_free(backend); if (ctx) ggml_free(ctx); return 1; }
	auto w=ggml_new_tensor_2d(ctx,GGML_TYPE_Q4_K,17408,5120); ggml_set_name(w,t->name.c_str());
	auto x=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,17408); auto y=ggml_mul_mat(ctx,w,x);
	auto wb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(w));
	auto xb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(x));
	auto yb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(y));
	bool okay=wb && xb && yb;
	std::vector<float> first(5120);
	if (okay) {
		// Metadata-only host weight mirror:GPU reads EXCLUSIVELY the original
		// authenticated FILE pages,NOT this deliberately uninitialized buffer.
		w->buffer=wb; w->data=ggml_backend_buffer_get_base(wb); ggml_backend_buffer_set_usage(wb,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
		x->buffer=xb; x->data=ggml_backend_buffer_get_base(xb); y->buffer=yb; y->data=ggml_backend_buffer_get_base(yb);
		for (unsigned k=0;k<17408;k++) static_cast<float*>(x->data)[k]=float(int((k*29+17)%255)-127)*.00390625f;
		auto graph=ggml_new_graph(ctx); ggml_build_forward_expand(graph,y); y->flags|=GGML_TENSOR_FLAG_COMPUTE;
		double start=Seconds(); okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		double repeat=Seconds(); uint64_t pages=state.session.pagesUploaded;
		if (okay) { memcpy(first.data(),y->data,5120*4); memset(y->data,0,5120*4); okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS; }
		printf("PUBLIC_FFN_GGML_GRAPH first_seconds%.6f repeat_seconds%.6f calls%llu projections%llu pages%llu;NOT_LLM_TOKEN_RATE\n",
			repeat-start,Seconds()-repeat,(unsigned long long)state.session.calls,(unsigned long long)PioneerQ38GgmlSnapshot().projections,
			(unsigned long long)state.session.pagesUploaded);
		okay=okay && !memcmp(first.data(),y->data,5120*4) && state.session.pagesUploaded==pages && PioneerQ38GgmlSnapshot().projections==2;
	}
	bool clean=PioneerQ38GgmlFinish(); okay=okay && clean;
	// Publish PUBLIC numeric diagnostic ONLY after clean END;never overwrite.
	if (okay) {
		int fd=open(output,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
		if (fd<0) okay=false;
		else {
			const uint8_t* bytes=reinterpret_cast<const uint8_t*>(first.data()); unsigned done=0;
			while (done<5120*4) { ssize_t n=write(fd,bytes+done,5120*4-done); if (n<0 && errno==EINTR) continue; if (n<=0) { okay=false; break; } done+=unsigned(n); }
			if (fsync(fd)) okay=false;
			if (close(fd)) okay=false;
		}
	}
	if (yb) ggml_backend_buffer_free(yb);
	if (xb) ggml_backend_buffer_free(xb);
	if (wb) ggml_backend_buffer_free(wb);
	ggml_backend_free(backend); ggml_free(ctx);
	printf("%s_PUBLIC_FFN_GGML_EXPORT clean_end%u;REPEATABILITY_NOT_INDEPENDENT_NUMERIC_REFERENCE_OR_LLM_INFERENCE\n",okay?"PASS":"FAIL",unsigned(clean));
	return okay?0:1;
}
int main(int argc,char** argv)
{
	bool graph=argc==3 && !strcmp(argv[1],"--public-ffn-graph-export");
	bool four=argc==2 && !strcmp(argv[1],"--public-four-tokens");
	if (!graph && !four && (argc!=2 || strcmp(argv[1],"--public-one-token"))) return 2;
	setvbuf(stdout,nullptr,_IOLBF,0); setvbuf(stderr,nullptr,_IOLBF,0);
	puts("PUBLIC27B_INFERENCE_AUTH_BEGIN;NO_ACCELERATOR_FD_YET;NO_CPU_REFERENCE_OR_FALLBACK");
	State state; if (!state.source.Open()) return 1;
	if (graph) return GraphSmoke(state,argv[2]);
	FILE* file=state.source.BorrowForModelLoad(); if (!file) return 1;
	llama_log_set(Log,nullptr); llama_backend_init();
	llama_model_params mp=llama_model_default_params();
	ggml_backend_dev_t hostOnly[]={nullptr}; mp.devices=hostOnly; mp.n_gpu_layers=0; mp.load_mode=LLAMA_LOAD_MODE_MMAP; mp.use_extra_bufts=false;
	double loadStart=Seconds(); auto model=llama_model_load_from_file_ptr(file,mp);
	if (!model || !state.source.Ready()) { if (model) llama_model_free(model); llama_backend_free(); return 1; }
	printf("PUBLIC27B_SAME_FD_MODEL_LOADED seconds%.6f;HOST_WEIGHTS_NOT_CARD_RESIDENCY\n",Seconds()-loadStart);
	PioneerQ38GgmlDispatch dispatch={&state,&state.source.Model(),State::Ready,State::Multiply,State::Finish,State::Abort};
	if (!PioneerQ38GgmlConfigure(dispatch)) { llama_model_free(model); llama_backend_free(); return 1; }
	ggml_backend_register(PioneerQ38GgmlReg());
	llama_context_params cp=llama_context_default_params(); cp.n_ctx=128; cp.n_batch=cp.n_ubatch=1;
	cp.n_threads=cp.n_threads_batch=32; cp.offload_kqv=false; cp.op_offload=true; cp.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_DISABLED;
	auto context=llama_init_from_model(model,cp);
	unsigned recognized=PioneerQ38GgmlSnapshot().recognizedWeights;
	printf("PUBLIC27B_GRAPH_COVERAGE recognized_learned_weights%u expected497;RESERVATION_ONLY_NOT_EXECUTION\n",recognized);
	bool okay=context && state.source.Ready() && recognized==497;
	const auto vocab=llama_model_get_vocab(model); llama_token tokens[8]={};
	int count=llama_tokenize(vocab,"Hello",5,tokens,8,false,false);
	okay=okay && count==1; // fixed PUBLIC one-position smoke,not private chat
	double start=Seconds(),warmSeconds=0,firstSeconds=0;
	unsigned generated=0,limit=four?4:1; uint64_t retainedPages=0;
	llama_token token=tokens[0],best=-1;
	for (unsigned position=0;okay && position<limit;position++) {
		uint64_t before=PioneerQ38GgmlSnapshot().projections;
		double began=Seconds(); auto batch=llama_batch_get_one(&token,1); okay=llama_decode(context,batch)==0;
		double elapsed=Seconds()-began; auto current=PioneerQ38GgmlSnapshot();
		// Exactly497 learned projections per position. Refuse to call a
		// silent CPU-only/partial-learned result a complete P100A smoke.
		okay=okay && current.projections-before==497 && !current.failed && state.opened;
		best=-1;
		if (okay) {
			const float* logits=llama_get_logits_ith(context,-1); int size=llama_vocab_n_tokens(vocab);
			for (int i=0;okay && i<size;i++) {
				if (!isfinite(logits[i])) okay=false;
				else if (best<0 || logits[i]>logits[best]) best=i;
			}
		}
		if (!position) { retainedPages=state.session.pagesUploaded; firstSeconds=elapsed; }
		else { okay=okay && retainedPages==state.session.pagesUploaded; warmSeconds+=elapsed; }
		printf("PUBLIC27B_POSITION %u selected_token%d seconds%.6f learned_projections%llu pages%llu success%u;NO_CPU_REFERENCE_OR_FALLBACK\n",
			position,best,elapsed,(unsigned long long)(current.projections-before),(unsigned long long)state.session.pagesUploaded,unsigned(okay));
		if (okay) { generated++; token=best; }
	}
	double end=Seconds(); auto stats=PioneerQ38GgmlSnapshot();
	bool clean=PioneerQ38GgmlFinish();
	printf("%s_PUBLIC27B_MODEL_SMOKE public_prompt_Hello generated%u selected_token%d seconds%.6f projections%llu uploaded_pages%llu calls%llu workers%u clean_end%u;COLD_LOAD_SMOKE_NOT_STEADY_TOKEN_RATE_OR_TEXT_QUALITY\n",
		okay && clean?"PASS":"FAIL",generated,best,end-start,(unsigned long long)stats.projections,(unsigned long long)state.session.pagesUploaded,
		(unsigned long long)state.session.calls,state.session.peakWorkers,unsigned(clean));
	if (four) printf("%s_PUBLIC27B_FOUR_TOKEN generated%u first_position_seconds%.6f subsequent_positions%u subsequent_seconds%.6f sample_tokens_per_second%.9f retained_weights%u;TINY_PUBLIC_SMOKE_NOT_STEADY_BENCHMARK_OR_TEXT_QUALITY\n",
		okay && clean && generated==4?"PASS":"FAIL",generated,firstSeconds,generated?generated-1:0,warmSeconds,
		warmSeconds>0 && generated>1?double(generated-1)/warmSeconds:0,unsigned(retainedPages && retainedPages==state.session.pagesUploaded));
	if (context) llama_free(context);
	llama_model_free(model); llama_backend_free();
	return okay && clean && end>=start?0:1;
}
