/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlBackend.h"
#include "llama.h"
#include "ggml-cpu.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
static double Seconds() { struct timespec t={}; if (clock_gettime(CLOCK_MONOTONIC,&t)) return NAN; return t.tv_sec+t.tv_nsec*1e-9; }
static void Log(enum ggml_log_level level,const char* text,void*) { if (level>=GGML_LOG_LEVEL_WARN) fputs(text,stderr); }
static int Argmax(const float* values,int n) {
	int best=0; for (int i=0;i<n;i++) { if (!isfinite(values[i])) return -1; if (values[i]>values[best]) best=i; } return best;
}
static void Piece(const llama_vocab* vocab,llama_token token) {
	char text[512]; int n=llama_token_to_piece(vocab,token,text,sizeof(text),0,true);
	if (n<0) { printf("<piece too long>"); return; }
	for (int i=0;i<n;i++) { unsigned char c=text[i]; if (c<32 || c==127) printf("\\x%02x",c); else putchar(c); }
}
#include "PioneerQwenWarmBench.h"
int main(int argc,char** argv) {
	bool layerBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-layer-kv-q8-host1");
	bool layerFfn=layerBenchmark || (argc==3 && !strcmp(argv[1],"--verify-layer-kv-q8-host1"));
	bool coordinatedBenchmark=layerBenchmark || (argc==3 && !strcmp(argv[1],"--benchmark-coordinated-kv-q8-host1"));
	bool coordinated=layerFfn || coordinatedBenchmark || (argc==3 && !strcmp(argv[1],"--verify-coordinated-kv-q8-host1"));
	bool blockBenchmark=coordinatedBenchmark || (argc==3 && !strcmp(argv[1],"--benchmark-blocks-kv-q8-host1"));
	bool payloadBlocks=coordinated || blockBenchmark || (argc==3 && !strcmp(argv[1],"--verify-blocks-kv-q8-host1"));
	bool residentBenchmark=blockBenchmark || (argc==3 && !strcmp(argv[1],"--benchmark-resident-kv-q8-host1"));
	bool residentKV=payloadBlocks || residentBenchmark || (argc==3 && !strcmp(argv[1],"--verify-resident-kv-q8-host1"));
	bool fleet=residentKV || (argc==3 && !strcmp(argv[1],"--verify-fleet-attention-q8-host1"));
	bool bulkUpload=fleet || (argc==3 && !strcmp(argv[1],"--verify-device-attention-bulk-upload-q8-host1"));
	bool deviceAttention=bulkUpload || (argc==3 && !strcmp(argv[1],"--verify-device-attention-q8-host1"));
	bool reuse=deviceAttention || (argc==3 && (!strcmp(argv[1],"--verify-dual-reuse-q8-host1") || !strcmp(argv[1],"--benchmark-dual-reuse-q8-host1")));
	bool dual=reuse || (argc==3 && (!strcmp(argv[1],"--verify-dual-q8-host1") || !strcmp(argv[1],"--benchmark-dual-q8-host1")));
	bool exact=dual || ((argc==3 || argc==4) && (!strcmp(argv[1],"--probe-native-q8-exact-reference") || !strcmp(argv[1],"--verify-native-q8-exact-host1") || !strcmp(argv[1],"--benchmark-native-q8-exact-host1") || !strcmp(argv[1],"--trace-native-q8-exact")));
	bool host1=dual || (argc==3 && (!strcmp(argv[1],"--verify-native-q8-host1") || !strcmp(argv[1],"--benchmark-native-q8-host1") || !strcmp(argv[1],"--verify-native-q8-exact-host1") || !strcmp(argv[1],"--benchmark-native-q8-exact-host1")));
	bool roundingProbe=(argc==3 || argc==4) && !strcmp(argv[1],"--probe-cpu-q8-rounding");
	bool cpuProbe=roundingProbe || ((argc==3 || argc==4) && !strcmp(argv[1],"--probe-cpu-rope-reference"));
	bool cardRounding=(argc==3 || argc==4) && !strcmp(argv[1],"--probe-native-q8-rounding");
	bool probe=cpuProbe || cardRounding || ((argc==3 || argc==4) && (!strcmp(argv[1],"--probe-native-q8-reference") || !strcmp(argv[1],"--probe-native-q8-exact-reference")));
	bool trace=(argc==3 || argc==4) && (!strcmp(argv[1],"--trace-native-q8") || !strcmp(argv[1],"--trace-native-q8-exact"));
	bool native=exact || host1 || (probe && !cpuProbe) || trace || ((argc==3 || argc==4) && (!strcmp(argv[1],"--diagnose-native-q8") || !strcmp(argv[1],"--verify-native-q8") || !strcmp(argv[1],"--benchmark-native-q8")));
	bool f32Kv=(argc==3 || argc==4) && !strcmp(argv[1],"--diagnose-compact-q8-f32-kv");
	bool benchmark=residentBenchmark || (argc==3 && (!strcmp(argv[1],"--benchmark-dual-reuse-q8-host1") || !strcmp(argv[1],"--benchmark-dual-q8-host1") || !strcmp(argv[1],"--benchmark-compact-q8") || !strcmp(argv[1],"--benchmark-int8-q8") || !strcmp(argv[1],"--benchmark-native-q8") || !strcmp(argv[1],"--benchmark-native-q8-host1") || !strcmp(argv[1],"--benchmark-native-q8-exact-host1")));
	bool compact=native || f32Kv || ((argc==3 || argc==4) && (!strcmp(argv[1],"--diagnose-compact-q8") || !strcmp(argv[1],"--verify-compact-q8") || !strcmp(argv[1],"--benchmark-compact-q8")));
	bool q8=compact || benchmark || ((argc==3 || argc==4) && (!strcmp(argv[1],"--diagnose-int8-q8") || !strcmp(argv[1],"--verify-int8-q8")));
	bool fp16=(argc==3 || argc==4) && (!strcmp(argv[1],"--diagnose-fp16-cpu-q8") || !strcmp(argv[1],"--verify-fp16-cpu-q8"));
	bool diagnoseCpuActivations=probe || trace || f32Kv || ((argc==3 || argc==4) && (!strcmp(argv[1],"--diagnose-cpu-q8") || !strcmp(argv[1],"--diagnose-fp16-cpu-q8") || !strcmp(argv[1],"--diagnose-int8-q8") || !strcmp(argv[1],"--diagnose-compact-q8") || !strcmp(argv[1],"--diagnose-native-q8")));
	bool verifyCpuActivations=(dual && !benchmark) || (argc==3 && (!strcmp(argv[1],"--verify-cpu-q8") || !strcmp(argv[1],"--verify-fp16-cpu-q8") || !strcmp(argv[1],"--verify-int8-q8") || !strcmp(argv[1],"--verify-compact-q8") || !strcmp(argv[1],"--verify-native-q8") || !strcmp(argv[1],"--verify-native-q8-host1") || !strcmp(argv[1],"--verify-native-q8-exact-host1")));
	bool cpuActivations=diagnoseCpuActivations || verifyCpuActivations || benchmark;
	bool diagnose=(argc==3 || argc==4) && (!strcmp(argv[1],"--diagnose-qwen") || diagnoseCpuActivations);
	if (!diagnose && !verifyCpuActivations && !benchmark && (argc!=3 || strcmp(argv[1],"--verify-qwen"))) {
		fprintf(stderr,"Resident KV qualification: %s --verify-resident-kv-q8-host1 MODEL\n",argv[0]);
		fprintf(stderr,"Usage: %s --verify-qwen|--verify-cpu-q8|--verify-fp16-cpu-q8|--verify-int8-q8|--verify-compact-q8|--verify-native-q8|--verify-native-q8-host1|--benchmark-int8-q8|--benchmark-compact-q8|--benchmark-native-q8|--benchmark-native-q8-host1 MODEL | --diagnose-qwen|--diagnose-cpu-q8|--diagnose-fp16-cpu-q8|--diagnose-int8-q8|--diagnose-compact-q8|--diagnose-compact-q8-f32-kv|--diagnose-native-q8|--trace-native-q8 MODEL [FRESH_FAILED_INPUT_FILE] | --probe-native-q8-reference|--probe-cpu-rope-reference MODEL [TENSOR]\nExperimental single-token resident hybrid. Independent CPU reference; not a speedup claim or Assistant change.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
	llama_log_set(Log,NULL); llama_backend_init();
	if (cpuProbe) { ggml_backend_register(ggml_backend_cpu_reg()); puts("CPU-only diagnostic: CPU backend only, no P100A registration/open/ioctl or Assistant changes"); }
	else ggml_backend_load_all();
	llama_model_params modelParams=llama_model_default_params(); modelParams.n_gpu_layers=0;
	llama_model* model=llama_model_load_from_file(argv[2],modelParams); if (!model) return 1;
	char arch[64]={}; llama_model_meta_val_str(model,"general.architecture",arch,sizeof(arch));
	if (strcmp(arch,"qwen3") || llama_model_n_embd(model)!=1024) { fprintf(stderr,"This bootstrap requires Qwen3-0.6B Q8_0\n"); llama_model_free(model); return 1; }
	llama_context_params params=llama_context_default_params();
	int referenceThreads=diagnose?1:32;
	params.n_ctx=256; params.n_batch=1; params.n_ubatch=1; params.n_threads=referenceThreads; params.n_threads_batch=referenceThreads;
	params.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_DISABLED; params.offload_kqv=false;
	if (f32Kv) params.type_k=params.type_v=GGML_TYPE_F32;
	params.cb_eval=diagnose?PioneerGgmlReferenceTap:NULL;
	PioneerGgmlTrace(trace || probe);
	// Create the independent CPU context before registering any P100A
	// device. Its backend list therefore cannot accidentally use the card.
	llama_context_params referenceParams=params;
	if (benchmark) referenceParams.n_batch=referenceParams.n_ubatch=128;
	llama_context* reference=llama_init_from_model(model,referenceParams);
	if (!reference) { llama_model_free(model); return 1; }
	if (host1) params.n_threads=params.n_threads_batch=1; // CPU32 reference unchanged; GPU handles learned matrices
	params.cb_eval=trace?PioneerGgmlCompareTap:NULL; params.cb_eval_user_data=NULL; // never overwrite CPU reference with card data
	PioneerGgmlDiagnose(diagnose,argc==4 && !probe?argv[3]:NULL); PioneerGgmlCpuActivationQuantization(cpuActivations); PioneerGgmlFp16Operands(fp16); PioneerGgmlQ8Operands(q8); PioneerGgmlQ8Compact(compact); PioneerGgmlQ8Native(native);
	PioneerGgmlQ8Exact(exact);
	PioneerGgmlQ8Dual(dual);
	PioneerGgmlQ8DualReuse(reuse);
	PioneerGgmlAttention(deviceAttention);
	PioneerGgmlBulkUpload(bulkUpload);
	PioneerGgmlFleet(fleet?(coordinated?119:120):0);
	PioneerGgmlResidentKV(residentKV);
	PioneerGgmlPayloadBlocks(payloadBlocks);
	PioneerGgmlCoordinated(coordinated);
	PioneerGgmlLayerFfn(layerFfn);
	if (deviceAttention) {
		params.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_ENABLED;
		params.offload_kqv=true; params.op_offload=true;
	}
	if (!cpuProbe) ggml_backend_register(PioneerGgmlReg());
	llama_context* actual=probe?NULL:llama_init_from_model(model,params);
	if (!actual && !probe) { llama_free(reference); llama_model_free(model); return 1; }
	if (actual && (llama_n_threads(actual)!=params.n_threads || llama_n_threads(reference)!=referenceThreads)) {
		fprintf(stderr,"CPU thread configuration mismatch; no decode submitted\n"); llama_free(actual); llama_free(reference); llama_model_free(model); return 1;
	}
	const llama_vocab* vocab=llama_model_get_vocab(model); int count=llama_vocab_n_tokens(vocab);
	uint64_t expectedTensors=uint64_t(llama_model_n_layer(model))*7+1;
	const char* prompt="<|im_start|>user\nWhat is the capital of France?<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
	std::vector<llama_token> tokens(256);
	int tokenCount=llama_tokenize(vocab,prompt,int(strlen(prompt)),tokens.data(),int(tokens.size()),false,true);
	bool passed=tokenCount>0 && tokenCount<128;
	if (probe) {
		passed &= tokenCount>=6;
		for (unsigned position=0;passed && position<6;position++) {
			PioneerGgmlBeginPosition(position); llama_token token=tokens[position]; auto batch=llama_batch_get_one(&token,1);
			passed=!llama_decode(reference,batch) && PioneerGgmlReferenceCount()==expectedTensors;
		}
		const char* name=argc==4?argv[3]:"blk.13.attn_k.weight";
		passed=passed && (roundingProbe?PioneerGgmlReplayRoundingReference(name):cpuProbe?PioneerGgmlReplayCpuRope(name):PioneerGgmlReplayReference(name,cardRounding));
		printf("%s: %s only; no full-model generation or speedup qualification\n",passed?"PASS":"FAIL",cpuProbe?"CPU control/simulation, NO card access":"one CPU-reference-input projection replay");
		llama_free(reference); llama_model_free(model); llama_backend_free(); return passed?0:1;
	}
	printf("Qwen resident prototype: %d prompt tokens, 4 greedy output tokens; %llu learned matrices required; CPU%d reference created before P100A registration; CPU-Q8 activation preprocessing %u; operand format %s\n",tokenCount,(unsigned long long)expectedTensors,referenceThreads,cpuActivations,q8?"Q8 integer blocks + device scales":fp16?"FP16":"BF16");
	printf("CPU threads: independent reference %d, card-side host %d; not a CPU baseline reduction\n",referenceThreads,params.n_threads);
	if (q8) printf("Q8 retained weight layout: %s\n",compact?"compact 1.125 bytes/parameter":"expanded 2.25 bytes/parameter");
	if (native) puts("Native signed INT8 hardware unpack; unsupported -128 weights fail without clamping or fallback");
	if (exact) puts("Exact FP32 scale bits + compensated SFPU accumulation, all learned sums stay on the card; experimental until full verification");
	if (dual) puts("Two fixed bulk workers; no reduction-dimension split; every matvec requires BOTH independent completion/stop/NoC proofs; not a CPU speedup claim");
	if (reuse) puts("Explicit guarded per-core input/scales reuse; every result still computed, poisoned, read and proved; no CPU learned products or result substitution");
	if (deviceAttention) puts("DEVICE ATTENTION: all28 fused attention operations/224 grouped-head tiles per position REQUIRED; QK/PV on SFPU, softmax on card TRISC0; host KV prefix staging, NOT a speedup claim");
	if (residentKV) puts("RESIDENT KV: separate immutable all8-head image; private28MiB tail; new KV token only; host prefix coherence check/replay, no CPU attention fallback");
	if (layerFfn) puts("FIXED FFN candidate: GPU gate/up/SiLU/Q8/down in one job; GLOBAL use-count/shape proofs; separately scheduled GLU also on GPU; no speedup claim");
	if (bulkUpload) puts("EXPLICIT BULK UPLOAD: synchronous 1..4-page owned transfer; original learned tensor bytes and all numerical/coverage/cleanup gates unchanged");
	printf("Both independent contexts K/V cache types: %d/%d (%s); graph/logit/greedy gates unchanged\n",int(params.type_k),int(params.type_v),f32Kv?"explicit FP32 precision experiment":"library defaults");
	double begin=Seconds(),decodeSeconds=0; unsigned generated=0; llama_token token=0;
	for (int position=0;passed && generated<4;position++) {
		if (diagnose) PioneerGgmlBeginPosition(unsigned(position));
		if (position<tokenCount) token=tokens[size_t(position)];
		llama_batch batch=llama_batch_get_one(&token,1);
		if (llama_decode(reference,batch)) { passed=false; break; }
		if (diagnose && PioneerGgmlReferenceCount()!=expectedTensors) {
			fprintf(stderr,"CPU graph tap captured %zu/%llu learned matrices; no card decode submitted\n",PioneerGgmlReferenceCount(),(unsigned long long)expectedTensors);
			passed=false; break;
		}
		PioneerGgmlStats coverageBefore={};
		if (deviceAttention && !PioneerGgmlSnapshot(coverageBefore)) { passed=false; break; }
		double start=Seconds(); int status=llama_decode(actual,batch); double elapsed=Seconds()-start;
		if (status) { fprintf(stderr,"P100A decode STOP status %d; no CPU fallback\n",status); passed=false; break; }
		const float* a=llama_get_logits(actual); const float* r=llama_get_logits(reference);
		if (!a || !r) { passed=false; break; }
		double dot=0,a2=0,r2=0,delta2=0;
		for (int i=0;i<count;i++) { dot+=double(a[i])*r[i]; a2+=double(a[i])*a[i]; r2+=double(r[i])*r[i]; double d=double(a[i])-r[i]; delta2+=d*d; }
		double cosine=dot/sqrt(a2*r2),relative=sqrt(delta2/r2);
		int next=Argmax(a,count),cpu=Argmax(r,count);
		PioneerGgmlStats stats={}; bool live=PioneerGgmlSnapshot(stats);
		if (deviceAttention) {
			if (layerFfn && (stats.ffnOps-coverageBefore.ffnOps!=28 || stats.gluOps!=coverageBefore.gluOps)) {
				fprintf(stderr,"Strict fused FFN coverage failed: deltaFFN%llu deltaGLU%llu,expected28/0; no accepted result\n",(unsigned long long)(stats.ffnOps-coverageBefore.ffnOps),(unsigned long long)(stats.gluOps-coverageBefore.gluOps)); passed=false; break;
			}
			printf("  DEVICE ATTENTION coverage: ops=%llu tiles=%llu matrix=%llu delta=%llu/%llu/%llu\n",
				(unsigned long long)stats.attentionOps,(unsigned long long)stats.attentionTiles,(unsigned long long)stats.matvecs,
				(unsigned long long)(stats.attentionOps-coverageBefore.attentionOps),(unsigned long long)(stats.attentionTiles-coverageBefore.attentionTiles),
				(unsigned long long)(stats.matvecs-coverageBefore.matvecs));
			if (stats.attentionOps-coverageBefore.attentionOps!=28 || stats.attentionTiles-coverageBefore.attentionTiles!=224
				|| stats.matvecs-coverageBefore.matvecs!=(fleet?197U:234U)
				|| (fleet && stats.peakWorkers!=(coordinated?119U:120U))) { fprintf(stderr,"Strict device attention/multicore coverage failed; no CPU fallback or accepted result\n"); passed=false; break; }
		}
		printf("Position %d: card decode %.6fs cosine %.9f relative-L2 %.9f argmax card/CPU %d/%d tensors %llu pages %llu matvecs %llu\n",
			position,elapsed,cosine,relative,next,cpu,(unsigned long long)stats.tensors,(unsigned long long)stats.uploads,(unsigned long long)stats.matvecs);
		if (layerFfn) printf("  Fixed FFN coverage total%llu,standalone GPU GLU%llu; intermediates stay on card\n",(unsigned long long)stats.ffnOps,(unsigned long long)stats.gluOps);
		if (!live || stats.failed || stats.tensors!=expectedTensors || stats.uploads!=stats.bytes/4096 || !stats.matvecs
			|| next<0 || cpu<0 || !isfinite(cosine) || !isfinite(relative) || cosine<.999 || relative>.02) {
			fprintf(stderr,"Full-logit verification failed; actual output is not replaced\n"); passed=false; break;
		}
		if (position+1<tokenCount) continue;
		if (next!=cpu) { fprintf(stderr,"Greedy output differs from CPU; stopping this strict bootstrap, not substituting the reference token\n"); passed=false; break; }
		if (llama_vocab_is_eog(vocab,next)) break;
		printf("OUTPUT token %d: ",next); Piece(vocab,next); putchar('\n');
		token=next; generated++;
		// Only decode of generated-token inputs is a steady-state sample;
		// the final prompt token belongs to time-to-first-token instead.
		if (position>=tokenCount) decodeSeconds+=elapsed;
	}
	PioneerGgmlStats stats={}; passed &= PioneerGgmlSnapshot(stats) && stats.tensors && !stats.failed;
	bool warmQualified=passed && generated==4;
	if (benchmark && warmQualified) {
		puts("Bootstrap Qwen generation verification PASS; starting separate repeated warm speed/quality gate");
		passed=PioneerQwenWarmBenchmark(model,reference,actual,expectedTensors,unsigned(params.n_threads),layerFfn);
		passed &= PioneerGgmlSnapshot(stats) && !stats.failed;
	}
	bool clean=PioneerGgmlFinish(); passed &= clean && generated==4;
	printf("%s: generated %u checked Qwen tokens, retained %llu tensors / %llu bytes, %llu pages uploaded, %llu real matvec calls; cleanup %u, total %.3fs\n",
		passed?"PASS":"FAIL",generated,(unsigned long long)stats.tensors,(unsigned long long)stats.bytes,(unsigned long long)stats.uploads,(unsigned long long)stats.matvecs,clean,Seconds()-begin);
	printf("Backend phase totals: weight preparation/upload/cache %.6fs, activation preprocessing %.6fs, matvec/staging/result %.6fs, attention/staging/result %.6fs, primitive diagnostics %.6fs; not device-only timing\n",
		stats.weightSeconds,stats.inputSeconds,stats.matvecSeconds,stats.attentionSeconds,stats.diagnosticSeconds);
	if (fleet) printf("Whole-projection path: peak workers%u,197 learned projections via%u matrix/FFN +28 attention host calls/position; still host norms/RoPE/KV,NOT zero-round-trip decoding\n",stats.peakWorkers,layerFfn?141U:197U);
	if (coordinated) printf("Compact activation staging: %llu exact same-input uploads skipped; all device arithmetic/completion proofs retained; NOT an end-to-end speed claim\n",(unsigned long long)stats.activationReuses);
	if (passed && decodeSeconds>0 && !diagnose) printf("Diagnostic warm decode: 3 samples / %.6fs = %.6f tokens/s; CPU reference excluded, NOT a benchmark or speedup claim\n",decodeSeconds,3/decodeSeconds);
	llama_free(actual); llama_free(reference); llama_model_free(model); llama_backend_free(); return passed?0:1;
}
