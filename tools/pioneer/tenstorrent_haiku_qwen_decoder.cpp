/* SPDX-License-Identifier: MIT */
// Explicit experimental whole-position verifier;never updates Assistant.
#include "TenstorrentHaikuResident.h"
#include "llama.h"
#include "ggml-cpu.h"
#include "PioneerAttentionMath.h"
#include "gguf.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <vector>
static double Seconds() { timespec t={}; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static void Log(ggml_log_level level,const char* s,void*) { if (level>=GGML_LOG_LEVEL_WARN) fputs(s,stderr); }
struct RawModel {
	gguf_context* metadata=NULL; ggml_context* tensors=NULL;
	~RawModel() { if (tensors) ggml_free(tensors); if (metadata) gguf_free(metadata); }
};
struct CpuReference {
	llama_model* model=NULL; llama_context* context=NULL;
	~CpuReference() { if (context) llama_free(context); if (model) llama_model_free(model); }
};
static bool MetaU32(gguf_context* m,const char* key,unsigned expected) {
	int64_t at=gguf_find_key(m,key); bool okay=at>=0 && gguf_get_kv_type(m,at)==GGUF_TYPE_UINT32 && gguf_get_val_u32(m,at)==expected;
	printf("MODEL %s expected%u %s\n",key,expected,okay?"PASS":"FAIL"); return okay;
}
static bool MetaFloat(gguf_context* m,const char* key,float expected) {
	int64_t at=gguf_find_key(m,key); bool okay=at>=0 && gguf_get_kv_type(m,at)==GGUF_TYPE_FLOAT32 && gguf_get_val_f32(m,at)==expected;
	printf("MODEL %s expected%.9g %s\n",key,expected,okay?"PASS":"FAIL"); return okay;
}
static ggml_tensor* Matrix(ggml_context* ctx,const std::string& name,unsigned k,unsigned n) {
	auto* t=ggml_get_tensor(ctx,name.c_str());
	return t && t->data && t->type==GGML_TYPE_Q8_0 && t->ne[0]==k && t->ne[1]==n && t->ne[2]==1 && t->ne[3]==1 && ggml_is_contiguous(t)?t:NULL;
}
static ggml_tensor* Norm(ggml_context* ctx,const std::string& name,unsigned n) {
	auto* t=ggml_get_tensor(ctx,name.c_str());
	if (!t || !t->data || t->type!=GGML_TYPE_F32 || t->ne[0]!=n || t->ne[1]!=1 || t->ne[2]!=1 || t->ne[3]!=1 || !ggml_is_contiguous(t)) return NULL;
	for (unsigned i=0;i<n;i++) if (!isfinite(static_cast<const float*>(t->data)[i]) || fabsf(static_cast<const float*>(t->data)[i])>65504) return NULL;
	return t;
}
static int Argmax(const float* values) {
	int best=0; for (unsigned i=0;i<151936;i++) { if (!isfinite(values[i])) return -1; if (values[i]>values[best]) best=int(i); } return best;
}
#include "PioneerWholeQwenBench.h"
static int Run(const char* path,bool inspect,bool parallel,bool benchmark,bool stages,bool glu,bool greedy,bool rms,bool counters,bool paired,bool direct,bool partial,bool cached,bool pull,bool phases,bool diagnostic)
{
	double coldStart=Seconds();
	RawModel raw; gguf_init_params rp={false,&raw.tensors}; raw.metadata=gguf_init_from_file(path,rp);
	if (!raw.metadata || !raw.tensors) return 1;
	bool valid=MetaU32(raw.metadata,"qwen3.block_count",28) && MetaU32(raw.metadata,"qwen3.embedding_length",1024)
		&& MetaU32(raw.metadata,"qwen3.feed_forward_length",3072) && MetaU32(raw.metadata,"qwen3.attention.head_count",16)
		&& MetaU32(raw.metadata,"qwen3.attention.head_count_kv",8) && MetaFloat(raw.metadata,"qwen3.rope.freq_base",1000000)
		&& MetaFloat(raw.metadata,"qwen3.attention.layer_norm_rms_epsilon",1e-6f);
	const char* roles[]={"attn_q.weight","attn_v.weight","attn_k.weight","attn_output.weight","ffn_gate.weight","ffn_up.weight","ffn_down.weight",
		"attn_norm.weight","ffn_norm.weight","attn_q_norm.weight","attn_k_norm.weight"};
	const unsigned k[]={1024,1024,1024,2048,1024,1024,3072},n[]={2048,1024,1024,1024,3072,3072,1024};
	ggml_tensor* tensors[28][11]={};
	for (unsigned layer=0;layer<28;layer++) for (unsigned role=0;role<11;role++) {
		std::string name="blk."+std::to_string(layer)+"."+roles[role];
		tensors[layer][role]=role<7?Matrix(raw.tensors,name,k[role],n[role]):Norm(raw.tensors,name,role<9?1024:128);
		if (!tensors[layer][role]) { fprintf(stderr,"Unsupported/missing fixed decoder tensor%s\n",name.c_str()); valid=false; }
	}
	auto* finalNorm=Norm(raw.tensors,"output_norm.weight",1024); auto* classifier=Matrix(raw.tensors,"output.weight",1024,151936);
	auto* embedding=Matrix(raw.tensors,"token_embd.weight",1024,151936);
	// Match the pinned Qwen3 model loader's explicit tied-output rule.
	// A present but malformed output tensor MUST NOT be substituted.
	if (!ggml_get_tensor(raw.tensors,"output.weight") && embedding) { classifier=embedding; puts("MODEL tied output uses token_embd.weight (pinned Qwen3 loader rule)"); }
	if (!finalNorm || !classifier || !embedding) fprintf(stderr,"Final model tensors norm/classifier/embedding:%u/%u/%u\n",finalNorm!=NULL,classifier!=NULL,embedding!=NULL);
	valid &= finalNorm && classifier && embedding;
	if (!valid) { fprintf(stderr,"Fixed Qwen3-0.6B Q8_0 contract refused BEFORE card open\n"); return 1; }
	puts("PASS fixed model geometry/197 Q8 matrices/113 FP32 norms/embedding;NO card opened yet");
	if (inspect) return 0;
	CpuReference cpu; llama_model_params mp=llama_model_default_params(); mp.n_gpu_layers=0;
	cpu.model=llama_model_load_from_file(path,mp); if (!cpu.model) return 1;
	const llama_vocab* vocab=llama_model_get_vocab(cpu.model);
	if (llama_model_n_embd(cpu.model)!=1024 || llama_model_n_layer(cpu.model)!=28 || llama_vocab_n_tokens(vocab)!=151936) return 1;
	llama_context_params cp=llama_context_default_params(); cp.n_ctx=256; cp.n_batch=cp.n_ubatch=1; cp.n_threads=cp.n_threads_batch=32;
	if (benchmark) cp.n_batch=cp.n_ubatch=128;
	cp.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_DISABLED; cp.offload_kqv=false; cp.op_offload=false;
	cpu.context=llama_init_from_model(cpu.model,cp); if (!cpu.context || llama_n_threads(cpu.context)!=32) return 1;
	printf("MODEL decoder scheduler:%s\n",diagnostic?"FIXED ROOT/PEER INPUT DIAGNOSTIC ONLY;NOT inference (contract26)":phases?"exact four-phase FPU pipeline/cached23 transport (experimental contract25)":pull?"exact math/parallel frozen-input fetch and ALL-bit readback (experimental contract24)":cached?"exact math/once-per-job cached query (experimental contract23)":partial?"experimental native partial-product accumulation (contract22,quality NOT qualified)":direct?"exact direct-instruction math/paired gate-up/eight KV/six GLU (experimental contract21)":paired?"119-peer paired gate/up,eight KV,six GLU,on-card argmax/RMS (experimental contract20)":counters?"eight KV/six GLU/on-card argmax/RMS/cumulative expectations (experimental contract19)":rms?"eight KV/six GLU/on-card argmax/positive FastTwoSum RMS (experimental contract18)":greedy?"eight KV/six GLU/on-card argmax (experimental contract17)":glu?"eight KV/six GLU groups (experimental contract16)":stages?"eight concurrent KV groups/device stages (experimental contract15)":parallel?"eight concurrent KV groups (experimental contract14)":"serial groups (contract13)");
	PioneerResidentDevice card; if (!card.Open(0x80000000ULL,false,true,true,true,true,true,true,true,1,119,true,true,true,false,diagnostic?17:phases?16:pull?15:cached?14:partial?13:direct?12:paired?11:counters?10:rms?9:greedy?8:glu?7:stages?6:parallel?5:4)) return 1;
	PioneerResidentTensorCache<PioneerResidentDevice> cache(card,0x80000000ULL,false,true,true,true,true);
	PioneerDecoderModelOffsets offsets={}; double uploadStart=Seconds(); bool okay=true;
	for (unsigned layer=0;layer<28 && okay;layer++) {
		auto* out=reinterpret_cast<uint64_t*>(&offsets.layers[layer]);
		for (unsigned role=0;role<7 && okay;role++) { auto* t=tensors[layer][role];
			okay=cache.Q8Weights(t->data,unsigned(t->type),k[role],n[role],[&](unsigned row,unsigned b,uint8_t* bytes) {
				memcpy(bytes,static_cast<const uint8_t*>(t->data)+row*t->nb[1]+b*34,34); return true; },out[role]); }
		printf("Decoder immutable matrices layer%u/28 retained%zu bytes%llu pages%llu\n",layer+1,cache.Tensors(),(unsigned long long)cache.Bytes(),(unsigned long long)card.uploads);
	}
	if (okay) okay=cache.Q8Weights(classifier->data,unsigned(classifier->type),1024,151936,[&](unsigned row,unsigned b,uint8_t* bytes) {
		memcpy(bytes,static_cast<const uint8_t*>(classifier->data)+row*classifier->nb[1]+b*34,34); return true; },offsets.vocabulary);
	uint64_t normAt=cache.Bytes();
	auto uploadNorm=[&](ggml_tensor* t,uint64_t& offset) { uint32_t page[1024]={}; memcpy(page,t->data,size_t(t->ne[0])*4); offset=normAt;
		if (!card.UploadPages(normAt,page,1)) return false;
		normAt+=4096; return true; };
	for (unsigned layer=0;layer<28 && okay;layer++) { auto* out=reinterpret_cast<uint64_t*>(&offsets.layers[layer]);
		for (unsigned role=7;role<11 && okay;role++) okay=uploadNorm(tensors[layer][role],out[role]); }
	if (okay) okay=uploadNorm(finalNorm,offsets.finalNorm);
	printf("Decoder immutable upload%.6fs tensors%zu bytes%llu uploaded pages%llu span calls%llu\n",Seconds()-uploadStart,cache.Tensors(),(unsigned long long)normAt,(unsigned long long)card.uploads,(unsigned long long)card.spanCalls);
	if (!okay || cache.Tensors()!=197) { card.Finish(); return 1; }
	double registryStart=Seconds();
	if (!card.RegisterDecoder(offsets)) { card.Finish(); return 1; }
	printf("Decoder registry/owners ready%.6fs;complete cold preparation%.6fs including model/CPU context/upload/registry\n",Seconds()-registryStart,Seconds()-coldStart);
	const uint64_t uploads=card.uploads;
	const char* prompt="<|im_start|>user\nWhat is the capital of France?<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
	std::vector<llama_token> tokens(256); int count=llama_tokenize(vocab,prompt,int(strlen(prompt)),tokens.data(),int(tokens.size()),false,true);
	okay=count>0 && count<128; std::vector<float> input(1024),logits(151936); uint32_t rope[128];
	llama_token token=0; unsigned generated=0; double decodeSeconds=0;
	for (unsigned position=0;okay && generated<4;position++) {
		if (position<unsigned(count)) token=tokens[position];
		auto batch=llama_batch_get_one(&token,1); if (llama_decode(cpu.context,batch)) { okay=false; break; }
		double start=Seconds(); ggml_get_type_traits(embedding->type)->to_float(static_cast<const uint8_t*>(embedding->data)+uint64_t(token)*embedding->nb[1],input.data(),1024);
		float theta=float(position),step=powf(1000000.0f,-2.0f/128);
		for (unsigned i=0;i<64;i++) { rope[2*i]=PioneerAttentionBits(cosf(theta)); rope[2*i+1]=PioneerAttentionBits(sinf(theta)); theta*=step; }
		okay=card.DecodeWhole(position+1,reinterpret_cast<const uint32_t*>(input.data()),rope,reinterpret_cast<uint32_t*>(logits.data()));
		double elapsed=Seconds()-start; if (!okay) break;
		if (diagnostic) {
			const char* names[]={"tag","root_Q","peer_Q","root_K","peer_K","root_gate","peer_gate","root_up","peer_up","layer","Q_K_mismatched_words","gate_up_mismatched_words","reserved12","reserved13","reserved14","reserved15"};
			puts("FIXED INPUT DIAGNOSTIC:these16 output slots are NOT model logits;NO inference/quality/performance qualification");
			for (unsigned i=0;i<16;i++) printf(" INPUT_TRACE %s=%.9g bits%08x\n",names[i],double(logits[i]),PioneerAttentionBits(logits[i]));
			bool tagged=PioneerAttentionBits(logits[0])==0x4640e400 && logits[9]==27.0f;
			bool clean=card.Finish();
			printf("DIAGNOSTIC completed%u cleanup%u generated0 projections%llu attention%llu full-results%llu;NOT an inference target\n",tagged,clean,(unsigned long long)card.calls,(unsigned long long)card.attentionCalls,(unsigned long long)card.wideCalls);
			return tagged && clean?0:1;
		}
		const float* reference=llama_get_logits_ith(cpu.context,-1); double dot=0,a2=0,b2=0,d2=0;
		for (unsigned i=0;i<151936;i++) { double a=logits[i],b=reference[i],d=a-b; dot+=a*b; a2+=a*a; b2+=b*b; d2+=d*d; }
		double cosine=dot/sqrt(a2*b2),relative=sqrt(d2/(b2+1e-30)); int actual=Argmax(logits.data()),expected=Argmax(reference);
		okay=isfinite(cosine) && isfinite(relative) && cosine>=.999 && relative<=.02 && actual>=0 && actual==expected && card.uploads==uploads;
		printf("WHOLE pos%u wall%.6fs cosine%.9f L2%.9f argmax%d/%d quality%u uploads%llu one GPU job/position\n",position,elapsed,cosine,relative,actual,expected,okay,(unsigned long long)card.uploads);
		tt_runtime_profile_result profile={};
		if (card.DecoderProfile(profile)) {
			printf(" WHOLE kernel phase us");
			for (unsigned slot=0;slot<TT_PROFILE_SLOTS;slot++) {
				if (profile.mask&(1U<<slot)) printf(" t%u=%llu",slot,(unsigned long long)profile.microseconds[slot]);
			}
			printf("\n");
		}
		if (stages) {
			tt_decoder_stage_profile p={};
			if (card.DecoderStages(p)) {
				const char* names[]={"matrices","RMS","Q8","RoPE","SiLU","attention","vocabulary","total"};
				printf(" WHOLE device clock ticks (diagnostic,NOT microseconds)");
				for (unsigned i=0;i<8;i++) printf(" %s=%u/%.2f%%/calls%u",names[i],p.cycles[i],100.0*p.cycles[i]/p.cycles[7],p.calls[i]);
				printf("\n");
			} else puts(" WHOLE device stage timing unavailable;inference proof is independent");
		}
		if (greedy && okay) { uint32_t selected=0,bestBits=0; double replayStart=Seconds();
			okay=card.DecodeGreedy(position+1,reinterpret_cast<const uint32_t*>(input.data()),rope,selected,bestBits);
			elapsed=Seconds()-replayStart; okay &= selected==unsigned(actual) && bestBits==PioneerAttentionBits(logits[unsigned(actual)]);
			printf("GREEDY pos%u wall%.6fs token%u full-logit/selected-bit replay%u sole128B result\n",position,elapsed,selected,okay);
		}
		if (position>=unsigned(count)) decodeSeconds+=elapsed;
		if (position+1>=unsigned(count)) { token=actual; generated++; printf("OUTPUT greedy token%d\n",actual); }
	}
	if (okay && generated==4 && benchmark) okay=PioneerWholeQwenBenchmark(card,cpu.model,cpu.context,embedding,greedy);
	bool clean=card.Finish(); okay &= clean && generated==4;
	printf("%s experimental whole decoder:generated%u projections%llu attention%llu full-vocab%llu cleanup%u;CPU32 reference OUTSIDE card timer,no model reupload\n",
		okay?"PASS":"FAIL",generated,(unsigned long long)card.calls,(unsigned long long)card.attentionCalls,(unsigned long long)card.wideCalls,clean);
	if (greedy) printf("Distinct128B token results%llu;full-vocabulary results above are diagnostic ONLY\n",(unsigned long long)card.greedyCalls);
	if (okay && decodeSeconds>0) printf("Diagnostic whole-decoder generation3/%.6fs=%.6ftok/s;NOT a repeated warm CPU speedup gate or Assistant qualification\n",decodeSeconds,3/decodeSeconds);
	return okay?0:1;
}
int main(int argc,char** argv)
{
	bool inspect=argc==3 && !strcmp(argv[1],"--inspect-model");
	bool stageBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-profile-decoder");
	bool rmsBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-rms-decoder");
	bool countersBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-counters-decoder");
	bool counters=countersBenchmark || (argc==3 && !strcmp(argv[1],"--verify-counters-decoder"));
	bool pairedBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-paired-decoder");
	bool diagnostic=argc==3 && !strcmp(argv[1],"--diagnose-pull-input");
	bool phaseBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-phase-decoder");
	bool phases=phaseBenchmark || (argc==3 && !strcmp(argv[1],"--verify-phase-decoder"));
	bool pullBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-pull-decoder");
	bool pull=pullBenchmark || (argc==3 && !strcmp(argv[1],"--verify-pull-decoder"));
	bool cachedBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-cached-decoder");
	bool cached=diagnostic || phases || pull || cachedBenchmark || (argc==3 && !strcmp(argv[1],"--verify-cached-decoder"));
	bool partialBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-partial-decoder");
	bool partial=partialBenchmark || (argc==3 && !strcmp(argv[1],"--verify-partial-decoder"));
	bool directBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-inline-decoder");
	bool direct=cached || partial || directBenchmark || (argc==3 && !strcmp(argv[1],"--verify-inline-decoder"));
	bool paired=direct || pairedBenchmark || (argc==3 && !strcmp(argv[1],"--verify-paired-decoder"));
	bool rms=paired || counters || rmsBenchmark || (argc==3 && !strcmp(argv[1],"--verify-rms-decoder"));
	bool greedyBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-greedy-decoder");
	bool greedy=rms || greedyBenchmark || (argc==3 && !strcmp(argv[1],"--verify-greedy-decoder"));
	bool gluBenchmark=argc==3 && !strcmp(argv[1],"--benchmark-glu-decoder");
	bool glu=greedy || gluBenchmark || (argc==3 && !strcmp(argv[1],"--verify-glu-decoder"));
	bool benchmark=phaseBenchmark || pullBenchmark || cachedBenchmark || partialBenchmark || directBenchmark || pairedBenchmark || countersBenchmark || rmsBenchmark || greedyBenchmark || gluBenchmark || stageBenchmark || (argc==3 && !strcmp(argv[1],"--benchmark-parallel-decoder"));
	bool stages=glu || stageBenchmark || (argc==3 && !strcmp(argv[1],"--profile-parallel-decoder"));
	bool parallel=stages || benchmark || (argc==3 && !strcmp(argv[1],"--verify-parallel-decoder"));
	if (argc!=3 || (!inspect && !parallel && strcmp(argv[1],"--verify-whole-decoder"))) { fprintf(stderr,"Usage:%s --inspect-model|--verify-whole-decoder|--verify-parallel-decoder|--benchmark-parallel-decoder|--profile-parallel-decoder|--benchmark-profile-decoder|--verify-glu-decoder|--benchmark-glu-decoder|--verify-greedy-decoder|--benchmark-greedy-decoder|--verify-rms-decoder|--benchmark-rms-decoder|--verify-counters-decoder|--benchmark-counters-decoder|--verify-paired-decoder|--benchmark-paired-decoder|--verify-inline-decoder|--benchmark-inline-decoder|--verify-partial-decoder|--benchmark-partial-decoder|--verify-cached-decoder|--benchmark-cached-decoder|--verify-pull-decoder|--benchmark-pull-decoder|--verify-phase-decoder|--benchmark-phase-decoder|--diagnose-pull-input MODEL.gguf\n",argv[0]); return 2; }
	setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0); llama_log_set(Log,NULL); llama_backend_init(); ggml_backend_register(ggml_backend_cpu_reg());
	int result=Run(argv[2],inspect,parallel,benchmark,stages,glu,greedy,rms,counters,paired,direct,partial,cached,pull,phases,diagnostic); llama_backend_free(); return result;
}
