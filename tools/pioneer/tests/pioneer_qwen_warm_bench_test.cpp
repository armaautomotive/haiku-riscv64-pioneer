/* SPDX-License-Identifier: MIT */
// Explicit control-flow/timing FAKE: no llama library, driver, open/ioctl or
// device arithmetic. Does not prove model quality, hardware or performance.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
typedef int llama_token;
struct llama_vocab {};
struct llama_model {};
struct llama_context { bool cpu; double cost; unsigned past=0,clears=0,calls=0; float logits[4]={}; };
struct llama_batch { llama_token* token; int count; };
struct PioneerGgmlStats { uint64_t tensors=197,bytes=4096,uploads=1,matvecs=1; bool failed=false; };
static PioneerGgmlStats state;
static double now=1;
static bool wrong=false,badLogits=false,reupload=false,earlyEos=false,longPrompt=false,missing=false;
static double Seconds() { return now; }
static int Argmax(const float* v,int n) { int best=0; for (int i=0;i<n;i++) { if (!std::isfinite(v[i])) return -1; if (v[i]>v[best]) best=i; } return best; }
static const llama_vocab* llama_model_get_vocab(llama_model*) { static llama_vocab v; return &v; }
static int llama_vocab_n_tokens(const llama_vocab*) { return 4; }
static bool llama_vocab_is_eog(const llama_vocab*,int) { return earlyEos; }
static int llama_tokenize(const llama_vocab*,const char*,int,llama_token* out,int,bool,bool) { for (int i=0;i<3;i++) out[i]=i; return longPrompt?129:3; }
static llama_context* llama_get_memory(llama_context* c) { return c; }
static void llama_memory_clear(llama_context* c,bool data) { assert(data); c->past=0; c->clears++; }
static llama_batch llama_batch_get_one(llama_token* token,int count) { return {token,count}; }
static int llama_decode(llama_context* c,llama_batch b) {
	assert(b.count>0 && b.count<129 && (c->cpu || b.count==1));
	c->calls++; c->past+=unsigned(b.count); now+=c->cost;
	for (auto& value:c->logits) value=.1f;
	unsigned token=c->past%4; if (!c->cpu && wrong) token=(token+1)%4;
	c->logits[token]=1;
	if (!c->cpu && badLogits) for (unsigned i=0;i<4;i++) if (i!=token) c->logits[i]=.9f;
	if (!c->cpu) { state.matvecs+=234; if (reupload) { state.uploads++; state.bytes+=4096; } }
	return 0;
}
static const float* llama_get_logits(llama_context* c) { return c->logits; }
static bool PioneerGgmlSnapshot(PioneerGgmlStats& out) { out=state; return !missing; }
using std::isfinite;
#include "../PioneerQwenWarmBench.h"
static void Reset() { now=1; state={}; wrong=badLogits=reupload=earlyEos=longPrompt=missing=false; }
int main() {
	puts("OFFLINE CONTROLLER FAKE: every timing/rate below is SYNTHETIC, NOT P100A/model performance");
	llama_model model;
	{
		Reset(); llama_context cpu={true,.3},card={false,.1};
		assert(PioneerQwenWarmBenchmark(&model,&cpu,&card,197));
		assert(cpu.clears==3 && card.clears==3 && cpu.calls==48 && card.calls==54 && state.uploads==1);
	}
	{
		Reset(); llama_context cpu={true,.3},card={false,.5};
		assert(!PioneerQwenWarmBenchmark(&model,&cpu,&card,197));
		assert(cpu.clears==3 && card.clears==3 && state.uploads==1); // quality alone is NOT a speedup
	}
	for (unsigned fault=0;fault<8;fault++) {
		Reset(); llama_context cpu={true,.3},card={false,.1};
		wrong=fault==0; badLogits=fault==1; reupload=fault==2; earlyEos=fault==3; longPrompt=fault==4;
		missing=fault==5; if (fault==6) card.cost=-.1;
		if (fault==7) card.cost=NAN;
		assert(!PioneerQwenWarmBenchmark(&model,&cpu,&card,197));
		if (fault==0 || fault==1) assert(card.calls==3); // stop before feeding any reference token
		if (fault==3 || fault==4 || fault==5) assert(!card.calls);
	}
	puts("PASS: benchmark controller fake, batched CPU/single-vector card, independent greedy/logit gates, fixed resident upload count, three-request latency/rate qualification and failure rejection; NO model or hardware performance proven");
}
