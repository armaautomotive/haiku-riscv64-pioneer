/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Hardware.h"
#include "Protocol.h"
#include "Settings.h"
#include <Application.h>
#include <Looper.h>
#include <Messenger.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

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

int main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-AssistantHardwareTest");
	assert(AssistantHardwareCanChat("cpu"));
	assert(AssistantHardwareCanChat("p100a") && !AssistantHardwareCanChat("auto"));
	assert(!AssistantHardwareCanChat(NULL) && AssistantHardwareIndex(NULL) == -1);
	assert(AssistantHardwareIndex("cpu") == 0 && AssistantHardwareIndex("p100a") == 1);
	BMessage settings;
	assert(AssistantHardwareSetting(settings) == "cpu");
	settings.AddString("backend_id", "p100a");
	assert(AssistantHardwareSetting(settings) == "p100a");
	settings.RemoveName("backend_id"); settings.AddInt32("backend_id", 99);
	assert(!AssistantHardwareCanChat(AssistantHardwareSetting(settings).String()));
	settings.AddString("model_path", "/test/model.gguf"); settings.AddString("unrelated", "keep");
	assert(UpdateAssistantHardwareSettings(settings, "cpu", 1) == B_OK);
	assert(UpdateAssistantHardwareSettings(settings, "p100a", 1) == B_OK);
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
		&& strcmp(argv[1], "--p100a-tests") != 0)) return 2;
	BMessenger service(kServiceSignature);
	if (!service.IsValid()) { puts("SKIP: resident service is not running"); return 77; }
	BMessage state = Query(service);
	bool loading = false, busy = false;
	state.FindBool("loading", &loading); state.FindBool("busy", &busy);
	state.FindInt32("cpu_threads", &threads);
	if (argc == 2 && strcmp(argv[1], "--loading-test") == 0) {
		if (!loading) { puts("SKIP: model already finished loading; run during a fresh load"); return 77; }
		assert(Configure(service, "cpu", threads) == B_BUSY);
		puts("PASS: hardware changes rejected while model is loading"); return 0;
	}
	bool initiallyReady = false; state.FindBool("ready", &initiallyReady);
	if (loading || busy || !initiallyReady) { puts("SKIP: service tests require a ready, idle model"); return 77; }
	const char* initialBackend = ""; state.FindString("backend_id", &initialBackend);
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
