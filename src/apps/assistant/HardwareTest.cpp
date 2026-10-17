/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Hardware.h"
#include "ModelCatalog.h"
#include "Protocol.h"
#include "Settings.h"
#include <Application.h>
#include <Looper.h>
#include <Messenger.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>

class RejectionReceiver : public BLooper {
public:
	RejectionReceiver() : BLooper("hardware rejection test"), done(create_sem(0, "hardware rejected")), rejected(false), failed(false) {}
	virtual ~RejectionReceiver() { delete_sem(done); }
	virtual void MessageReceived(BMessage* message)
	{
		if (message->what != kDone) return;
		bool success = true; const char* text = "";
		message->FindBool("success", &success); message->FindString("status", &text);
		rejected = !success && strstr(text, "No CPU fallback") != NULL;
		failed = !success;
		release_sem(done);
	}
	sem_id done;
	bool rejected;
	bool failed;
};

static BMessage Query(BMessenger& service)
{
	BMessage query(kServiceStatus), reply;
	assert(service.SendMessage(&query, &reply, 1000000, 1000000) == B_OK);
	return reply;
}

static status_t Configure(BMessenger& service, const char* id, int32 threads)
{
	BMessage request(kConfigureHardware), reply;
	request.AddString("backend_id", id); request.AddInt32("cpu_threads", threads);
	assert(service.SendMessage(&request, &reply, 1000000, 2000000) == B_OK);
	status_t status; assert(reply.FindInt32("status", &status) == B_OK);
	return status;
}

static status_t Memory(BMessenger& service, uint32 action)
{
	BMessage request(action), reply;
	assert(service.SendMessage(&request, &reply, 1000000, 1000000) == B_OK);
	status_t status; assert(reply.FindInt32("status", &status) == B_OK);
	return status;
}

static BMessage WaitIdle(BMessenger& service)
{
	for (int i = 0; i < 6000; i++) {
		BMessage state = Query(service);
		bool loading = true, unloading = true;
		assert(state.FindBool("loading", &loading) == B_OK);
		assert(state.FindBool("unloading", &unloading) == B_OK);
		if (!loading && !unloading) return state;
		bool progress = false; float value = -2; const char* phase = "";
		assert(state.FindBool("model_progress_active", &progress) == B_OK && progress);
		assert(state.FindFloat("model_progress", &value) == B_OK && value >= -1 && value <= 1);
		assert(state.FindString("model_phase", &phase) == B_OK && *phase);
		if (loading) {
			assert(state.FindString("loading_model_path", &phase) == B_OK && *phase);
			assert(state.FindString("loading_backend_id", &phase) == B_OK && AssistantHardwareCanChat(phase));
		}
		snooze(100000);
	}
	assert(!"model lifecycle timed out"); return BMessage();
}

