/* SPDX-License-Identifier: MIT */
// Actual Assistant client, one fixed FOUR-token Work request. Not a speed target.
// Launch ONLY after the independent cached host-load/unload parent has CLOSED0
// and a fresh frozen idle-owner/driver/runtime preflight. Never an auto retry.
#ifndef ASSISTANT_QWEN27_WORKER
#error Compile for the isolated cached worker, never the installed Assistant
#endif
#include "../../../src/apps/assistant/P100AQwen27.h"
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <unistd.h>

static double Clock() {
	timespec time{};return clock_gettime(CLOCK_MONOTONIC,&time)?-1:double(time.tv_sec)+double(time.tv_nsec)*1e-9;
}
static bool Statistics(const std::string& text) {
	unsigned tokens=0,samples=0;double total=0,warm=0,rate=0;int end=0;
	const int fields=sscanf(text.c_str(),"P100A Qwen27B Work:%u output tokens,%lfs; warm generation%u samples/%lfs (%lf tok/s). CPU attention/state; CPU validation OFF; weights retained; experimental.%n",
		&tokens,&total,&samples,&warm,&rate,&end);
	const bool valid=fields==5 && end>0 && size_t(end)==text.size() && tokens==4 && samples==3
		&& std::isfinite(total) && std::isfinite(warm) && std::isfinite(rate)
		&& total>=0 && warm>=0 && rate>0;
	if(!valid)return false;
	// Worker reports total to1 decimal, warm/rate to3 decimals. Check the
	// ROUNDING INTERVALS, not an exact ratio of already rounded values;
	// otherwise genuinely faster measurements would be wrongly rejected.
	const double lowWarm=warm>.000501?warm-.000501:0,highWarm=warm+.000501;
	const double lowRate=rate>.000501?rate-.000501:0,highRate=rate+.000501;
	return total+.050001>=lowWarm && highRate*highWarm>=samples && lowRate*lowWarm<=samples;
}
int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--cached-generate-four-only"))return 2;
	AssistantP100AQwen27 client;std::string error;
	std::array<bool,851> verified{};bool cacheBanner=false,converted=false;
	auto loadProgress=[&](float,const char* text) {
		if(!text)return;
		puts(text);fflush(stdout);
		if(!strcmp(text,Assistant27::LosslessCachePreparation))cacheBanner=true;
		unsigned ordinal=0,total=0;
		if(sscanf(text,"Verifying lossless weight cache: tensor %u/%u,",&ordinal,&total)==2 && total==verified.size() && ordinal && ordinal<=total)
			verified[ordinal-1]=true;
		if(!strncmp(text,"Preparing lossless weight cache:",32))converted=true;
	};
	const double began=Clock();if(began<0)return 1;
	puts("CACHED_ASSISTANT_FIXED_FOUR_TOKEN_WORK_REQUEST;NO_CPU_REFERENCE_NO_RETRY");fflush(stdout);
	if(!client.Load("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",4,error,loadProgress)) {
		fprintf(stderr,"CACHE_INFERENCE_HOST_LOAD_REFUSED faulted%u: %s\n",unsigned(client.Faulted()),error.c_str());return 1;
	}
	const double ready=Clock();unsigned count=0;for(bool value:verified)count+=unsigned(value);
	if(!cacheBanner || converted || count!=verified.size() || ready<began) {
		// HostReady is a known boundary; one normal Unload, NEVER Generate.
		const bool clean=client.Unload(error);
		fprintf(stderr,"CACHE_INFERENCE_HOST_NOT_QUALIFIED verified%u converted%u banner%u clean%u faulted%u\n",count,unsigned(converted),unsigned(cacheBanner),unsigned(clean),unsigned(client.Faulted()));return 1;
	}
	printf("CACHED_ASSISTANT_HOST_READY_SECONDS %.6f verified_tensors%u;GPU_UPLOAD_STILL_DEFERRED\n",ready-began,count);fflush(stdout);
	std::string answer,statistics;
#ifdef PIONEER_BATCHED_LOAD_TIMING
	const double requestBegan=Clock();if(requestBegan<0)return 1;
#endif
	auto progress=[&](const std::string&,const char* status) {
		if(status){statistics=status;puts(status);fflush(stdout);}
	};
	if(!client.Complete("Hello.",4,answer,error,[]{return false;},progress,
#ifdef PIONEER_BATCHED_LOAD_TIMING
		[&](float fraction,const char* status){
			if(status && strstr(status,"Qwen27B P100A upload:")){
				const double now=Clock();if(now>=requestBegan)printf("BATCHED_UPLOAD_PROGRESS seconds=%.6f fraction=%.6f\n",now-requestBegan,double(fraction));
				fflush(stdout);
			}
		}
#else
		[](float,const char*){}
#endif
		)) {
		// Do NOT let the client's RAII destructor send Unload after an
		// unproven execution. Closing this process's channel is the ONLY
		// action, including Rejected/Cancelled replies not requested here.
		fprintf(stderr,"CACHE_INFERENCE_EXECUTION_UNPROVEN faulted%u: %s\n",unsigned(client.Faulted()),error.c_str());fflush(stderr);_exit(70);
	}
#ifdef PIONEER_BATCHED_LOAD_TIMING
	const double requestEnded=Clock();
	if(requestEnded>=requestBegan)printf("BATCHED_COMPLETE_SECONDS %.6f;INCLUDES_UPLOAD_PROMPT_AND_GENERATION\n",requestEnded-requestBegan);
#endif
	const bool observed=!client.Faulted() && !answer.empty() && Statistics(statistics);
	// Successful Complete is a known token boundary, even if its diagnostic
	// statistics are unsuitable. One ordinary known-clean Unload is owed.
	const bool clean=client.Unload(error);
	if(!observed || !clean || client.Faulted()) {
		fprintf(stderr,"CACHE_INFERENCE_NOT_QUALIFIED statistics%u clean%u faulted%u: %s\n",unsigned(observed),unsigned(clean),unsigned(client.Faulted()),error.c_str());return 1;
	}
	printf("CACHED_ASSISTANT_FOUR_TOKEN_ANSWER_BYTES %zu\n",answer.size());
	printf("CACHED_ASSISTANT_FOUR_TOKEN_ANSWER %s\n",answer.c_str());
	puts("PASS_ACTUAL_ASSISTANT_CACHED_FOUR_TOKEN_WORK_KNOWN_CLEAN;NOT_FULL_NATIVE_NOT_20TOKENS_SEC_OR_COMPARATIVE_SPEEDUP_PROOF");return 0;
}
