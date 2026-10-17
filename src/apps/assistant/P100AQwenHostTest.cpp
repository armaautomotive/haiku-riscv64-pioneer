/* SPDX-License-Identifier: MIT */
// Model/context lifecycle ONLY. Never call Available, Complete or decode.
// Does not read/write settings, chats, device files or model installation.
#include "P100AQwen.h"
#include "../../../tools/pioneer/PioneerGgmlBackend.h"
#include <llama.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
static void HostOnly()
{
	PioneerGgmlStats stats={};
	assert(PioneerGgmlSnapshot(stats) && !stats.failed && stats.tensors==0 && stats.uploads==0
		&& stats.matvecs==0 && stats.attentionOps==0 && stats.attentionTiles==0);
}
int main(int argc,char** argv)
{
	if (argc!=2) { fprintf(stderr,"Usage: P100AQwenHostTest PINNED_QWEN3_0.6B_GGUF\nNo device probe/decode/settings changes.\n"); return 2; }
	llama_backend_init(); ggml_backend_load_all(); PioneerGgmlContextExecution(false);
	{
		AssistantP100AQwen qwen; std::string error;
		assert(qwen.Load(argv[1],error,false,std::function<bool(float,const char*)>(),true)); HostOnly();
		assert(qwen.SetMode(false,false,error)); HostOnly();
		assert(qwen.SetMode(true,true,error)); HostOnly();
		assert(qwen.SetMode(false,true,error)); HostOnly();
		assert(qwen.SetMode(false,true,error,true)); HostOnly();
		assert(qwen.SetValidation(true,error)); HostOnly();
		assert(qwen.SetValidation(false,error)); HostOnly();
		assert(qwen.SetMode(false,true,error,false)); HostOnly();
		assert(qwen.SetMode(false,false,error)); HostOnly();
		assert(qwen.Unload(error) && qwen.Unload(error) && !qwen.Faulted());
		PioneerGgmlStats stats={}; assert(!PioneerGgmlSnapshot(stats));
	}
	llama_backend_free();
	puts("PASS: HOST ONLY throughput/attention/Fast/checked graph changes, disabled CPU reference backend, idle unload/idempotence; zero uploads/matrices/attention ops. No decode/card/settings/chat operation.");
}
