/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
#include "Settings.h"

#include <Application.h>
#include <Autolock.h>
#include <Locker.h>
#include <Messenger.h>

#include <llama.h>
#include <ggml-backend.h>

#include <algorithm>
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <unistd.h>

struct Request {
	BMessenger target;
	int64 id;
	int32 threads;
	std::string prompt;
	std::atomic<bool> cancelled;
	Request() : id(0), cancelled(false) {}
};

static void
Reply(Request* request, uint32 what, const std::string& text,
	const char* status, bool success = false)
{
	BMessage message(what);
	message.AddInt64("id", request->id);
	message.AddString("text", text.c_str());
	message.AddString("status", status);
	message.AddBool("success", success);
	if (request->target.SendMessage(&message, (BHandler*)NULL, 100000) != B_OK)
		request->cancelled.store(true);
}

class AssistantService : public BApplication {
public:
	AssistantService()
		: BApplication(kServiceSignature), fWake(create_sem(0, "assistant requests")),
			fWorker(-1), fRequest(NULL), fQuit(false), fReady(false), fLoads(0),
			fThreads(LoadAssistantThreads()), fActiveThreads(0), fModel(NULL), fContext(NULL)
	{
	}

	virtual void ReadyToRun()
	{
		if (fWake < 0) {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
		fWorker = spawn_thread(Worker, "resident Qwen inference", B_NORMAL_PRIORITY, this);
		if (fWorker < 0 || resume_thread(fWorker) != B_OK) {
			if (fWorker >= 0) kill_thread(fWorker);
			fWorker = -1;
			PostMessage(B_QUIT_REQUESTED);
		}
	}

	virtual bool QuitRequested()
	{
		fQuit.store(true);
		release_sem(fWake);
		if (fWorker >= 0) {
			status_t result;
			wait_for_thread(fWorker, &result);
		}
		delete fRequest;
		fRequest = NULL;
		delete_sem(fWake);
		return true;
	}

	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == kConfigureThreads) {
			int32 threads;
			status_t status = B_BAD_VALUE;
			if (message->FindInt32("cpu_threads", &threads) == B_OK
				&& threads >= 1 && threads <= AssistantMaxThreads()) {
				status = SaveAssistantThreads(threads);
				if (status == B_OK) fThreads.store(threads);
			}
			BMessage reply(kConfigureThreads);
			reply.AddInt32("status", status);
			reply.AddInt32("cpu_threads", fThreads.load());
			message->SendReply(&reply, (BHandler*)NULL, 1000000);
			return;
		}
		if (message->what == kServiceStatus) {
			BAutolock lock(&fLock);
			BMessage reply(kServiceStatus);
			reply.AddBool("ready", fReady.load());
			reply.AddBool("busy", fRequest != NULL);
			reply.AddInt32("model_loads", fLoads.load());
			reply.AddInt32("cpu_threads", fThreads.load());
			reply.AddInt32("active_threads", fActiveThreads.load());
			reply.AddInt32("max_threads", AssistantMaxThreads());
			message->SendReply(&reply);
			return;
		}
		if (message->what == kCancel) {
			BMessenger target;
			int64 id;
			if (message->FindMessenger("target", &target) != B_OK
				|| message->FindInt64("id", &id) != B_OK)
				return;
			BAutolock lock(&fLock);
			if (fRequest != NULL && fRequest->id == id && fRequest->target == target)
				fRequest->cancelled.store(true);
			return;
		}
		if (message->what != kGenerate) {
			BApplication::MessageReceived(message);
			return;
		}
		Request* request = new Request;
		const char* prompt;
		if (message->FindMessenger("target", &request->target) != B_OK
			|| message->FindInt64("id", &request->id) != B_OK
			|| message->FindString("prompt", &prompt) != B_OK) {
			delete request;
			return;
		}
		request->prompt = prompt;
		BAutolock lock(&fLock);
		if (fRequest != NULL || request->prompt.size() > 12000) {
			Reply(request, kDone, "", fRequest != NULL
				? "Service is busy. Try again shortly." : "Prompt is too long.");
			delete request;
			return;
		}
		fRequest = request;
		request->threads = fThreads.load();
		Reply(request, kUpdate, "", fReady.load()
			? "Processing prompt (model already loaded)..." : "Loading background model...");
		release_sem(fWake);
	}

