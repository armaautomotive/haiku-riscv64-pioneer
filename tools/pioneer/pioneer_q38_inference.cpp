/* SPDX-License-Identifier: MIT */
// Separate PUBLIC prompt smoke only. Never installs/updates Assistant.
#include "TenstorrentHaikuQwen38.h"
#include "PioneerQ38GgmlBackend.h"
#include "PioneerQ38PublicTiming.h"
#include "llama.h"
#include <time.h>
#include <signal.h>
#ifdef PIONEER_Q38_MODEL_NODE_PROFILE
#include "PioneerQ38ModelNodeProfile.h"
#endif
#ifdef PIONEER_Q38_FFN_REPEAT_BENCH
#include "PioneerQ38FfnSamples.h"
#endif
// PIONEER_PUBLIC_STOP_BEGIN
static volatile sig_atomic_t stopRequested=0;
static void RequestStop(int) { stopRequested=1; } // NO IO/cleanup in a signal handler
// PIONEER_PUBLIC_STOP_END
static double Seconds() { timespec t={}; if (clock_gettime(CLOCK_MONOTONIC,&t)) return -1; return double(t.tv_sec)+double(t.tv_nsec)*1e-9; }
#ifdef PIONEER_Q38_MODEL_NODE_PROFILE
static uint64_t NodeNanos(){
 timespec t={};if(clock_gettime(CLOCK_MONOTONIC,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000
  ||uint64_t(t.tv_sec)>(UINT64_MAX-uint64_t(t.tv_nsec))/1000000000)return UINT64_MAX;
 return uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec);
}
// PIONEER_MODEL_NODE_OBSERVER_BEGIN
static bool NodeObserver(ggml_tensor* t,bool ask,void* opaque){
 // ALWAYS true after execution:even malformed metrics CANNOT cancel a
 // graph. Outside the measured run,ask=false preserves normal batching.
 auto* profile=static_cast<PioneerQ38ModelNodeProfile*>(opaque);
 if(!profile||!profile->Active())return !ask;
 if(ask)profile->Ask(t,t?unsigned(t->op):UINT32_MAX,NodeNanos());
 else profile->After(t,NodeNanos());
 return true;
}
// PIONEER_MODEL_NODE_OBSERVER_END
#endif
static void Log(ggml_log_level level,const char* text,void*) { if (level>=GGML_LOG_LEVEL_WARN) fputs(text,stderr); }
struct State {
	PioneerQ38PublicModelSource source;
	PioneerQ38NativeTransport io;
	PioneerQ38NativeSession session;
	bool opened=false;
	double matrixWallSeconds=0;
	uint64_t matrixCallbacks=0;
	bool profileValid=true;
#ifdef PIONEER_Q38_SPAN_PRELOAD
	bool singleFfnSpan=false;
#endif
	State():session(io,source) {}
	static bool Ready(void* p) { auto& s=*static_cast<State*>(p); return s.source.Ready() && !s.session.Failed(); }
	static bool Multiply(void* p,unsigned tensor,const float* x,unsigned k,float* y,unsigned n) {
		auto& s=*static_cast<State*>(p);
		if (!s.opened) {
			if (!s.session.Open()) return false;
			s.opened=true;
#ifdef PIONEER_Q38_SPAN_PRELOAD
			double preloadStart=Seconds();
			if (!s.session.PrimeSpan(s.singleFfnSpan?tensor:UINT32_MAX)) return false;
			printf("PUBLIC_PRECOHORT_SPAN_PRIMED pages%llu bytes%llu seconds%.6f source_repack_seconds%.6f span_upload_seconds%.6f;CODE_RESTORED_BEFORE_ANY_COHORT_RUN\n",
				(unsigned long long)s.session.pagesUploaded,(unsigned long long)s.session.bytesRetained,Seconds()-preloadStart,
				double(s.session.sourceReadNanos)*1e-9,double(s.session.bulkUploadNanos)*1e-9);
#endif
		}
		uint64_t before=s.session.pagesUploaded;
		uint64_t readBefore=s.session.sourceReadNanos,bulkBefore=s.session.bulkUploadNanos,runBefore=s.session.runNanos;
		double start=Seconds(); bool okay=s.session.Multiply(tensor,x,k,y,n);
		double finish=Seconds(); s.matrixCallbacks++;
		if (start>=0 && finish>=start) s.matrixWallSeconds+=finish-start;
		else s.profileValid=false; // diagnostics ONLY,not completion/timeout authority
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
#ifdef PIONEER_Q38_Q8_REPEAT_BENCH
#define PIONEER_Q38_GRAPH_LABEL "Q8"
	constexpr unsigned graphInputs=5120,graphOutputs=10240,packedType=8;
	constexpr auto graphQuantType=GGML_TYPE_Q8_0;
	const auto* t=state.source.Model().Find("blk.0.attn_qkv.weight");
#else
#define PIONEER_Q38_GRAPH_LABEL "FFN"
	constexpr unsigned graphInputs=17408,graphOutputs=5120,packedType=12;
	constexpr auto graphQuantType=GGML_TYPE_Q4_K;
	const auto* t=state.source.Model().Find("blk.0.ffn_down.weight");
#endif
	if (!t || t->type!=packedType || t->inputs!=graphInputs || t->outputs!=graphOutputs) return 1;
	PioneerQ38GgmlDispatch d={&state,&state.source.Model(),State::Ready,State::Multiply,State::Finish,State::Abort};
#ifdef PIONEER_Q38_SPAN_PRELOAD
	state.singleFfnSpan=true;
#endif
	if (!PioneerQ38GgmlConfigure(d)) return 1;
	auto dev=ggml_backend_reg_dev_get(PioneerQ38GgmlReg(),0); auto backend=ggml_backend_dev_init(dev,nullptr);
	ggml_init_params p={1024*1024,nullptr,true}; auto ctx=ggml_init(p);
	if (!backend || !ctx) { if (backend) ggml_backend_free(backend); if (ctx) ggml_free(ctx); return 1; }
	auto w=ggml_new_tensor_2d(ctx,graphQuantType,graphInputs,graphOutputs); ggml_set_name(w,t->name.c_str());
	auto x=ggml_new_tensor_1d(ctx,GGML_TYPE_F32,graphInputs); auto y=ggml_mul_mat(ctx,w,x);
	auto wb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(w));
	auto xb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(x));
	auto yb=ggml_backend_buft_alloc_buffer(ggml_backend_cpu_buffer_type(),ggml_nbytes(y));
	bool okay=wb && xb && yb;
	std::vector<float> first(graphOutputs);
	if (okay) {
		// Metadata-only host weight mirror:GPU reads EXCLUSIVELY the original
		// authenticated FILE pages,NOT this deliberately uninitialized buffer.
		w->buffer=wb; w->data=ggml_backend_buffer_get_base(wb); ggml_backend_buffer_set_usage(wb,GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
		x->buffer=xb; x->data=ggml_backend_buffer_get_base(xb); y->buffer=yb; y->data=ggml_backend_buffer_get_base(yb);
		for (unsigned k=0;k<graphInputs;k++) static_cast<float*>(x->data)[k]=float(int((k*29+17)%255)-127)*.00390625f;
		auto graph=ggml_new_graph(ctx); ggml_build_forward_expand(graph,y); y->flags|=GGML_TENSOR_FLAG_COMPUTE;
		double start=Seconds(); okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
		double repeat=Seconds(); uint64_t pages=state.session.pagesUploaded;
		double matrixBefore=state.matrixWallSeconds;
		uint64_t ioctlBefore=state.session.runNanos;
#ifdef PIONEER_Q38_ROOT_STATS_BENCH
		state.session.rootStats.Reset(); // host metrics ONLY;exclude first graph/priming
#endif
#ifdef PIONEER_Q38_FFN_REPEAT_BENCH
		PioneerQ38FfnSamples samples;
		if (okay) memcpy(first.data(),y->data,graphOutputs*4);
		// PIONEER_FFN_REPEAT_LOOP_BEGIN
		for (unsigned sample=0;okay && !stopRequested && sample<32;sample++) {
			memset(y->data,0,graphOutputs*4); double began=Seconds();
			okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS;
			double finished=Seconds(); samples.Add(began,finished); // timing NEVER controls device cleanup
			if (okay) okay=!memcmp(first.data(),y->data,graphOutputs*4); // repeatability OUTSIDE timed graph;NO CPU learned reference
		}
		// PIONEER_FFN_REPEAT_LOOP_END
#else
		if (okay) { memcpy(first.data(),y->data,graphOutputs*4); memset(y->data,0,graphOutputs*4); okay=ggml_backend_graph_compute(backend,graph)==GGML_STATUS_SUCCESS; }
#endif
		double repeated=Seconds()-repeat;
#ifdef PIONEER_Q38_FFN_REPEAT_BENCH
		PioneerQ38FfnSamples::Summary summary; bool timing=samples.Finish(summary);
		printf("PUBLIC_" PIONEER_Q38_GRAPH_LABEL "_REPEAT32 samples%u timing_valid%u graph_success%u mean_seconds%.9f median_seconds%.9f min_seconds%.9f max_seconds%.9f first_seconds%.6f warm_wall_seconds%.6f;REPEATABILITY_OUTSIDE_TIMING_NO_CPU_LEARNED_REFERENCE_NOT_TOKEN_RATE\n",
			samples.Count(),unsigned(timing),unsigned(okay),summary.mean,summary.median,summary.minimum,summary.maximum,repeat-start,repeated);
		printf("PUBLIC_" PIONEER_Q38_GRAPH_LABEL "_HOST_REPEAT32 graph_wall_seconds%.6f multiply_seconds%.6f ioctl_seconds%.6f profile_valid%u calls%llu projections%llu pages%llu;CPU_WALL_NOT_PURE_GPU_OR_LLM_TOKEN_RATE\n",
			repeated,state.matrixWallSeconds-matrixBefore,double(state.session.runNanos-ioctlBefore)*1e-9,unsigned(state.profileValid),
			(unsigned long long)state.session.calls,(unsigned long long)PioneerQ38GgmlSnapshot().projections,(unsigned long long)state.session.pagesUploaded);
		okay=okay && state.session.pagesUploaded==pages && PioneerQ38GgmlSnapshot().projections==1+samples.Count();
#ifdef PIONEER_Q38_ROOT_STATS_BENCH
		const auto& root=state.session.rootStats;
		printf("PUBLIC_" PIONEER_Q38_GRAPH_LABEL "_ROOT_HOST_COPY_PROFILE samples%llu data_valid%u prepare_cycles%llu wait_audit_cycles%llu gather_dma_cycles%llu read_packets%llu write_packets%llu read_flits%llu write_flits%llu peer_polls%llu terminal_audits%llu;ALREADY_RETURNED_CPU_MEMORY_RAW_COUNTERS_NOT_PURE_ARITHMETIC_OR_TOKEN_SPEED_NO_ADDITIONAL_CARD_IO\n",
			(unsigned long long)root.samples,unsigned(root.valid),(unsigned long long)root.cycles[0],
			(unsigned long long)root.cycles[1],(unsigned long long)root.cycles[2],
			(unsigned long long)root.packets[0],(unsigned long long)root.packets[1],
			(unsigned long long)root.packets[2],(unsigned long long)root.packets[3],
			(unsigned long long)root.packets[4],(unsigned long long)root.packets[5]);
#endif
#else
		printf("PUBLIC_" PIONEER_Q38_GRAPH_LABEL "_GGML_GRAPH first_seconds%.6f repeat_seconds%.6f calls%llu projections%llu pages%llu;NOT_LLM_TOKEN_RATE\n",
			repeat-start,repeated,(unsigned long long)state.session.calls,(unsigned long long)PioneerQ38GgmlSnapshot().projections,
			(unsigned long long)state.session.pagesUploaded);
		printf("PUBLIC_" PIONEER_Q38_GRAPH_LABEL "_HOST_REPEAT graph_seconds%.6f multiply_seconds%.6f ioctl_seconds%.6f timing_valid%u;CPU_WALL_NOT_PURE_GPU_OR_LLM_TOKEN_RATE\n",
			repeated,state.matrixWallSeconds-matrixBefore,double(state.session.runNanos-ioctlBefore)*1e-9,
			unsigned(state.profileValid && repeat>=0 && repeated>=0 && state.session.runNanos>=ioctlBefore));
		okay=okay && !memcmp(first.data(),y->data,graphOutputs*4) && state.session.pagesUploaded==pages && PioneerQ38GgmlSnapshot().projections==2;
#endif
	}
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
	if (!okay) State::Abort(&state); // graph/repeat mismatch is CLOSE ONLY too
#endif
	bool clean=PioneerQ38GgmlFinish(); okay=okay && clean;
	// Publish PUBLIC numeric diagnostic ONLY after clean END;never overwrite.
	if (okay) {
		int fd=open(output,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
		if (fd<0) okay=false;
		else {
			const uint8_t* bytes=reinterpret_cast<const uint8_t*>(first.data()); unsigned done=0;
			while (done<graphOutputs*4) { ssize_t n=write(fd,bytes+done,graphOutputs*4-done); if (n<0 && errno==EINTR) continue; if (n<=0) { okay=false; break; } done+=unsigned(n); }
			if (fsync(fd)) okay=false;
			if (close(fd)) okay=false;
		}
	}
	if (yb) ggml_backend_buffer_free(yb);
	if (xb) ggml_backend_buffer_free(xb);
	if (wb) ggml_backend_buffer_free(wb);
	ggml_backend_free(backend); ggml_free(ctx);
	printf("%s_PUBLIC_" PIONEER_Q38_GRAPH_LABEL "_GGML_EXPORT clean_end%u;"
#ifdef PIONEER_Q38_CPU_VALIDATION
		"CPU_REFERENCE_ALL_OUTPUTS_PLUS_REPEATABILITY_NOT_LLM_INFERENCE\n",
#else
		"REPEATABILITY_NOT_INDEPENDENT_NUMERIC_REFERENCE_OR_LLM_INFERENCE\n",
#endif
		okay?"PASS":"FAIL",unsigned(clean));
	return okay?0:1;
}
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
#if !defined(PIONEER_Q38_WORK_INFERENCE) || !defined(PIONEER_Q38_ORDERED_EXTENT_FAST) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_MODEL_NODE_PROFILE) || defined(PIONEER_Q38_MODEL_ROOT_PROFILE) || defined(PIONEER_Q38_BENCHMARK_HOST) || defined(PIONEER_Q38_FFN_REPEAT_BENCH) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT)
#error Thread sweep is isolated original-TC public THREE32 Work ONLY
#endif
// PIONEER_MODEL_THREAD_SWITCH_BEGIN
static bool SetModelThreadSequence(llama_context* context,ggml_threadpool_t pool32,ggml_threadpool_t pool8,unsigned sequence) {
	unsigned threads=PioneerQ38ModelSequenceThreads(sequence);
	if(!context || !pool32 || !pool8 || !threads)return false;
	// All previous graph work has completed before replacing either pool.
	// Both pools were allocated BEFORE any accelerator FD opened.
	llama_synchronize(context);llama_set_n_threads(context,threads,threads);
	llama_attach_threadpool(context,threads==PioneerQ38ModelNarrowThreads?pool8:pool32,threads==PioneerQ38ModelNarrowThreads?pool8:pool32);
	return true;
}
// PIONEER_MODEL_THREAD_SWITCH_END
#endif
int main(int argc,char** argv)
{
#ifdef PIONEER_Q38_Q8_REPEAT_BENCH
	bool graph=argc==3 && !strcmp(argv[1],"--public-q8-graph-export");
#else
	bool graph=argc==3 && !strcmp(argv[1],"--public-ffn-graph-export");
#endif
#ifdef PIONEER_Q38_FFN_REPEAT_BENCH
	if (!graph) return 2; // isolated diagnostic executable,NEVER normal model/Assistant mode
#endif
	unsigned limit=argc==2?PioneerQ38PublicTokenLimit(argv[1]):0;
	unsigned sequences=argc==2?PioneerQ38PublicSequenceCount(argv[1]):0;
	bool repeatedRun=sequences==3,four=limit==4,longTiming=limit==32 && !repeatedRun;
	// PIONEER_MODEL_THREAD_ARGUMENT_GUARD_BEGIN
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
	if (!repeatedRun || limit!=32 || graph) return 2; // before source/device/threadpool
#endif
	// PIONEER_MODEL_THREAD_ARGUMENT_GUARD_END
	// PIONEER_MODEL_ROOT_ARGUMENT_GUARD_BEGIN
#ifdef PIONEER_Q38_MODEL_ROOT_PROFILE
	if (!repeatedRun || limit!=32 || graph) return 2; // fixed PUBLIC THREE32 ONLY;BEFORE authentication/device/threadpool
#endif
	// PIONEER_MODEL_ROOT_ARGUMENT_GUARD_END
	// PIONEER_NATIVE_Q8_MODEL_ARGUMENT_GUARD_BEGIN
#ifdef PIONEER_Q38_NATIVE_Q8_MODEL_QUALIFICATION
	if (!repeatedRun || limit!=32 || graph) return 2; // fixed PUBLIC THREE32 ONLY,not Assistant/interactive generation
#endif
	// PIONEER_NATIVE_Q8_MODEL_ARGUMENT_GUARD_END
	if (!graph && !limit) return 2;
#if !defined(PIONEER_Q38_WORK_INFERENCE) || !defined(PIONEER_Q38_SPAN_PRELOAD) || !defined(PIONEER_Q38_SPAN_FAST)
	if (repeatedRun) return 2; // explicit diagnostic for retained fast-span Work ONLY
#endif
	signal(SIGINT,RequestStop); signal(SIGTERM,RequestStop);
	setvbuf(stdout,nullptr,_IOLBF,0); setvbuf(stderr,nullptr,_IOLBF,0);
#ifdef PIONEER_Q38_NATIVE_Q8_MODEL_QUALIFICATION
	puts("NATIVE_Q8_PUBLIC_MODEL_QUALIFICATION;EXACT_IDLE_TAG14_ABI3;THREE32_PUBLIC_TOKEN_REPEATABILITY;NO_CPU_LEARNED_REFERENCE_OR_ASSISTANT_READY_CLAIM");
#endif
#ifdef PIONEER_Q38_FFN_REPEAT_BENCH
	puts(PIONEER_Q38_GRAPH_LABEL "_REPEAT32_DIAGNOSTIC_ONLY;FIRST_PLUS32_WARM_GRAPHS;NO_CPU_LEARNED_REFERENCE;OUTPUT_REPEATABILITY_CHECK_OUTSIDE_TIMING");
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
#ifdef PIONEER_Q38_SPAN_FAST
	puts("PUBLIC27B_WORK_TRANSPORT NO_CPU_DATA_VALIDATION;NO_DEVICE_ECHO_READBACK;OWNED_BOUNDS_SOURCE_ACK_COMPLETION_CHECKS_RETAINED");
#endif
	puts("PUBLIC27B_TRANSPORT PRECOHORT_SPAN_1M;ALL_SELECTED_WEIGHTS_BEFORE_ANY_COHORT;NO_LEGACY_UPLOAD_FALLBACK");
#endif
#ifdef PIONEER_Q38_CPU_VALIDATION
	puts("PUBLIC27B_MODE CPU_VALIDATED_DEBUG;EVERY_LEARNED_MATRIX_INDEPENDENT_CPU_REFERENCE;NO_CPU_FALLBACK;SLOW_DIAGNOSTIC");
	puts("PUBLIC27B_INFERENCE_AUTH_BEGIN;NO_ACCELERATOR_FD_YET;CPU_REFERENCE_ENABLED_NO_FALLBACK");
#else
#ifdef PIONEER_Q38_WORK_INFERENCE
	if (repeatedRun) puts("PUBLIC27B_MODE WORK_TRANSPORT_WITH_PUBLIC_REPEATABILITY_DIAGNOSTIC;TOKEN_IDENTITIES_CHECKED_BETWEEN_RUNS;NO_CPU_LEARNED_REFERENCE_OR_FALLBACK");
	else puts("PUBLIC27B_MODE WORK;NO_CPU_REFERENCE_OR_RESULT_VALIDATION;MANDATORY_DRIVER_SAFETY_ONLY;NO_CHECKED_FALLBACK");
#endif
	puts("PUBLIC27B_INFERENCE_AUTH_BEGIN;NO_ACCELERATOR_FD_YET;NO_CPU_REFERENCE_OR_FALLBACK");
#endif
	State state; if (!state.source.Open()) return 1;
	if (stopRequested) return 130; // authentication completed,no accelerator FD exists
	if (graph) { int result=GraphSmoke(state,argv[2]); return stopRequested && !result?130:result; }
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
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
	cp.n_threads=cp.n_threads_batch=PioneerQ38ModelWideThreads;
#endif
#ifdef PIONEER_Q38_MODEL_NODE_PROFILE
	static_assert(unsigned(GGML_OP_COUNT)<=unsigned(PioneerQ38ModelNodeProfile::Operators),"fixed bounded operator histogram");
	PioneerQ38ModelNodeProfile nodeProfile;cp.cb_eval=NodeObserver;cp.cb_eval_user_data=&nodeProfile;
	puts("PUBLIC27B_NODE_OBSERVER_ACTIVE;FORCES_PER_NODE_SCHEDULER_SPLIT_AND_SYNCHRONIZE;INSTRUMENTED_HOST_WALL_NOT_PRODUCTION_SPEED_OR_CPU_REFERENCE;NO_TENSOR_DATA_READS");
#endif
	auto context=llama_init_from_model(model,cp);
	// The frozen CPU backend otherwise creates/frees a disposable worker
	// pool for EVERY CPU graph fragment between accelerator projections.
	// Attach one caller-owned sleeping pool BEFORE any accelerator FD opens.
	ggml_threadpool_t cpuPool=nullptr;
	if (context) {
		auto poolParams=ggml_threadpool_params_default(
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
			PioneerQ38ModelWideThreads
#else
			32
#endif
		); poolParams.poll=0;
		cpuPool=ggml_threadpool_new(&poolParams);
		if (cpuPool) llama_attach_threadpool(context,cpuPool,cpuPool);
	}
	// Frozen library exports new/free,not the optional get_n_threads accessor.
	bool poolReady=cpuPool!=nullptr;
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
	ggml_threadpool_t cpuPool8=nullptr;
	if(context && cpuPool){auto params8=ggml_threadpool_params_default(PioneerQ38ModelNarrowThreads);params8.poll=0;cpuPool8=ggml_threadpool_new(&params8);}
	poolReady=poolReady && cpuPool8;
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP_NARROW
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP_FOUR
	puts("PUBLIC27B_CPU_THREAD_SWEEP schedule8_4_8;TWO_PREALLOCATED_SLEEPING_POOLS;UNCHANGED_MATRIX_WORK;NO_NODE_SPLIT_INSTRUMENTATION;NOT_ASSISTANT_OR_GENERAL_TUNING");
#else
	puts("PUBLIC27B_CPU_THREAD_SWEEP schedule8_1_8;TWO_PREALLOCATED_SLEEPING_POOLS;UNCHANGED_MATRIX_WORK;NO_NODE_SPLIT_INSTRUMENTATION;NOT_ASSISTANT_OR_GENERAL_TUNING");
#endif
#else
	puts("PUBLIC27B_CPU_THREAD_SWEEP schedule32_8_32;TWO_PREALLOCATED_SLEEPING_POOLS;UNCHANGED_MATRIX_WORK;NO_NODE_SPLIT_INSTRUMENTATION;NOT_ASSISTANT_OR_GENERAL_TUNING");
#endif
#endif
	printf("PUBLIC27B_CPU_POOL persistent%u threads%u poll0;NON_MATRIX_CPU_WORK_ONLY"
#ifdef PIONEER_Q38_CPU_VALIDATION
		"_SEPARATE_SERIAL_CPU_REFERENCE_ENABLED\n"
#else
		"_NO_CPU_REFERENCE\n"
#endif
		,unsigned(poolReady),poolReady?
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
		unsigned(PioneerQ38ModelWideThreads)
#else
		32
#endif
		:0);
	unsigned recognized=PioneerQ38GgmlSnapshot().recognizedWeights;
	printf("PUBLIC27B_GRAPH_COVERAGE recognized_learned_weights%u expected497;RESERVATION_ONLY_NOT_EXECUTION\n",recognized);
	bool okay=context && poolReady && state.source.Ready() && recognized==497;
	const auto vocab=llama_model_get_vocab(model); llama_token tokens[8]={};
	int count=llama_tokenize(vocab,"Hello",5,tokens,8,false,false);
	okay=okay && count==1; // fixed PUBLIC one-position smoke,not private chat
	double start=Seconds(),warmSeconds=0,firstSeconds=0;
	unsigned generated=0; uint64_t retainedPages=0;
	PioneerQ38PublicTiming timing;
	PioneerQ38PublicRepeatedTiming repeated;
	llama_token firstSequence[32]={}; unsigned completedSequences=0;
	llama_token token=tokens[0],best=-1;
	if (repeatedRun) puts("PUBLIC27B_REPEATED_TIMING_DIAGNOSTIC fixed_sequences3 positions_per_sequence32 token_identity_repeatability_only;NO_CPU_LEARNED_REFERENCE;WEIGHTS_RETAINED_CPU_SEQUENCE_STATE_RESET");
	// PIONEER_PUBLIC_REPEAT_LOOP_BEGIN
	for (unsigned sequence=0;okay && !stopRequested && sequence<sequences;sequence++) {
		if (sequence) {
			// Current backend claims matrices ONLY;KV/recurrent buffers are
			// CPU-owned. Reset them through the frozen runtime,NOT a driver
			// reset/reload/new owner. The SAME weight pages/session remain.
			llama_synchronize(context); auto memory=llama_get_memory(context);
			uint64_t pages=state.session.pagesUploaded,calls=state.session.calls;
			if (!memory || !state.source.Ready() || state.session.Failed()) { okay=false; break; }
			double resetStart=Seconds(); llama_memory_clear(memory,true);
			okay=state.source.Ready() && !state.session.Failed() && pages==state.session.pagesUploaded && calls==state.session.calls;
			printf("PUBLIC27B_CPU_SEQUENCE_RESET sequence%u seconds%.6f pages%llu unchanged_native_calls%u success%u;NO_CARD_RESET_OR_RELOAD\n",
				sequence,Seconds()-resetStart,(unsigned long long)pages,unsigned(calls==state.session.calls),unsigned(okay));
		}
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
		if(!SetModelThreadSequence(context,cpuPool,cpuPool8,sequence)){okay=false;break;}
		printf("PUBLIC27B_CPU_THREAD_SEQUENCE sequence%u threads%u;SYNCHRONIZED_POOL_SWITCH_NO_CARD_RESET_RELOAD_OR_WEIGHT_UPLOAD\n",sequence,PioneerQ38ModelSequenceThreads(sequence));
#endif
		token=tokens[0]; unsigned sequenceGenerated=0;
		if (repeatedRun) printf("PUBLIC27B_SEQUENCE_BEGIN %u;NOT_A_COMPLETED_BENCHMARK_YET\n",sequence);
	for (unsigned position=0;okay && !stopRequested && position<limit;position++) {
#ifdef PIONEER_Q38_MODEL_ROOT_PROFILE
		state.session.modelRootProfile.Begin(sequence,position); // CPU metrics ONLY,not execution authority
#endif
#ifdef PIONEER_Q38_MODEL_NODE_PROFILE
		nodeProfile.Begin(sequence,position);
#endif
		uint64_t before=PioneerQ38GgmlSnapshot().projections;
		double matrixBefore=state.matrixWallSeconds;
		uint64_t callbacksBefore=state.matrixCallbacks,runBefore=state.session.runNanos;
		double began=Seconds(); auto batch=llama_batch_get_one(&token,1); okay=llama_decode(context,batch)==0;
		double elapsed=Seconds()-began; auto current=PioneerQ38GgmlSnapshot();
#ifdef PIONEER_Q38_MODEL_ROOT_PROFILE
		state.session.modelRootProfile.End(okay); // invalid metrics NEVER alter okay/END
#endif
#ifdef PIONEER_Q38_MODEL_NODE_PROFILE
		nodeProfile.End(okay);
#endif
		if (longTiming) (void)timing.Observe(position,elapsed); // diagnostics ONLY
		if (repeatedRun) (void)repeated.Observe(sequence,position,elapsed);
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
		// Explicit benchmark-only repeatability,not a CPU learned product
		// or production Work result validator. Any mismatch is CLOSE ONLY.
		if (okay && repeatedRun) {
			if (!sequence) firstSequence[position]=best;
			else if (best!=firstSequence[position]) {
				printf("PUBLIC27B_REPEAT_TOKEN_MISMATCH sequence%u position%u expected%d actual%d;CLOSE_ONLY_NO_END_RETRY_OR_RESET\n",sequence,position,firstSequence[position],best);
				okay=false;
			}
		}
		if (!position && !sequence) { retainedPages=state.session.pagesUploaded; firstSeconds=elapsed; }
		else { okay=okay && retainedPages==state.session.pagesUploaded; warmSeconds+=elapsed; }
		printf("PUBLIC27B_POSITION %u selected_token%d seconds%.6f learned_projections%llu pages%llu success%u;"
#ifdef PIONEER_Q38_CPU_VALIDATION
			"CPU_REFERENCE_ENABLED_NO_FALLBACK\n",
#else
			"NO_CPU_REFERENCE_OR_FALLBACK\n",
#endif
			position,best,elapsed,(unsigned long long)(current.projections-before),(unsigned long long)state.session.pagesUploaded,unsigned(okay));
		double matrixSeconds=state.matrixWallSeconds-matrixBefore;
		printf("PUBLIC27B_WALL_PROFILE position%u valid%u matrix_callbacks%llu matrix_callback_seconds%.6f checked_matrix_ioctl_seconds%.6f other_ggml_seconds%.6f;HOST_WALL_DIAGNOSTICS_NOT_PURE_GPU_OR_CPU_TIMES\n",
			position,unsigned(state.profileValid),(unsigned long long)(state.matrixCallbacks-callbacksBefore),matrixSeconds,
			double(state.session.runNanos-runBefore)*1e-9,elapsed-matrixSeconds);
		if (okay) { generated++; sequenceGenerated++; token=best; }
	}
		if (okay && sequenceGenerated==limit) completedSequences++;
		if (repeatedRun) printf("PUBLIC27B_SEQUENCE_COMPUTED %u generated%u success%u;EXPLICIT_CLEAN_END_STILL_REQUIRED\n",sequence,sequenceGenerated,unsigned(okay));
	}
	bool stopped=stopRequested && okay;
	double end=Seconds(); auto stats=PioneerQ38GgmlSnapshot();
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
	if (!okay) State::Abort(&state); // never END after a user-only graph failure
#endif
	bool clean=PioneerQ38GgmlFinish();
	// PIONEER_PUBLIC_REPEAT_LOOP_END
#ifdef PIONEER_Q38_MODEL_NODE_PROFILE
	bool publishNodes=nodeProfile.PublishAllowed(okay,clean,stopped);
	printf("PUBLIC27B_NODE_WALL_PROFILE publication_valid%u;POST_EXPLICIT_CLEAN_END_ONLY;INCLUDES_DIAGNOSTIC_SPLIT_SYNC_OVERHEAD_NOT_PRODUCTION_SPEED_OR_PURE_CPU_TIME\n",unsigned(publishNodes));
	if(publishNodes)for(unsigned op=0;op<unsigned(GGML_OP_COUNT);op++){
		const auto& b=nodeProfile.bucket[op];if(!b.nodes)continue;
		printf("PUBLIC27B_NODE_WALL_OP op%u name%s warm_positions81 nodes%llu nanos%llu;HOST_CLOCK_ONLY_NO_TENSOR_DATA_OR_ADDITIONAL_CARD_IO\n",op,ggml_op_name(ggml_op(op)),(unsigned long long)b.nodes,(unsigned long long)b.nanos);
	}
#endif
#ifdef PIONEER_Q38_MODEL_ROOT_PROFILE
	const auto& rootProfile=state.session.modelRootProfile;
	bool publishRoot= rootProfile.PublishAllowed(okay,clean,stopped);
	printf("PUBLIC27B_ROOT_COPY_PROFILE publication_valid%u;HOST_MEMORY_ONLY_NO_ADDITIONAL_CARD_IO_NOT_PURE_ARITHMETIC_OR_TOKEN_RATE\n",unsigned(publishRoot));
	if(publishRoot)for(unsigned shape=0;shape<9;shape++) {
		const auto& b=rootProfile.bucket[shape];unsigned types[]={8,12,14},widths[]={5120,6144,17408};
		printf("PUBLIC27B_ROOT_COPY_SHAPE type%u K%u measured_positions81 samples%llu worker_sum%llu prepare_cycles%llu wait_audit_cycles%llu gather_dma_cycles%llu read_packets%llu write_packets%llu read_flits%llu write_flits%llu peer_polls%llu terminal_audits%llu;POST_EXPLICIT_CLEAN_END_ONLY\n",
			types[shape/3],widths[shape%3],(unsigned long long)b.samples,(unsigned long long)b.workers,
			(unsigned long long)b.cycles[0],(unsigned long long)b.cycles[1],(unsigned long long)b.cycles[2],
			(unsigned long long)b.packets[0],(unsigned long long)b.packets[1],(unsigned long long)b.packets[2],
			(unsigned long long)b.packets[3],(unsigned long long)b.packets[4],(unsigned long long)b.packets[5]);
	}
#endif
	printf("%s_PUBLIC27B_MODEL_SMOKE public_prompt_Hello generated%u selected_token%d seconds%.6f projections%llu uploaded_pages%llu calls%llu workers%u clean_end%u;COLD_LOAD_SMOKE_NOT_STEADY_TOKEN_RATE_OR_TEXT_QUALITY\n",
		stopped && clean?"STOP":okay && clean?"PASS":"FAIL",generated,best,end-start,(unsigned long long)stats.projections,(unsigned long long)state.session.pagesUploaded,
		(unsigned long long)state.session.calls,state.session.peakWorkers,unsigned(clean));
	if (four) {
		bool complete=okay && clean && !stopped && generated==4 && warmSeconds>0;
		printf("%s_PUBLIC27B_FOUR_TOKEN generated%u first_position_seconds%.6f subsequent_positions%u subsequent_seconds%.6f sample_tokens_per_second%.9f retained_weights%u;TINY_PUBLIC_SMOKE_NOT_STEADY_BENCHMARK_OR_TEXT_QUALITY\n",
			complete?"PASS":"FAIL",generated,firstSeconds,generated?generated-1:0,warmSeconds,
			complete?3/warmSeconds:0,unsigned(retainedPages && retainedPages==state.session.pagesUploaded));
	}
	if (longTiming) {
		bool complete=okay && clean && !stopped && generated==32 && timing.Complete();
		printf("%s_PUBLIC27B_32_TOKEN_TIMING generated%u timing_valid%u skipped_cold_positions1 skipped_warmup_positions4 measured_positions%u measured_seconds%.6f sample_tokens_per_second%.9f minimum_seconds%.6f median_seconds%.6f p95_nearest_rank_seconds%.6f maximum_seconds%.6f retained_weights%u clean_end%u;SINGLE_PUBLIC_SEQUENCE_NOT_REPEATED_BENCHMARK_OR_TEXT_QUALITY\n",
			complete?"PASS":"FAIL",generated,unsigned(complete),timing.Measured(),complete?timing.Seconds():0,complete?timing.Rate():0,
			complete?timing.Percentile(0):0,complete?timing.Percentile(13):0,complete?timing.Percentile(25):0,complete?timing.Percentile(26):0,
			unsigned(retainedPages && retainedPages==state.session.pagesUploaded),unsigned(clean));
	}
	if (repeatedRun) {
		bool complete=okay && clean && !stopped && completedSequences==3 && generated==96 && repeated.Complete();
		printf("%s_PUBLIC27B_REPEATED_TIMING sequences%u generated%u timing_valid%u measured_positions%u measured_seconds%.6f sample_tokens_per_second%.9f retained_weights%u clean_end%u;THREE_IDENTICAL_PUBLIC_SEQUENCES_NOT_DIVERSE_PROMPTS_OR_TEXT_QUALITY\n",
			complete?"PASS":stopped && clean?"STOP":"FAIL",completedSequences,generated,unsigned(complete),complete?81U:0U,
			complete?repeated.Seconds():0,complete?repeated.Rate():0,unsigned(retainedPages && retainedPages==state.session.pagesUploaded),unsigned(clean));
		for (unsigned sequence=0;sequence<3;sequence++) {
			const auto* sample=repeated.Sequence(sequence);
			printf("PUBLIC27B_REPEAT_SAMPLE sequence%u timing_valid%u skipped_prefix_positions5 measured_positions%u seconds%.6f tokens_per_second%.9f median_seconds%.6f p95_nearest_rank_seconds%.6f;POST_CLEAN_END_ONLY\n",
				sequence,unsigned(complete),complete?27U:0U,complete?sample->Seconds():0,complete?sample->Rate():0,
				complete?sample->Percentile(13):0,complete?sample->Percentile(25):0);
		}
	}
	if (context) { llama_synchronize(context); llama_detach_threadpool(context); llama_free(context); }
	if (cpuPool) ggml_threadpool_free(cpuPool);
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP
	if(cpuPool8)ggml_threadpool_free(cpuPool8);
#endif
	llama_model_free(model); llama_backend_free();
	return stopped && clean?130:okay && clean && end>=start?0:1;
}
