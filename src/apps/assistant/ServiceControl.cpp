/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
#include "ModelStore.h"
#include "Hardware.h"
#include <Application.h>
#include <Looper.h>
#include <Messenger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

class Receiver : public BLooper {
public:
	Receiver() : BLooper("assistant test receiver"), done(create_sem(0, "reply")), success(false) {}
	virtual ~Receiver() { delete_sem(done); }
	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == kDone) {
			const char* text = "";
			const char* status = "";
			message->FindString("text", &text);
			message->FindString("status", &status);
			message->FindBool("success", &success);
			printf("%s\n%s\nsuccess=%d\n", text, status, success);
			release_sem(done);
		}
	}
	sem_id done;
	bool success;
};

int main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-AssistantControl");
	BMessenger service(kServiceSignature);
	if (argc < 2) {
		fprintf(stderr, "Usage: assistantctl status | load | unload | hardware | hardware cpu|p100a|p100a-qwen|p100a-qwen-fast|p100a-qwen-attention [THREADS] | models | install INDEX | verify INDEX | model INDEX | threads N | stop | toggle | ask TEXT | cancel-test TEXT\n");
		return 2;
	}
	if (strcmp(argv[1], "models") == 0) {
		for (size_t i = 0; i < kAssistantModelCount; i++)
			printf("%zu: %s | %llu bytes | %s | %s\n", i, kAssistantModels[i].name,
				(unsigned long long)kAssistantModels[i].bytes,
				AssistantModelPresent(i) ? "installed" : "not installed",
				kAssistantModels[i].compatible ? AssistantModelBackend(i) : "unsupported");
		return 0;
	}
	if ((strcmp(argv[1], "install") == 0 || strcmp(argv[1], "verify") == 0
		|| strcmp(argv[1], "model") == 0) && argc == 3) {
		char* end;
		long index = strtol(argv[2], &end, 10);
		if (end == argv[2] || *end != '\0' || index < 0 || (size_t)index >= kAssistantModelCount) return 2;
		if (strcmp(argv[1], "model") == 0) {
			BMessage request(kSelectModel), reply;
			request.AddInt32("model_index", index);
			status_t status;
			if (service.SendMessage(&request, &reply, 1000000, 1000000) != B_OK
				|| reply.FindInt32("status", &status) != B_OK) return 1;
			printf("select status=%ld (accepted is asynchronous; check status for load result)\n", (long)status);
			return status == B_OK ? 0 : 1;
		}
		std::atomic<bool> cancelled(false);
		BString error;
		status_t status = strcmp(argv[1], "install") == 0
			? InstallAssistantModel(index, cancelled, BMessenger(), error)
			: VerifyAssistantModel(index, AssistantModelPath(index).String(), cancelled, error);
		printf("status=%ld %s\n", (long)status, status == B_OK ? "Verified successfully." : error.String());
		return status == B_OK ? 0 : 1;
	}
	if (strcmp(argv[1], "toggle") == 0)
		return BMessenger(kAssistantSignature).SendMessage(kToggle) == B_OK ? 0 : 1;
	if (!service.IsValid()) { fprintf(stderr, "Service is not running.\n"); return 1; }
	if (argc == 2 && (strcmp(argv[1], "load") == 0 || strcmp(argv[1], "unload") == 0)) {
		BMessage request(strcmp(argv[1], "load") == 0 ? kLoadModel : kUnloadModel), reply;
		status_t status;
		if (service.SendMessage(&request, &reply, 1000000, 1000000) != B_OK
			|| reply.FindInt32("status", &status) != B_OK) return 1;
		const char* explanation = ""; reply.FindString("explanation", &explanation);
		printf("%s status=%ld %s\n", argv[1], (long)status, explanation);
		return status == B_OK ? 0 : 1;
	}
	if (strcmp(argv[1], "hardware") == 0) {
		BMessage query(kServiceStatus), state;
		if (service.SendMessage(&query, &state, 1000000, 1000000) != B_OK) return 1;
		if (argc == 2) {
			const char* selected = "unknown";
			state.FindString("backend_id", &selected); printf("selected=%s\n", selected);
			BMessage entry;
			for (int32 i = 0; state.FindMessage("hardware", i, &entry) == B_OK; i++) {
				const char* id = ""; const char* name = ""; const char* description = ""; bool available = false;
				entry.FindString("id", &id); entry.FindString("name", &name);
				entry.FindString("description", &description); entry.FindBool("available", &available);
				printf("%s: %s [%s]\n%s\n", id, name, available ? "available" : "unavailable", description);
			}
			return state.HasMessage("hardware") ? 0 : 1;
		}
		if (argc != 3 && argc != 4) return 2;
		int32 threads;
		if (state.FindInt32("cpu_threads", &threads) != B_OK) return 1;
		if (argc == 4) {
			char* end; long value = strtol(argv[3], &end, 10);
			if (end == argv[3] || *end != '\0' || value < 1 || value > 4096) return 2;
			threads = value;
		}
		BMessage request(kConfigureHardware), reply;
		request.AddString("backend_id", argv[2]); request.AddInt32("cpu_threads", threads);
		status_t result;
		if (service.SendMessage(&request, &reply, 1000000, 2000000) != B_OK
			|| reply.FindInt32("status", &result) != B_OK) return 1;
		const char* explanation = ""; reply.FindString("explanation", &explanation);
		printf("status=%ld %s\n", (long)result, explanation);
		return result == B_OK ? 0 : 1;
	}
	if (strcmp(argv[1], "stop") == 0)
		return service.SendMessage(B_QUIT_REQUESTED) == B_OK ? 0 : 1;
	if (strcmp(argv[1], "threads") == 0 && argc == 3) {
		char* end;
		long threads = strtol(argv[2], &end, 10);
		if (end == argv[2] || *end != '\0' || threads < 1 || threads > 4096) return 2;
		BMessage request(kConfigureThreads), reply;
		request.AddInt32("cpu_threads", (int32)threads);
		status_t status;
		if (service.SendMessage(&request, &reply, 1000000, 2000000) != B_OK
			|| reply.FindInt32("status", &status) != B_OK) return 1;
		printf("configure status=%ld\n", (long)status);
		return status == B_OK ? 0 : 1;
	}
	if (strcmp(argv[1], "status") == 0) {
		BMessage query(kServiceStatus), reply;
		if (service.SendMessage(&query, &reply, 1000000, 1000000) != B_OK) return 1;
		bool ready = false, busy = false;
		int32 loads = 0;
		reply.FindBool("ready", &ready);
		reply.FindBool("busy", &busy);
		reply.FindInt32("model_loads", &loads);
		printf("team=%ld ready=%d busy=%d model_loads=%ld\n",
			(long)service.Team(), ready, busy, (long)loads);
		int32 threads = 0, active = 0, maximum = 0;
		reply.FindInt32("cpu_threads", &threads);
		reply.FindInt32("active_threads", &active);
		reply.FindInt32("max_threads", &maximum);
		printf("cpu_threads=%ld last_request_threads=%ld max_threads=%ld\n",
			(long)threads, (long)active, (long)maximum);
		bool loading = false;
		const char* path = ""; const char* error = ""; const char* backend = "";
		reply.FindBool("loading", &loading); reply.FindString("model_path", &path);
		reply.FindString("model_error", &error); reply.FindString("backend", &backend);
		printf("loading=%d model=%s\nbackend=%s\nmodel_error=%s\n", loading, path, backend, error);
		const char* selected = ""; const char* activeBackend = "";
		reply.FindString("backend_id", &selected); reply.FindString("active_backend_id", &activeBackend);
		printf("selected_backend=%s active_backend=%s\n", selected, activeBackend);
		bool unloading = false, progress = false; int32 resident = -1; float value = -1; const char* phase = "";
		reply.FindBool("unloading", &unloading); reply.FindInt32("resident_models", &resident);
		reply.FindBool("model_progress_active", &progress); reply.FindFloat("model_progress", &value);
		reply.FindString("model_phase", &phase);
		printf("unloading=%d resident_models=%ld progress_active=%d progress=%.4f phase=%s\n",
			unloading, (long)resident, progress, value, phase);
		return 0;
	}
	bool cancel = strcmp(argv[1], "cancel-test") == 0;
	if (argc != 3 || (!cancel && strcmp(argv[1], "ask") != 0)) return 2;
	Receiver* receiver = new Receiver;
	receiver->Run();
	BMessenger target(receiver);
	int64 id = system_time();
	std::string prompt = "<|im_start|>user\n" + std::string(argv[2])
		+ " /no_think<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
	BMessage request(kGenerate);
	BMessage query(kServiceStatus), state;
	if (service.SendMessage(&query, &state, 1000000, 1000000) != B_OK) return 1;
	const char* backend = ""; const char* path = "";
	state.FindString("backend_id", &backend); state.FindString("model_path", &path);
	request.AddString("backend_id", backend); request.AddString("model_path", path);
	if (strcmp(backend, "p100a") == 0) {
		prompt = argv[2]; request.AddString("prompt_format", "story"); request.AddInt32("max_tokens", 8);
	}
	if (AssistantHardwareIsP100AQwen(backend)) {
		prompt=argv[2]; request.AddString("prompt_format",AssistantHardwareQwenFormat(backend)); request.AddInt32("max_tokens",8);
	}
	request.AddInt64("id", id);
	request.AddMessenger("target", target);
	request.AddString("prompt", prompt.c_str());
	status_t result = service.SendMessage(&request, (BHandler*)NULL, 1000000);
	if (result == B_OK && cancel) {
		snooze(1000000);
		BMessage stop(kCancel);
		stop.AddInt64("id", id);
		stop.AddMessenger("target", target);
		service.SendMessage(&stop, (BHandler*)NULL, 1000000);
	}
	if (result == B_OK)
		result = acquire_sem_etc(receiver->done, 1, B_RELATIVE_TIMEOUT,
			AssistantHardwareIsP100AQwen(backend) ? 2400000000LL : strcmp(backend, "p100a") == 0 ? 900000000 : 180000000);
	bool success = result == B_OK && (cancel ? !receiver->success : receiver->success);
	if (receiver->Lock()) receiver->Quit();
	return success ? 0 : 1;
}
