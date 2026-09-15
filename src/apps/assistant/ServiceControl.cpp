/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
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
		fprintf(stderr, "Usage: assistantctl status | threads N | stop | toggle | ask TEXT | cancel-test TEXT\n");
		return 2;
	}
	if (strcmp(argv[1], "toggle") == 0)
		return BMessenger(kAssistantSignature).SendMessage(kToggle) == B_OK ? 0 : 1;
	if (!service.IsValid()) { fprintf(stderr, "Service is not running.\n"); return 1; }
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
		result = acquire_sem_etc(receiver->done, 1, B_RELATIVE_TIMEOUT, 180000000);
	bool success = result == B_OK && (cancel ? !receiver->success : receiver->success);
	if (receiver->Lock()) receiver->Quit();
	return success ? 0 : 1;
}
