/* SPDX-License-Identifier: MIT */
#include "PioneerGgmlBackend.h"
#include "llama.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
static double Seconds() { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static void Log(enum ggml_log_level level,const char* text,void*) { if (level>=GGML_LOG_LEVEL_WARN) fputs(text,stderr); }
static int Argmax(const float* values,int n) {
	int best=0; for (int i=0;i<n;i++) { if (!isfinite(values[i])) return -1; if (values[i]>values[best]) best=i; } return best;
}
static void Piece(const llama_vocab* vocab,llama_token token) {
	char text[512]; int n=llama_token_to_piece(vocab,token,text,sizeof(text),0,true);
	if (n<0) { printf("<piece too long>"); return; }
	for (int i=0;i<n;i++) { unsigned char c=text[i]; if (c<32 || c==127) printf("\\x%02x",c); else putchar(c); }
}
int main(int argc,char** argv) {
	bool diagnoseCpuActivations=(argc==3 || argc==4) && !strcmp(argv[1],"--diagnose-cpu-q8");
	bool verifyCpuActivations=argc==3 && !strcmp(argv[1],"--verify-cpu-q8");
	bool cpuActivations=diagnoseCpuActivations || verifyCpuActivations;
	bool diagnose=(argc==3 || argc==4) && (!strcmp(argv[1],"--diagnose-qwen") || diagnoseCpuActivations);
	if (!diagnose && !verifyCpuActivations && (argc!=3 || strcmp(argv[1],"--verify-qwen"))) {
		fprintf(stderr,"Usage: %s --verify-qwen|--verify-cpu-q8 MODEL | --diagnose-qwen|--diagnose-cpu-q8 MODEL [FRESH_FAILED_INPUT_FILE]\nExperimental single-token resident BF16 hybrid. Independent CPU reference; not a speedup claim or Assistant change.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
	llama_log_set(Log,NULL); llama_backend_init(); ggml_backend_load_all();
	llama_model_params modelParams=llama_model_default_params(); modelParams.n_gpu_layers=0;
	llama_model* model=llama_model_load_from_file(argv[2],modelParams); if (!model) return 1;
	char arch[64]={}; llama_model_meta_val_str(model,"general.architecture",arch,sizeof(arch));
	if (strcmp(arch,"qwen3") || llama_model_n_embd(model)!=1024) { fprintf(stderr,"This bootstrap requires Qwen3-0.6B Q8_0\n"); llama_model_free(model); return 1; }
	llama_context_params params=llama_context_default_params();
	int referenceThreads=diagnose?1:32;
	params.n_ctx=256; params.n_batch=1; params.n_ubatch=1; params.n_threads=referenceThreads; params.n_threads_batch=referenceThreads;
	params.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_DISABLED; params.offload_kqv=false;
	params.cb_eval=diagnose?PioneerGgmlReferenceTap:NULL;
	// Create the independent CPU context before registering any P100A
	// device. Its backend list therefore cannot accidentally use the card.
	llama_context* reference=llama_init_from_model(model,params);
	if (!reference) { llama_model_free(model); return 1; }
	params.cb_eval=NULL; params.cb_eval_user_data=NULL; // never overwrite CPU reference with card data
	PioneerGgmlDiagnose(diagnose,argc==4?argv[3]:NULL); PioneerGgmlCpuActivationQuantization(cpuActivations); ggml_backend_register(PioneerGgmlReg());
	llama_context* actual=llama_init_from_model(model,params);
	if (!actual) { llama_free(reference); llama_model_free(model); return 1; }
	const llama_vocab* vocab=llama_model_get_vocab(model); int count=llama_vocab_n_tokens(vocab);
	uint64_t expectedTensors=uint64_t(llama_model_n_layer(model))*7+1;
	const char* prompt="<|im_start|>user\nWhat is the capital of France?<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
	std::vector<llama_token> tokens(256);
	int tokenCount=llama_tokenize(vocab,prompt,int(strlen(prompt)),tokens.data(),int(tokens.size()),false,true);
	bool passed=tokenCount>0 && tokenCount<128;
	printf("Qwen resident prototype: %d prompt tokens, 4 greedy output tokens; %llu learned matrices required; CPU%d reference created before P100A registration; CPU-Q8 activation preprocessing %u\n",tokenCount,(unsigned long long)expectedTensors,referenceThreads,cpuActivations);
	double begin=Seconds(),decodeSeconds=0; unsigned generated=0; llama_token token=0;
	for (int position=0;passed && generated<4;position++) {
		if (position<tokenCount) token=tokens[size_t(position)];
		llama_batch batch=llama_batch_get_one(&token,1);
		if (llama_decode(reference,batch)) { passed=false; break; }
		if (diagnose && PioneerGgmlReferenceCount()!=expectedTensors) {
			fprintf(stderr,"CPU graph tap captured %zu/%llu learned matrices; no card decode submitted\n",PioneerGgmlReferenceCount(),(unsigned long long)expectedTensors);
			passed=false; break;
		}
		double start=Seconds(); int status=llama_decode(actual,batch); double elapsed=Seconds()-start;
		if (status) { fprintf(stderr,"P100A decode STOP status %d; no CPU fallback\n",status); passed=false; break; }
		const float* a=llama_get_logits(actual); const float* r=llama_get_logits(reference);
		if (!a || !r) { passed=false; break; }
		double dot=0,a2=0,r2=0,delta2=0;
		for (int i=0;i<count;i++) { dot+=double(a[i])*r[i]; a2+=double(a[i])*a[i]; r2+=double(r[i])*r[i]; double d=double(a[i])-r[i]; delta2+=d*d; }
		double cosine=dot/sqrt(a2*r2),relative=sqrt(delta2/r2);
		int next=Argmax(a,count),cpu=Argmax(r,count);
		PioneerGgmlStats stats={}; bool live=PioneerGgmlSnapshot(stats);
		printf("Position %d: card decode %.6fs cosine %.9f relative-L2 %.9f argmax card/CPU %d/%d tensors %llu pages %llu matvecs %llu\n",
			position,elapsed,cosine,relative,next,cpu,(unsigned long long)stats.tensors,(unsigned long long)stats.uploads,(unsigned long long)stats.matvecs);
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
	bool clean=PioneerGgmlFinish(); passed &= clean && generated==4;
	printf("%s: generated %u checked Qwen tokens, retained %llu tensors / %llu bytes, %llu pages uploaded, %llu real matvec calls; cleanup %u, total %.3fs\n",
		passed?"PASS":"FAIL",generated,(unsigned long long)stats.tensors,(unsigned long long)stats.bytes,(unsigned long long)stats.uploads,(unsigned long long)stats.matvecs,clean,Seconds()-begin);
	printf("Backend phase totals: weight preparation/upload/cache %.6fs, activation preprocessing %.6fs, matvec/staging/result %.6fs, primitive diagnostics %.6fs; not device-only timing\n",
		stats.weightSeconds,stats.inputSeconds,stats.matvecSeconds,stats.diagnosticSeconds);
	if (passed && decodeSeconds>0 && !diagnose) printf("Diagnostic warm decode: 3 samples / %.6fs = %.6f tokens/s; CPU reference excluded, NOT a benchmark or speedup claim\n",decodeSeconds,3/decodeSeconds);
	llama_free(actual); llama_free(reference); llama_model_free(model); llama_backend_free(); return passed?0:1;
}
