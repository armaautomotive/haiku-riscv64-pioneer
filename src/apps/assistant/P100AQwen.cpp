/* SPDX-License-Identifier: MIT */
#include "P100AQwen.h"
#include "../../../tools/pioneer/PioneerGgmlBackend.h"
#include "../../../tools/pioneer/TenstorrentHaikuTile.h"
#include <llama.h>
#include <math.h>
#include <time.h>
#include <vector>

static double Now() { struct timespec t={}; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int Maximum(const float* values,int n) {
	int best=0; for (int i=0;i<n;i++) { if (!isfinite(values[i])) return -1; if (values[i]>values[best]) best=i; } return best;
}
struct AssistantP100AQwen::Data {
	llama_model* model=NULL;
	llama_context* reference=NULL;
	llama_context* actual=NULL;
	bool validate=true;
	bool stoppedAtBoundary=false;
	unsigned referenceDecodes=0,positions=0;
	double generationSeconds=0;
	unsigned generationSamples=0;
	const std::function<bool()>* cancelled=NULL;
	const std::function<void(const std::string&,const char*)>* progress=NULL;
	const std::string* answer=NULL;
	double started=0,last=0;
	bool CloseActual() {
		bool clean=!actual || PioneerGgmlFinish();
		if (actual) llama_free(actual);
		actual=NULL; PioneerGgmlContextExecution(false); return clean;
	}
	~Data() { CloseActual(); if (reference) llama_free(reference); if (model) llama_model_free(model); }
	bool CreateReference() {
		if (reference) return true;
		PioneerGgmlContextExecution(false);
		llama_context_params p=llama_context_default_params();
		p.n_ctx=256; p.n_batch=p.n_ubatch=1; p.n_threads=p.n_threads_batch=32;
		p.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_DISABLED; p.offload_kqv=false; p.op_offload=false;
		reference=llama_init_from_model(model,p);
		return reference!=NULL;
	}
	bool CreateActual() {
		if (actual) return true;
		PioneerGgmlDiagnose(false); PioneerGgmlTrace(false);
		PioneerGgmlFp16Operands(false); PioneerGgmlCpuActivationQuantization(true);
		PioneerGgmlQ8Operands(true); PioneerGgmlQ8Compact(true); PioneerGgmlQ8Native(true);
		PioneerGgmlQ8Exact(true); PioneerGgmlQ8Dual(true); PioneerGgmlQ8DualReuse(true);
		static bool registered=false;
		if (!registered) { ggml_backend_register(PioneerGgmlReg()); registered=true; }
		llama_context_params p=llama_context_default_params();
		p.n_ctx=256; p.n_batch=p.n_ubatch=1; p.n_threads=p.n_threads_batch=1;
		p.flash_attn_type=LLAMA_FLASH_ATTN_TYPE_DISABLED; p.offload_kqv=false;
		PioneerGgmlContextExecution(true); actual=llama_init_from_model(model,p);
		PioneerGgmlContextExecution(false);
		PioneerGgmlStats stats={}; return actual && PioneerGgmlSnapshot(stats) && !stats.failed;
	}
	static bool Boundary(void* opaque) {
		auto& self=*static_cast<Data*>(opaque);
		if (self.cancelled && (*self.cancelled)()) { self.stoppedAtBoundary=true; return false; }
		double now=Now();
		if (self.progress && now-self.last>=1) {
			PioneerGgmlStats s={}; PioneerGgmlSnapshot(s); char status[240];
			if (s.tensors<197) snprintf(status,sizeof(status),"P100A first-use upload: %.1f/639.4 MiB, %llu/197 matrices; can take ~15 minutes. Stop is transfer-boundary safe.",double(s.uploads)/256,(unsigned long long)s.tensors);
			else snprintf(status,sizeof(status),"P100A Qwen%s: %llu completed matvecs, %.1fs; CPU attention; CPU reference validation %s.",self.validate?"":" Fast",(unsigned long long)s.matvecs,now-self.started,self.validate?"ON":"OFF");
			(*self.progress)(*self.answer,status); self.last=now;
		}
		return true;
	}
};
AssistantP100AQwen::AssistantP100AQwen():fData(NULL),fFaulted(false) {}
AssistantP100AQwen::~AssistantP100AQwen() { delete fData; }
bool AssistantP100AQwen::Available() {
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR); if (fd<0) return false;
	tt_resident_session_result s={}; int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&s,sizeof(s)); close(fd);
	return !status && s.version==1 && !s.cold && (s.capabilities&TT_PIONEER_CAP_Q8_DUAL_REUSE)!=0;
}
bool AssistantP100AQwen::Load(const char* path,std::string& error,bool validate) {
	Data* data=new Data;
	data->validate=validate;
	PioneerGgmlContextExecution(false);
	llama_model_params m=llama_model_default_params(); m.n_gpu_layers=0;
	data->model=llama_model_load_from_file(path,m);
	char arch[64]={};
	if (data->model) llama_model_meta_val_str(data->model,"general.architecture",arch,sizeof(arch));
	bool okay=data->model && !strcmp(arch,"qwen3") && llama_model_n_embd(data->model)==1024 && llama_model_n_layer(data->model)==28;
	if (okay) {
		okay=(!validate || data->CreateReference()) && data->CreateActual();
	}
	if (!okay) { delete data; error="Could not initialize the exact Qwen3-0.6B P100A runtime; no CPU fallback."; return false; }
	delete fData; fData=data;
	fprintf(stderr,"Assistant P100A Qwen load: CPU validation %s; reference context %s\n",validate?"ON":"OFF",data->reference?"present":"absent");
	return true;
}
bool AssistantP100AQwen::SetValidation(bool enabled,std::string& error) {
	if (!fData || fFaulted || (enabled && !fData->CreateReference())) {
		error="Could not configure CPU validation; previous Qwen mode retained. No CPU fallback."; return false;
	}
	// An existing reference may remain allocated, but fast mode never clears,
	// decodes, samples or reads it. Cold fast loads do not create it at all.
	fData->validate=enabled;
	return true;
}
bool AssistantP100AQwen::Complete(const std::string& question,int limit,std::string& answer,
	std::string& error,const std::function<bool()>& cancelled,
	const std::function<void(const std::string&,const char*)>& progress)
{
	if (!fData || fFaulted) { error="P100A Qwen is unavailable; no CPU fallback."; return false; }
	if (question.empty() || question.size()>512 || limit<1 || limit>16) { error="Experimental P100A Qwen: at most 512 UTF-8 bytes and 1-16 output tokens."; return false; }
	std::string prompt="<|im_start|>user\n"+question+"<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
	const llama_vocab* vocab=llama_model_get_vocab(fData->model); int vocabulary=llama_vocab_n_tokens(vocab);
	std::vector<llama_token> tokens(256);
	int count=llama_tokenize(vocab,prompt.c_str(),int(prompt.size()),tokens.data(),int(tokens.size()),false,true);
	if (count<=0 || count>192 || count+limit>256) { error="Question exceeds the P100A experimental 192-token prompt limit. Shorten it."; return false; }
	if (!fData->CreateActual()) { fFaulted=true; error="P100A context unavailable; no retry or CPU fallback."; return false; }
	fData->cancelled=&cancelled; fData->progress=&progress; fData->answer=&answer;
	fData->started=Now(); fData->last=0;
	fData->referenceDecodes=fData->positions=fData->generationSamples=0;
	fData->generationSeconds=0;
	fData->stoppedAtBoundary=false;
	PioneerGgmlBoundary(Data::Boundary,fData);
	if (fData->validate) llama_memory_clear(llama_get_memory(fData->reference),true);
	llama_memory_clear(llama_get_memory(fData->actual),true);
	bool success=true; int generated=0; llama_token token=0;
	for (int position=0;generated<limit;position++) {
		double stepStarted=Now();
		if (cancelled()) { fData->stoppedAtBoundary=true; success=false; break; }
		if (position<count) token=tokens[position];
		auto batch=llama_batch_get_one(&token,1);
		if (fData->validate) {
			fData->referenceDecodes++;
			if (llama_decode(fData->reference,batch)) { error="Independent CPU validation failed; no substituted output."; success=false; break; }
		}
		if (cancelled()) { fData->stoppedAtBoundary=true; success=false; break; }
		PioneerGgmlStats before={},after={};
		if (!PioneerGgmlSnapshot(before) || llama_decode(fData->actual,batch)
			|| !PioneerGgmlSnapshot(after) || after.failed || after.tensors!=197 || after.bytes!=670482432
			|| after.uploads!=163692 || after.matvecs-before.matvecs!=234) {
			error="P100A execution/coverage stopped; no CPU fallback or result substitution."; success=false; break;
		}
		fData->positions++;
		const float* a=llama_get_logits(fData->actual);
		if (!a) { error="P100A logits unavailable."; success=false; break; }
		int next=Maximum(a,vocabulary);
		if (next<0) { error="Non-finite P100A logits; backend disabled, no fallback."; success=false; break; }
		if (fData->validate) {
			const float* r=llama_get_logits(fData->reference);
			if (!r) { error="CPU reference logits unavailable."; success=false; break; }
			double dot=0,a2=0,r2=0,delta=0;
			for (int i=0;i<vocabulary;i++) { double x=a[i],y=r[i]; dot+=x*y; a2+=x*x; r2+=y*y; delta+=(x-y)*(x-y); }
			double cosine=dot/sqrt(a2*r2),relative=sqrt(delta/r2);
			int cpu=Maximum(r,vocabulary);
			if (cpu<0 || !isfinite(cosine) || !isfinite(relative) || cosine<.999 || relative>.02
				|| (position+1>=count && next!=cpu)) { error="P100A numerical/token validation failed; backend disabled, no fallback."; success=false; break; }
		}
		if (position>=count) { fData->generationSeconds+=Now()-stepStarted; fData->generationSamples++; }
		if (position+1<count) continue;
		if (llama_vocab_is_eog(vocab,next)) break;
		char piece[1024]; int n=llama_token_to_piece(vocab,next,piece,sizeof(piece),0,true);
		if (n<0 || answer.size()+n>8192) { error="P100A reply exceeded its output bound."; success=false; break; }
		answer.append(piece,n); token=next; generated++;
		progress(answer,fData->validate?"P100A Qwen3-0.6B — experimental; each token independently CPU-validated.":"P100A Fast Qwen3-0.6B — CPU validation OFF; device safety checks retained.");
	}
	PioneerGgmlBoundary(NULL,NULL);
	fData->cancelled=NULL; fData->progress=NULL; fData->answer=NULL;
	if (!success) {
		bool clean=fData->CloseActual();
		fFaulted=!fData->stoppedAtBoundary || !clean;
		if (!clean) error="P100A cleanup could not be proven; backend disabled. No retry, reset or CPU fallback.";
		else if (fData->stoppedAtBoundary) error="Stopped safely between P100A transfers/matvecs. Model retained; next use may need a fresh upload.";
		return false;
	}
	char status[384]; snprintf(status,sizeof(status),"P100A Qwen: %d output tokens, %.1fs; CPU validation %s, reference decodes %u, device positions %u; generation %u samples/%.3fs (%.3f tok/s). Experimental, not a CPU speedup.",generated,Now()-fData->started,fData->validate?"ON":"OFF",fData->referenceDecodes,fData->positions,fData->generationSamples,fData->generationSeconds,fData->generationSeconds>0?fData->generationSamples/fData->generationSeconds:0);
	fprintf(stderr,"%s\n",status); progress(answer,status); return true;
}