private:
	static int32 Worker(void* data)
	{
		static_cast<AssistantService*>(data)->RunWorker();
		return 0;
	}

	static bool LoadProgress(float, void* data)
	{
		return !static_cast<AssistantService*>(data)->fQuit.load();
	}

	static bool Abort(void* data)
	{
		AssistantService* self = static_cast<AssistantService*>(data);
		BAutolock lock(&self->fLock);
		return self->fQuit.load()
			|| (self->fRequest != NULL && self->fRequest->cancelled.load());
	}

	void RunWorker()
	{
		llama_backend_init();
		ggml_backend_load_all();
		const char* path = getenv("LLAMA_MODEL");
		if (path == NULL) {
			path = "/boot/home/models/Qwen3-0.6B-Q8_0.gguf";
			if (access(path, R_OK) != 0)
				path = "/Haiku1/home/models/Qwen3-0.6B-Q8_0.gguf";
		}
		llama_model_params modelParams = llama_model_default_params();
		modelParams.n_gpu_layers = 0;
		modelParams.progress_callback = LoadProgress;
		modelParams.progress_callback_user_data = this;
		fModel = llama_model_load_from_file(path, modelParams);
		if (fModel != NULL) {
			fLoads.fetch_add(1);
			llama_context_params params = llama_context_default_params();
			params.n_ctx = 4096;
			params.n_batch = 64;
			params.n_ubatch = 64;
			params.n_threads = fThreads.load();
			params.n_threads_batch = params.n_threads;
			fContext = llama_init_from_model(fModel, params);
			if (fContext != NULL)
				llama_set_abort_callback(fContext, Abort, this);
		}
		fReady.store(fContext != NULL);
		fprintf(stderr, "Assistant service: model_loads=%d ready=%d\n",
			fLoads.load(), fReady.load());
		while (!fQuit.load()) {
			if (acquire_sem(fWake) != B_OK || fQuit.load())
				break;
			Request* request;
			{
				BAutolock lock(&fLock);
				request = fRequest;
			}
			if (request == NULL)
				continue;
			std::string answer;
			std::string error;
			bool success = fReady.load() && Complete(request, answer, error);
			if (!fReady.load())
				error = "Model/context load failed. Check service log and restart service.";
			if (request->cancelled.load()) {
				success = false;
				error = "Stopped. Model remains loaded.";
			}
			{
				BAutolock lock(&fLock);
				fRequest = NULL;
			}
			Reply(request, kDone, answer, success
				? "Ready. Model stays loaded in the background." : error.c_str(), success);
			delete request;
		}
		if (fContext != NULL)
			llama_free(fContext);
		if (fModel != NULL)
			llama_model_free(fModel);
		llama_backend_free();
	}

	bool Complete(Request* request, std::string& answer, std::string& error)
	{
		// Only the worker touches the context, between complete requests.
		llama_set_n_threads(fContext, request->threads, request->threads);
		fActiveThreads.store(request->threads);
		const llama_vocab* vocab = llama_model_get_vocab(fModel);
		int count = -llama_tokenize(vocab, request->prompt.c_str(),
			request->prompt.size(), NULL, 0, true, true);
		if (count <= 0 || count + 256 > (int)llama_n_ctx(fContext)) {
			error = "Context is full. Start a new chat or shorten the message.";
			return false;
		}
		std::vector<llama_token> tokens(count);
		if (llama_tokenize(vocab, request->prompt.c_str(), request->prompt.size(),
			tokens.data(), count, true, true) != count) {
			error = "Tokenization failed.";
			return false;
		}
		// Reset request state, not model weights or the allocated context.
		llama_memory_clear(llama_get_memory(fContext), true);
		for (int offset = 0; offset < count; offset += 64) {
			if (Abort(this))
				return false;
			llama_batch batch = llama_batch_get_one(tokens.data() + offset,
				std::min(64, count - offset));
			if (llama_decode(fContext, batch) != 0) {
				error = "Prompt evaluation failed.";
				return false;
			}
			Reply(request, kUpdate, answer, "Processing prompt (resident model)...");
		}
		llama_sampler* sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
		llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
		llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
		llama_sampler_chain_add(sampler, llama_sampler_init_min_p(0.05f, 1));
		llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.8f));
		llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
		bool success = true;
		for (int i = 0; i < 256; i++) {
			if (Abort(this)) {
				success = false;
				break;
			}
			llama_token token = llama_sampler_sample(sampler, fContext, -1);
			if (llama_vocab_is_eog(vocab, token))
				break;
			char piece[1024];
			// Preserve structured tool-call delimiters; end-of-generation is
			// already handled above. The client validates before dispatch.
			int size = llama_token_to_piece(vocab, token, piece, sizeof(piece), 0, true);
			if (size < 0 || answer.size() + size > 65536) {
				error = "Reply exceeded output limit.";
				success = false;
				break;
			}
			answer.append(piece, size);
			Reply(request, kUpdate, answer, "Generating (resident model)...");
			llama_batch batch = llama_batch_get_one(&token, 1);
			if (llama_decode(fContext, batch) != 0) {
				error = "Generation failed.";
				success = false;
				break;
			}
		}
		llama_sampler_free(sampler);
		return success;
	}

	sem_id fWake;
	thread_id fWorker;
	BLocker fLock;
	Request* fRequest;
	std::atomic<bool> fQuit;
	std::atomic<bool> fReady;
	std::atomic<int> fLoads;
	std::atomic<int32> fThreads, fActiveThreads;
	llama_model* fModel;
	llama_context* fContext;
};

int main()
{
	AssistantService service;
	service.Run();
	return 0;
}
