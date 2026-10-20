/* SPDX-License-Identifier: MIT */
// Actual extracted CLI loop/stop/cleanup selection,synthetic CPU runtime.
// No model/FD/device data or physical completion authority exists here.
#include "../PioneerQ38PublicTiming.h"
#include <cassert>
#include <cstdio>
#include <signal.h>
#include <cstdint>
#include <initializer_list>
enum class Fault { None,StopBefore,StopMid,StopReset,TokenMismatch,Decode,Time,NoMemory,ResetSource,ResetCalls,ResetPages,Pages,Logits };
using llama_token=int;
struct State {
	struct Source { bool valid=true; bool Ready() { return valid; } } source;
	struct Session {
		uint64_t pagesUploaded=0,calls=0,runNanos=0;
		bool failed=false; bool Failed() { return failed; }
	} session;
	bool opened=false,profileValid=true;
	double matrixWallSeconds=0;
	uint64_t matrixCallbacks=0;
	unsigned aborts=0,ends=0,finishes=0;
	static void Abort(State* s) { assert(!s->session.failed); s->session.failed=true; s->aborts++; }
};
struct Snapshot { uint64_t projections=0; bool failed=false; };
struct FakeContext {
	State* state=nullptr;
	Snapshot stats;
	Fault fault=Fault::None;
	unsigned position=0,resets=0,decodes=0;
	double clock=100;
	bool badTime=false;
	float logits[4]={};
};
static FakeContext* current=nullptr;
#include "pioneer_public_stop.inc"
static double Seconds() {
	current->clock+=1;
	if (current->badTime) { current->badTime=false; return NAN; }
	return current->clock;
}
static int QuietPrintf(const char*,...) { return 0; }
static Snapshot PioneerQ38GgmlSnapshot() { return current->stats; }
static bool PioneerQ38GgmlFinish() {
	auto& s=*current->state; s.finishes++;
	if (s.session.failed) { assert(s.aborts==1 && !s.ends); return false; }
	if (s.opened) s.ends++;
	return true;
}
static void llama_synchronize(FakeContext*) {}
static FakeContext* llama_get_memory(FakeContext* c) { return c->fault==Fault::NoMemory && c->decodes==32?nullptr:c; }
static void llama_memory_clear(FakeContext* c,bool data) {
	assert(data && c->decodes==32*(c->resets+1)); c->resets++; c->position=0;
	if (c->fault==Fault::StopReset) raise(SIGINT);
	if (c->fault==Fault::ResetSource) c->state->source.valid=false;
	if (c->fault==Fault::ResetCalls) c->state->session.calls++;
	if (c->fault==Fault::ResetPages) c->state->session.pagesUploaded++;
}
struct Batch { llama_token* token; int count; };
static Batch llama_batch_get_one(llama_token* token,int n) { return {token,n}; }
static int llama_decode(FakeContext* c,Batch batch) {
	assert(batch.count==1 && batch.token && !c->state->session.failed);
	auto& s=*c->state; s.opened=true; c->decodes++; c->position++;
	s.session.calls+=505; s.session.pagesUploaded=4558653; s.session.runNanos+=100000000;
	s.matrixCallbacks+=497; s.matrixWallSeconds+=.5; c->stats.projections+=497;
	for (auto& value:c->logits) value=0;
	unsigned selected=(c->position-1)%4;
	if (c->fault==Fault::TokenMismatch && c->resets && c->position==8) selected=(selected+1)%4;
	c->logits[selected]=1;
	if (c->position==4) {
		if (c->fault==Fault::StopMid) raise(SIGINT);
		if (c->fault==Fault::Decode) { c->stats.failed=true; return 1; }
		if (c->fault==Fault::Time) c->badTime=true;
		if (c->fault==Fault::Pages) s.session.pagesUploaded++;
		if (c->fault==Fault::Logits) c->logits[0]=NAN;
	}
	return 0;
}
static const float* llama_get_logits_ith(FakeContext* c,int index) { assert(index==-1); return c->logits; }
static int llama_vocab_n_tokens(int) { return 4; }
struct Result { bool okay,clean,stopped,timed; unsigned generated,sequences,resets,decodes,aborts,ends,finishes; uint64_t projections; };
static Result Run(Fault fault)
{
	State state; FakeContext fake; fake.state=&state; fake.fault=fault; current=&fake;
	stopRequested=fault==Fault::StopBefore; signal(SIGINT,RequestStop); signal(SIGTERM,RequestStop);
	bool okay=true,repeatedRun=true,longTiming=false;
	unsigned sequences=3,limit=32,generated=0,completedSequences=0;
	FakeContext* context=&fake; int vocab=0;
	llama_token tokens[8]={7},token=7,best=-1,firstSequence[32]={};
	uint64_t retainedPages=0; double firstSeconds=0,warmSeconds=0;
	PioneerQ38PublicTiming timing; PioneerQ38PublicRepeatedTiming repeated;
#define printf QuietPrintf
#define PIONEER_Q38_WORK_INFERENCE
#include "pioneer_public_repeat_loop.inc"
#undef PIONEER_Q38_WORK_INFERENCE
#undef printf
	(void)firstSeconds; (void)warmSeconds; (void)end;
	return {okay,clean,stopped,repeated.Complete(),generated,completedSequences,fake.resets,fake.decodes,state.aborts,state.ends,state.finishes,stats.projections};
}
int main()
{
	auto complete=Run(Fault::None);
	assert(complete.okay && complete.clean && !complete.stopped && complete.timed && complete.generated==96
		&& complete.sequences==3 && complete.resets==2 && complete.decodes==96 && !complete.aborts && complete.ends==1
		&& complete.finishes==1 && complete.projections==96*497);
	for (Fault stop:{Fault::StopBefore,Fault::StopMid,Fault::StopReset}) {
		auto r=Run(stop);
		assert(r.okay && r.clean && r.stopped && !r.timed && !r.aborts && r.finishes==1);
		if (stop==Fault::StopBefore) assert(!r.decodes && !r.generated && !r.ends && !r.resets);
		else if (stop==Fault::StopMid) assert(r.decodes==4 && r.generated==4 && r.ends==1 && !r.resets);
		else assert(r.decodes==32 && r.generated==32 && r.sequences==1 && r.resets==1 && r.ends==1);
	}
	for (Fault fault:{Fault::TokenMismatch,Fault::Decode,Fault::NoMemory,Fault::ResetSource,Fault::ResetCalls,Fault::ResetPages,Fault::Pages,Fault::Logits}) {
		auto r=Run(fault);
		assert(!r.okay && !r.clean && !r.stopped && !r.timed && r.aborts==1 && !r.ends && r.finishes==1);
		if (fault==Fault::TokenMismatch) assert(r.decodes==40 && r.generated==39 && r.resets==1);
		else if (fault==Fault::Decode || fault==Fault::Pages || fault==Fault::Logits) assert(r.decodes==4 && r.generated==3);
		else assert(r.decodes==32 && r.generated==32 && r.resets==(fault==Fault::NoMemory?0U:1U));
	}
	auto timing=Run(Fault::Time);
	assert(timing.okay && timing.clean && !timing.stopped && !timing.timed && timing.decodes==96
		&& timing.resets==2 && !timing.aborts && timing.ends==1); // diagnostic time failure NEVER grants/withdraws cleanup authority
	puts("PASS ACTUAL extracted public repetition loop:3x32/same weights/CPU reset only,all96 positions/all497 projections each/no reupload,real SIGINT before/mid/reset stops at complete boundary with clean END,token/logit/source/page/reset/graph failures CLOSE ONLY/no retry/END,next operation refused;bad timing suppresses rate but never authorizes cleanup;SYNTHETIC CPU runtime NOT native state-reset qualification");
}
