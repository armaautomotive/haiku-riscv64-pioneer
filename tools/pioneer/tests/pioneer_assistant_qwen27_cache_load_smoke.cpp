/* SPDX-License-Identifier: MIT */
// Actual Assistant channel/client, load + unload ONLY. Never send Generate.
#ifndef ASSISTANT_QWEN27_WORKER
#error Compile with an explicit isolated worker; never select the installed Assistant
#endif
#include "../../../src/apps/assistant/P100AQwen27.h"
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <array>
#include <cstdio>
#include <cstring>

int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--load-unload-only"))return 2;
	AssistantP100AQwen27 client;std::string error;
	std::array<bool,851> verified{};bool cacheBanner=false,converted=false;
	auto progress=[&](float,const char* text) {
		if(!text)return;
		puts(text);fflush(stdout);
		if(!strcmp(text,Assistant27::LosslessCachePreparation))cacheBanner=true;
		unsigned ordinal=0,total=0;
		if(sscanf(text,"Verifying lossless weight cache: tensor %u/%u,",&ordinal,&total)==2 && total==verified.size() && ordinal && ordinal<=total)
			verified[ordinal-1]=true;
		if(!strncmp(text,"Preparing lossless weight cache:",32))converted=true;
	};
	puts("CACHE_WORKER_LOAD_UNLOAD_ONLY;NO_GENERATE_REQUEST_NO_GPU_BEGIN_AUTHORITY");fflush(stdout);
	if(!client.Load("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",4,error,progress)) {
		fprintf(stderr,"CACHE_WORKER_LOAD_REFUSED faulted%u: %s\n",unsigned(client.Faulted()),error.c_str());return 1;
	}
	// Even an unexpected worker trace gets one ordinary clean unload after
	// host-ready. Never retry a failed channel or submit inference to test it.
	const bool closed=client.Unload(error);unsigned count=0;
	for(bool value:verified)count+=unsigned(value);
	if(!closed || client.Faulted() || !cacheBanner || converted || count!=verified.size()) {
		fprintf(stderr,"CACHE_WORKER_NOT_QUALIFIED closed%u faulted%u banner%u converted%u verified_tensors%u: %s\n",
			unsigned(closed),unsigned(client.Faulted()),unsigned(cacheBanner),unsigned(converted),count,error.c_str());return 1;
	}
	puts("PASS_ACTUAL_ASSISTANT_CACHE_LOAD_UNLOAD verified_tensors851;NO_GENERATE_NO_WEIGHT_UPLOAD_NO_INFERENCE_SPEED_CLAIM");return 0;
}