int main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-AssistantHardwareTest");
	assert(AssistantHardwareCanChat("cpu"));
	assert(AssistantHardwareCanChat("p100a") && !AssistantHardwareCanChat("auto"));
	assert(!AssistantHardwareCanChat(NULL) && AssistantHardwareIndex(NULL) == -1);
	assert(AssistantHardwareIndex("cpu") == 0 && AssistantHardwareIndex("p100a") == 1);
	assert(AssistantHardwareIndex("p100a-qwen")==2 && AssistantHardwareCanChat("p100a-qwen"));
	assert(AssistantHardwareIndex("p100a-qwen-fast")==3 && AssistantHardwareCanChat("p100a-qwen-fast"));
	assert(AssistantHardwareIsP100AQwen("p100a-qwen") && AssistantHardwareIsP100AQwen("p100a-qwen-fast"));
	assert(!AssistantHardwareIsP100AQwen(NULL) && !AssistantHardwareIsP100AQwen("cpu"));
	assert(AssistantHardwareIsP100AFast("p100a-qwen-fast") && !AssistantHardwareIsP100AFast("p100a-qwen"));
	assert(!AssistantHardwareIsP100AFast(NULL));
	assert(AssistantHardwareIndex("p100a-qwen-attention")==4 && AssistantHardwareCanChat("p100a-qwen-attention"));
	assert(AssistantHardwareIndex("p100a-qwen-throughput")==5 && AssistantHardwareCanChat("p100a-qwen-throughput"));
	assert(AssistantHardwareIsP100AQwen("p100a-qwen-throughput") && AssistantHardwareIsP100AAttention("p100a-qwen-throughput"));
	assert(AssistantHardwareIsP100AThroughput("p100a-qwen-throughput") && !AssistantHardwareIsP100AThroughput(NULL) && !AssistantHardwareIsP100AThroughput("p100a-qwen-attention"));
	assert(AssistantHardwareSkipsValidation("p100a-qwen-throughput"));
	assert(!strcmp(AssistantHardwareQwenFormat("p100a-qwen-throughput"),"qwen-p100a-throughput"));
	assert(AssistantHardwareIsP100AQwen("p100a-qwen-attention") && AssistantHardwareIsP100AAttention("p100a-qwen-attention"));
	assert(!AssistantHardwareIsP100AAttention(NULL) && !AssistantHardwareIsP100AAttention("p100a-qwen-fast"));
	assert(AssistantHardwareSkipsValidation("p100a-qwen-attention") && AssistantHardwareSkipsValidation("p100a-qwen-fast"));
	assert(!AssistantHardwareSkipsValidation("p100a-qwen") && !AssistantHardwareSkipsValidation("cpu"));
	assert(!strcmp(AssistantHardwareQwenFormat("p100a-qwen-attention"),"qwen-p100a-attention"));
	assert(!strcmp(AssistantHardwareQwenFormat("p100a-qwen-fast"),"qwen-p100a-fast"));
	assert(!strcmp(AssistantHardwareQwenFormat("p100a-qwen"),"qwen-p100a"));
	assert(AssistantHardwareQwenFormat("cpu")==NULL && AssistantHardwareQwenFormat(NULL)==NULL);
	assert(AssistantModelSupportsBackend(0,"cpu") && AssistantModelSupportsBackend(0,"p100a-qwen"));
	assert(!AssistantModelSupportsBackend(1,"p100a-qwen") && !AssistantModelSupportsBackend(3,"p100a-qwen"));
	assert(!AssistantModelSupportsBackend(kAssistantTinyModel,"p100a-qwen"));
	assert(AssistantModelSupportsBackend(0,"p100a-qwen-fast"));
	assert(AssistantModelSupportsBackend(0,"p100a-qwen-attention"));
	assert(AssistantModelSupportsBackend(0,"p100a-qwen-throughput"));
	for (size_t index=1;index<kAssistantModelCount;index++) assert(!AssistantModelSupportsBackend(index,"p100a-qwen-throughput"));
	for (size_t index=1;index<kAssistantModelCount;index++) assert(!AssistantModelSupportsBackend(index,"p100a-qwen-attention"));
	for (size_t index=1;index<kAssistantModelCount;index++) assert(!AssistantModelSupportsBackend(index,"p100a-qwen-fast"));
	assert(!AssistantModelSupportsBackend(0,NULL) && !AssistantModelSupportsBackend(kAssistantModelCount,"p100a-qwen"));
	BMessage settings;
	assert(AssistantHardwareSetting(settings) == "cpu");
	settings.AddString("backend_id", "p100a");
	assert(AssistantHardwareSetting(settings) == "p100a");
	settings.RemoveName("backend_id"); settings.AddInt32("backend_id", 99);
	assert(!AssistantHardwareCanChat(AssistantHardwareSetting(settings).String()));
	settings.AddString("model_path", "/test/model.gguf"); settings.AddString("unrelated", "keep");
	assert(UpdateAssistantHardwareSettings(settings, "cpu", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "p100a", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "p100a-qwen", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "p100a-qwen-fast", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "p100a-qwen-attention", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "p100a-qwen-throughput", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "cpu", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "unknown", 1) == B_BAD_VALUE);
	assert(UpdateAssistantHardwareSettings(settings, "cpu", 0) == B_BAD_VALUE);
	assert(UpdateAssistantHardwareSettings(settings, "cpu", AssistantMaxThreads() + 1) == B_BAD_VALUE);
	const char* value; int32 threads;
	assert(settings.FindString("backend_id", &value) == B_OK && strcmp(value, "cpu") == 0);
	assert(settings.FindString("model_path", &value) == B_OK && strcmp(value, "/test/model.gguf") == 0);
	assert(settings.FindString("unrelated", &value) == B_OK && strcmp(value, "keep") == 0);
	assert(settings.FindInt32("cpu_threads", &threads) == B_OK && threads == 1);
	puts("PASS: hardware policy, fail-closed saved IDs, thread limits and field preservation (no settings written)");
	if (argc == 1) return 0;
	if (argc != 2 || (strcmp(argv[1], "--loading-test") != 0 && strcmp(argv[1], "--service-tests") != 0
		&& strcmp(argv[1], "--p100a-tests") != 0 && strcmp(argv[1],"--p100a-qwen-tests")!=0
		&& strcmp(argv[1],"--qwen-mode-switch-tests")!=0 && strcmp(argv[1],"--memory-tests")!=0
		&& strcmp(argv[1],"--busy-memory-test")!=0)) return 2;
	BMessenger service(kServiceSignature);
	if (!service.IsValid()) { puts("SKIP: resident service is not running"); return 77; }
	BMessage state = Query(service);
	bool loading = false, busy = false;
	state.FindBool("loading", &loading); state.FindBool("busy", &busy);
	state.FindInt32("cpu_threads", &threads);
	if (!strcmp(argv[1], "--busy-memory-test")) {
		if (!busy) { puts("SKIP: requires an active reply"); return 77; }
		assert(Memory(service, kLoadModel) == B_BUSY && Memory(service, kUnloadModel) == B_BUSY);
		puts("PASS: load/unload rejected while inference owns the model/device"); return 0;
	}
	if (argc == 2 && strcmp(argv[1], "--loading-test") == 0) {
		if (!loading) { puts("SKIP: model already finished loading; run during a fresh load"); return 77; }
		assert(Configure(service, "cpu", threads) == B_BUSY);
		assert(Configure(service, "p100a-qwen-fast", threads) == B_BUSY);
		assert(Memory(service, kLoadModel) == B_BUSY && Memory(service, kUnloadModel) == B_BUSY);
		puts("PASS: hardware changes rejected while model is loading"); return 0;
	}
	bool initiallyReady = false; state.FindBool("ready", &initiallyReady);
	if (loading || busy || !initiallyReady) { puts("SKIP: service tests require a ready, idle model"); return 77; }
	const char* initialBackend = ""; state.FindString("backend_id", &initialBackend);
	if (!strcmp(argv[1], "--memory-tests")) {
		BString model, backend(initialBackend);
		assert(state.FindString("model_path", &model) == B_OK);
		int32 loads = 0, resident = 0;
		assert(state.FindInt32("model_loads", &loads) == B_OK);
		assert(state.FindInt32("resident_models", &resident) == B_OK && resident > 0);
		BString savedModel = LoadAssistantModel(), savedHardware = LoadAssistantHardware();
		int32 savedThreads = LoadAssistantThreads();
		assert(Memory(service, kLoadModel) == B_OK); // Already loaded: no reload.
		assert(Memory(service, kUnloadModel) == B_OK);
		BMessage empty = WaitIdle(service);
		bool ready = true, progress = true;
		assert(empty.FindBool("ready", &ready) == B_OK && !ready);
		assert(empty.FindInt32("resident_models", &resident) == B_OK && resident == 0);
		assert(empty.FindBool("model_progress_active", &progress) == B_OK && !progress);
		assert(empty.FindString("active_backend_id", &value) == B_OK && !*value);
		assert(empty.FindString("model_path", &value) == B_OK && model == value);
		assert(empty.FindString("backend_id", &value) == B_OK && backend == value);
		assert(empty.FindInt32("model_loads", &resident) == B_OK && resident == loads);
		assert(Memory(service, kUnloadModel) == B_OK); // Idempotent, no device reopen.
		RejectionReceiver* receiver = new RejectionReceiver; receiver->Run();
		BMessage request(kGenerate);
		request.AddMessenger("target", BMessenger(receiver)); request.AddInt64("id", system_time());
		request.AddString("backend_id", backend); request.AddString("model_path", model);
		request.AddString("prompt", "This unloaded-model test must never execute.");
		request.AddString("prompt_format", AssistantHardwareIsP100AQwen(backend.String())
			? AssistantHardwareQwenFormat(backend.String()) : "story");
		assert(service.SendMessage(&request) == B_OK);
		assert(acquire_sem_etc(receiver->done, 1, B_RELATIVE_TIMEOUT, 2000000) == B_OK && receiver->failed);
		if (receiver->Lock()) receiver->Quit();
		assert(LoadAssistantModel() == savedModel && LoadAssistantHardware() == savedHardware
			&& LoadAssistantThreads() == savedThreads);
		assert(Memory(service, kLoadModel) == B_OK);
		BMessage after = WaitIdle(service);
		assert(after.FindBool("ready", &ready) == B_OK && ready);
		assert(after.FindInt32("resident_models", &resident) == B_OK && resident == 1);
		assert(after.FindInt32("model_loads", &resident) == B_OK && resident == loads + 1);
		assert(after.FindString("model_path", &value) == B_OK && model == value);
		assert(after.FindString("backend_id", &value) == B_OK && backend == value);
		assert(after.FindInt32("cpu_threads", &resident) == B_OK && resident == threads);
		assert(after.FindBool("model_progress_active", &progress) == B_OK && !progress);
		assert(LoadAssistantModel() == savedModel && LoadAssistantHardware() == savedHardware
			&& LoadAssistantThreads() == savedThreads);
		puts("PASS: unload frees all caches, no implicit inference reload, idempotent memory commands, reload/progress completion and saved selection/thread preservation");
		return 0;
	}
	if (!strcmp(argv[1],"--qwen-mode-switch-tests")) {
		if (!AssistantHardwareIsP100AQwen(initialBackend)) { puts("SKIP: a Qwen/P100A mode must be selected"); return 77; }
		std::string original=initialBackend;
		const char* other=AssistantHardwareIsP100AFast(initialBackend)?"p100a-qwen":"p100a-qwen-fast";
		BString model; assert(state.FindString("model_path",&model)==B_OK);
		int32 loads=0; assert(state.FindInt32("model_loads",&loads)==B_OK);
		for (const char* target:{other,original.c_str()}) {
			assert(Configure(service,target,threads)==B_OK);
			bool finished=false;
			for (int poll=0;poll<200;poll++) {
				BMessage after=Query(service); bool loadingNow=true;
				after.FindBool("loading",&loadingNow);
				if (!loadingNow) {
					bool ready=false; int32 afterLoads=-1,afterThreads=0;
					assert(after.FindBool("ready",&ready)==B_OK && ready);
					assert(after.FindString("backend_id",&value)==B_OK && !strcmp(value,target));
					assert(after.FindString("model_path",&value)==B_OK && model==value);
					assert(after.FindInt32("model_loads",&afterLoads)==B_OK && afterLoads==loads);
					assert(after.FindInt32("cpu_threads",&afterThreads)==B_OK && afterThreads==threads);
					finished=true; break;
				}
				snooze(100000);
			}
			assert(finished);
		}
		puts("PASS: explicit fast/validated round-trip retains model, load count and CPU threads without inference"); return 0;
	}
	if (!strcmp(argv[1],"--p100a-qwen-tests")) {
		if (!AssistantHardwareIsP100AQwen(initialBackend)) { puts("SKIP: experimental P100A Qwen must be selected/idle"); return 77; }
		bool fast=AssistantHardwareSkipsValidation(initialBackend);
		const char* format=AssistantHardwareQwenFormat(initialBackend);
		const char* model=""; assert(state.FindString("model_path",&model)==B_OK);
		RejectionReceiver* receiver=new RejectionReceiver; receiver->Run();
		for (int test=0;test<12;test++) {
			BMessage request(kGenerate); request.AddMessenger("target",BMessenger(receiver)); request.AddInt64("id",system_time());
			if (test!=0) request.AddString("backend_id",test==1?"cpu":test==10?(fast?"p100a-qwen":"p100a-qwen-fast"):initialBackend);
			if (test!=2) request.AddString("model_path",test==3?"/wrong/model.gguf":model);
			if (test!=4) request.AddString("prompt_format",test==5?"story":test==11?(fast?"qwen-p100a":"qwen-p100a-fast"):format);
			request.AddString("prompt",test==8?std::string(513,'x').c_str():test==9?"":"Public rejection test");
			if (test==6) request.AddInt32("max_tokens",17);
			if (test==7) request.AddString("max_tokens","8");
			assert(service.SendMessage(&request)==B_OK);
			assert(acquire_sem_etc(receiver->done,1,B_RELATIVE_TIMEOUT,2000000)==B_OK && receiver->failed);
			if (test!=3) assert(receiver->rejected);
		}
		if (receiver->Lock()) receiver->Quit();
		BMessage after=Query(service); bool ready=false; after.FindBool("ready",&ready); assert(ready);
		assert(after.FindString("backend_id",&value)==B_OK && !strcmp(value,initialBackend));
		puts("PASS: P100A Qwen rejects missing/wrong hardware/model/format, token limit/type and question bounds; no inference or CPU fallback"); return 0;
	}
	if (strcmp(argv[1], "--p100a-tests") == 0) {
		if (strcmp(initialBackend, "p100a")) { puts("SKIP: P100A must be selected and idle"); return 77; }
		RejectionReceiver* receiver = new RejectionReceiver; receiver->Run();
		for (int test=0; test<6; test++) {
			BMessage request(kGenerate);
			request.AddMessenger("target", BMessenger(receiver)); request.AddInt64("id", system_time());
			request.AddString("backend_id", test==0 ? "cpu" : "p100a");
			request.AddString("prompt", test==4 ? "bad\ninput" : "Once");
			if (test!=1) request.AddString("prompt_format", "story");
			if (test==2) request.AddInt32("max_tokens",17);
			if (test==3) request.AddString("max_tokens","8");
			if (test==5) request.AddString("model_path","/not/the/selected/model");
			assert(service.SendMessage(&request) == B_OK);
			assert(acquire_sem_etc(receiver->done,1,B_RELATIVE_TIMEOUT,2000000)==B_OK && receiver->failed);
			if (test<4) assert(receiver->rejected);
		}
		if (receiver->Lock()) receiver->Quit();
		BMessage after=Query(service); bool ready=false; after.FindBool("ready",&ready); assert(ready);
		assert(after.FindString("backend_id",&value)==B_OK && strcmp(value,"p100a")==0);
		puts("PASS: P100A service rejects wrong hardware, non-story format, token limits/types, non-ASCII input and wrong model without CPU fallback");
		return 0;
	}
	if (strcmp(initialBackend, "cpu")) { puts("SKIP: rejection tests require CPU selected"); return 77; }
	int32 loads = 0; state.FindInt32("model_loads", &loads);
	BString model; assert(state.FindString("model_path", &model) == B_OK);
	assert(Configure(service, "invalid", threads) == B_BAD_VALUE);
	assert(Configure(service, "cpu", 0) == B_BAD_VALUE);
	assert(Configure(service, "cpu", threads) == B_OK);
	BMessage malformed(kConfigureHardware), invalidReply;
	malformed.AddString("backend_id", "cpu"); malformed.AddString("cpu_threads", "invalid");
	assert(service.SendMessage(&malformed, &invalidReply, 1000000, 1000000) == B_OK);
	status_t invalidStatus; assert(invalidReply.FindInt32("status", &invalidStatus) == B_OK && invalidStatus == B_BAD_VALUE);
	BMessage threadRequest(kConfigureThreads), threadReply;
	threadRequest.AddInt32("cpu_threads", threads);
	assert(service.SendMessage(&threadRequest, &threadReply, 1000000, 2000000) == B_OK);
	assert(threadReply.FindInt32("status", &invalidStatus) == B_OK && invalidStatus == B_OK);
	BMessage saved; LoadAssistantSettings(saved);
	assert(saved.FindString("backend_id", &value) == B_OK && strcmp(value, "cpu") == 0);
	assert(LoadAssistantHardware() == "cpu" && LoadAssistantModel() == model && LoadAssistantThreads() == threads);
	BMessage after = Query(service);
	int32 afterLoads = -1; after.FindInt32("model_loads", &afterLoads); assert(afterLoads == loads);
	assert(after.FindString("backend_id", &value) == B_OK && strcmp(value, "cpu") == 0);
	bool ready = false; after.FindBool("ready", &ready); assert(ready);
	BMessage entry;
	assert(after.FindMessage("hardware", 1, &entry) == B_OK);
	bool available = false; assert(entry.FindBool("available", &available) == B_OK);
	RejectionReceiver* receiver = new RejectionReceiver; receiver->Run();
	BMessage generate(kGenerate);
	generate.AddMessenger("target", BMessenger(receiver)); generate.AddInt64("id", system_time());
	generate.AddString("prompt", "This must never reach CPU inference."); generate.AddString("backend_id", "p100a");
	assert(service.SendMessage(&generate) == B_OK);
	assert(acquire_sem_etc(receiver->done, 1, B_RELATIVE_TIMEOUT, 2000000) == B_OK && receiver->rejected);
	generate.RemoveName("backend_id"); generate.AddInt32("backend_id", 1);
	assert(service.SendMessage(&generate) == B_OK);
	assert(acquire_sem_etc(receiver->done, 1, B_RELATIVE_TIMEOUT, 2000000) == B_OK && receiver->rejected);
	if (receiver->Lock()) receiver->Quit();
	puts("PASS: service capabilities, CPU save without reload, model/thread preservation, unsupported selection and generation rejection without CPU fallback");
	return 0;
}
