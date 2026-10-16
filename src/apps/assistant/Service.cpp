/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
#include "Settings.h"
#include "ModelStore.h"
#include "P100ABackend.h"
#include "P100AQwen.h"
#include "../../../tools/pioneer/PioneerGgmlBackend.h"

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
	int32 tokens;
	std::string prompt;
	std::string summary;
	std::atomic<bool> cancelled;
	Request() : id(0), tokens(8), cancelled(false) {}
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
			fThreads(LoadAssistantThreads()), fActiveThreads(0), fModel(NULL), fContext(NULL),
			fLoading(true), fPersistModel(false), fTiny(NULL), fP100ASupported(false), fQwen(NULL), fP100AQwenSupported(false)
	{
		fHardware = LoadAssistantHardware();
		const char* overridePath = getenv("LLAMA_MODEL");
		fPendingPath = overridePath != NULL ? BString(overridePath) : LoadAssistantModel();
		if (fPendingPath.IsEmpty()) {
			fPendingPath = AssistantModelPath(0);
			if (access(fPendingPath.String(), R_OK) != 0)
				fPendingPath = "/Haiku1/home/models/Qwen3-0.6B-Q8_0.gguf";
		}
		fPendingHardware = fHardware;
		fPendingThreads = fThreads.load();
		BMessage settings; LoadAssistantSettings(settings);
		const char* lastCPU;
		fStartupPath = settings.FindString("last_cpu_model_path", &lastCPU) == B_OK
			? BString(lastCPU) : AssistantModelPath(0);
		if (fHardware=="cpu" && AssistantModelIndex(fPendingPath.String()) >= 0
			&& AssistantModelIndex(fPendingPath.String()) != (int)kAssistantTinyModel)
			fStartupPath = fPendingPath;
	}

	virtual void ReadyToRun()
	{
		if (fWake < 0) {
			PostMessage(B_QUIT_REQUESTED);
			return;
		}
		fWorker = spawn_thread(Worker, "resident assistant inference", B_NORMAL_PRIORITY, this);
		if (fWorker < 0 || resume_thread(fWorker) != B_OK) {
			if (fWorker >= 0) kill_thread(fWorker);
			fWorker = -1;
			PostMessage(B_QUIT_REQUESTED);
		}
		release_sem(fWake);
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
		if (message->what == kConfigureHardware) {
			BAutolock lock(&fLock);
			const char* id = NULL;
			int32 threads = fThreads.load();
			int32 requestedThreads;
			status_t threadStatus = message->FindInt32("cpu_threads", &requestedThreads);
			if (threadStatus == B_OK) threads = requestedThreads;
			status_t status = B_BAD_VALUE;
			BString explanation("Invalid hardware selection or CPU-thread count.");
			if (message->FindString("backend_id", &id) == B_OK && AssistantHardwareIndex(id) >= 0) {
				if (fLoading || fRequest != NULL) {
					status = B_BUSY; explanation = "Finish/stop the current reply or wait for model loading, then try again.";
				} else if (threadStatus == B_OK || threadStatus == B_NAME_NOT_FOUND) {
					BString path = strcmp(id, "p100a-qwen") == 0 ? AssistantModelPath(0)
						: strcmp(id, "p100a") == 0 ? AssistantModelPath(kAssistantTinyModel)
						: (fHardware == "cpu" && !fModelPath.IsEmpty() ? fModelPath : fStartupPath);
					status = QueueSelection(path, id, threads);
					explanation = status == B_OK
						? "Verifying model/hardware pair; saved only after success. P100A Qwen uses 0.6B only, is slower than CPU, and first-use upload can take 15 minutes."
						: "Could not select hardware. Install its supported model and check driver availability; previous selection kept.";
				}
			}
			BMessage reply(kConfigureHardware);
			reply.AddInt32("status", status); reply.AddString("explanation", explanation);
			reply.AddString("backend_id", fHardware); reply.AddInt32("cpu_threads", fThreads.load());
			message->SendReply(&reply);
			return;
		}
		if (message->what == kConfigureThreads) {
			BAutolock lock(&fLock);
			int32 threads;
			status_t status = B_BAD_VALUE;
			if (message->FindInt32("cpu_threads", &threads) == B_OK
				&& threads >= 1 && threads <= AssistantMaxThreads()) {
				status = fLoading ? B_BUSY : SaveAssistantThreads(threads);
				if (status == B_OK) fThreads.store(threads);
			}
			BMessage reply(kConfigureThreads);
			reply.AddInt32("status", status);
			reply.AddInt32("cpu_threads", fThreads.load());
			message->SendReply(&reply, (BHandler*)NULL, 1000000);
			return;
		}
		if (message->what == kSelectModel) {
			BAutolock lock(&fLock);
			int32 index = -1;
			status_t status = B_BAD_VALUE;
			if (message->FindInt32("model_index", &index) == B_OK && index >= 0
				&& (size_t)index < kAssistantModelCount && kAssistantModels[index].compatible) {
				status = QueueSelection(AssistantModelPath(index), AssistantModelBackend(index), fThreads.load());
			}
			BMessage reply(kSelectModel);
			reply.AddInt32("status", status);
			message->SendReply(&reply);
			return;
		}
		if (message->what == kServiceStatus) {
			BAutolock lock(&fLock);
			BMessage reply(kServiceStatus);
			reply.AddBool("ready", fReady.load());
			reply.AddBool("busy", fRequest != NULL);
			reply.AddBool("loading", fLoading);
			reply.AddString("model_path", fModelPath);
			reply.AddString("model_error", fModelError);
			reply.AddString("backend", fHardware == "p100a-qwen" ? "P100A Qwen3-0.6B exact dual-reuse hybrid (experimental, slower than CPU)"
				: fHardware == "p100a" ? "P100A TinyStories experimental hybrid (not Qwen)"
				: (fHardware == "cpu" ? "CPU (llama.cpp)" : "Unavailable hardware; no fallback"));
			reply.AddString("backend_id", fHardware);
			reply.AddString("active_backend_id", fReady.load() ? fHardware.String() : "");
			for (size_t i = 0; i < kAssistantHardwareCount; i++) {
				BMessage hardware;
				hardware.AddString("id", kAssistantHardware[i].id);
				hardware.AddString("name", kAssistantHardware[i].name);
				hardware.AddBool("available", i == 0 || (i==1 && fP100ASupported.load() && AssistantModelPresent(kAssistantTinyModel))
					|| (i==2 && fP100AQwenSupported.load() && AssistantModelPresent(0)));
				hardware.AddString("description", kAssistantHardware[i].description);
				reply.AddMessage("hardware", &hardware);
			}
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
		const char* expected;
		status_t backendStatus = message->FindString("backend_id", &expected);
		if (!AssistantHardwareCanChat(fHardware.String())
			|| (backendStatus != B_NAME_NOT_FOUND && (backendStatus != B_OK || fHardware != expected))) {
			Reply(request, kDone, "", "Requested hardware is unavailable or changed. No CPU fallback was used.");
			delete request; return;
		}
		if (message->FindString("model_path", &expected) == B_OK && fModelPath != expected) {
			Reply(request, kDone, "", "Model changed. Start a new chat and try again.");
			delete request; return;
		}
		if (fHardware == "p100a") {
			const char* format = NULL;
			status_t tokensStatus = message->FindInt32("max_tokens", &request->tokens);
			if (message->FindString("prompt_format", &format) != B_OK || strcmp(format, "story")
				|| (tokensStatus != B_OK && tokensStatus != B_NAME_NOT_FOUND)
				|| request->tokens < 1 || request->tokens > 16 || !fP100ASupported.load()) {
				Reply(request, kDone, "", "P100A requires explicit story-completion mode (1–16 tokens). No CPU fallback.");
				delete request; return;
			}
		}
		if (fHardware == "p100a-qwen") {
			const char* format=NULL;
			const char* pinnedModel=NULL;
			status_t tokensStatus=message->FindInt32("max_tokens",&request->tokens);
			if (backendStatus!=B_OK || message->FindString("model_path",&pinnedModel)!=B_OK || fModelPath!=pinnedModel
				|| message->FindString("prompt_format",&format)!=B_OK || strcmp(format,"qwen-p100a")
				|| (tokensStatus!=B_OK && tokensStatus!=B_NAME_NOT_FOUND) || request->tokens<1 || request->tokens>16
				|| request->prompt.empty() || request->prompt.size()>512 || !fP100AQwenSupported.load()) {
				Reply(request,kDone,"","P100A Qwen requires explicit short-question mode, 512 bytes and 1-16 output tokens. No CPU fallback.");
				delete request; return;
			}
		}
		if (fLoading || fRequest != NULL || request->prompt.size() > 12000) {
			Reply(request, kDone, "", (fLoading || fRequest != NULL)
				? "Service is loading or busy. Try again shortly." : "Prompt is too long.");
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
	// Caller holds fLock. Selection is an atomic model/backend pair, committed
	// only after hash verification and successful loading on the worker.
	status_t QueueSelection(const BString& path, const char* backend, int32 threads)
	{
		if (fLoading || fRequest != NULL) return B_BUSY;
		int index = AssistantModelIndex(path.String());
		if (index < 0 || !kAssistantModels[index].compatible
			|| !AssistantModelSupportsBackend(index,backend)
			|| threads < 1 || threads > AssistantMaxThreads()) return B_BAD_VALUE;
		if (strcmp(backend, "p100a") == 0 && !fP100ASupported.load()) return B_NOT_SUPPORTED;
		if (strcmp(backend, "p100a-qwen") == 0 && !fP100AQwenSupported.load()) return B_NOT_SUPPORTED;
		if (!AssistantModelPresent(index)) return B_ENTRY_NOT_FOUND;
		if (fReady.load() && fHardware == backend && fModelPath == path) {
			status_t status = SaveAssistantHardware(backend, threads);
			if (status == B_OK) fThreads.store(threads);
			return status;
		}
		fPendingPath = path; fPendingHardware = backend; fPendingThreads = threads;
		fLoading = true; fPersistModel = true; fModelError = "";
		release_sem(fWake); return B_OK;
	}

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
		PioneerGgmlContextExecution(false);
		fP100ASupported.store(AssistantP100A::Available());
		fP100AQwenSupported.store(AssistantP100AQwen::Available());
		while (!fQuit.load()) {
			if (acquire_sem(fWake) != B_OK || fQuit.load()) break;
			BString path, backend;
			int32 threads;
			Request* request;
			{
				BAutolock lock(&fLock);
				path = fPendingPath;
				backend = fPendingHardware; threads = fPendingThreads;
				fPendingPath = "";
				request = fRequest;
			}
			if (!path.IsEmpty()) { LoadModel(path, backend, threads); continue; }
			if (request == NULL) continue;
			std::string answer, error;
			bool success = false;
			if (fReady.load()) {
				if (fHardware == "p100a") {
					fActiveThreads.store(1);
					success = fTiny != NULL && fTiny->Complete(request->prompt, request->tokens, answer, error,
						[&]() { return fQuit.load() || request->cancelled.load(); },
						[&](const std::string& text, const char* status) {
							if (strncmp(status, "P100A:", 6) == 0) request->summary = status;
							Reply(request, kUpdate, text, status);
						});
					if (fTiny != NULL && fTiny->Faulted()) {
						BAutolock lock(&fLock);
						fP100ASupported.store(false); fReady.store(false); fModelError = error.c_str();
						fP100AQwenSupported.store(false);
					}
				} else if (fHardware == "p100a-qwen") {
					fActiveThreads.store(1);
					success=fQwen && fQwen->Complete(request->prompt,request->tokens,answer,error,
						[&]() { return fQuit.load() || request->cancelled.load(); },
						[&](const std::string& text,const char* status) {
							if (!strncmp(status,"P100A Qwen:",11)) request->summary=status;
							Reply(request,kUpdate,text,status);
						});
					if (fQwen && fQwen->Faulted()) {
						BAutolock lock(&fLock); fP100AQwenSupported.store(false); fReady.store(false); fModelError=error.c_str();
						fP100ASupported.store(false);
					}
				} else success = Complete(request, answer, error);
			}
			if (!fReady.load() && error.empty()) error = "No model is ready. Open Models to select or retry a model.";
			if (request->cancelled.load()) {
				success = false;
				// Do not replace a device/cleanup failure with a reassuring
				// cancellation message when Stop races with that failure.
				if (error.empty()) error = fHardware != "cpu"
					? "Stopped safely between P100A batches. No CPU fallback."
					: "Stopped. Model remains loaded.";
			}
			{
				BAutolock lock(&fLock);
				fRequest = NULL;
			}
			Reply(request, kDone, answer, success
				? (fHardware != "cpu" ? request->summary.c_str()
					: "Ready. Model stays loaded in the background.") : error.c_str(), success);
			delete request;
		}
		if (fContext != NULL) llama_free(fContext);
		if (fModel != NULL) llama_model_free(fModel);
		delete fTiny; fTiny = NULL;
		delete fQwen; fQwen=NULL;
		llama_backend_free();
	}

	void LoadModel(const BString& path, const BString& backend, int32 threads)
	{
		{
			BAutolock lock(&fLock);
			if (!AssistantHardwareCanChat(backend.String())) {
				fLoading = false;
				fModelError = "Saved hardware backend is unavailable. Open Hardware and explicitly choose CPU.";
				return;
			}
		}
		BString error;
		int index = AssistantModelIndex(path.String());
		status_t status = B_NOT_SUPPORTED;
		if (index >= 0 && kAssistantModels[index].compatible
			&& AssistantModelSupportsBackend(index,backend.String()))
			status = VerifyAssistantModel(index, path.String(), fQuit, error);
		else error = "Model is outside the supported catalog. Open Models to choose a supported Qwen3 model.";
		llama_model* model = NULL;
		llama_context* context = NULL;
		AssistantP100A* tiny = NULL;
		AssistantP100AQwen* qwen=NULL;
		if (status==B_OK && backend=="p100a-qwen") {
			if (!fP100AQwenSupported.load()) error="P100A exact Qwen driver unavailable or disabled after a fault; no fallback.";
			else {
				qwen=new AssistantP100AQwen; std::string reason;
				if (!qwen->Load(path.String(),reason)) { error=reason.c_str(); delete qwen; qwen=NULL; }
			}
		}
		if (status == B_OK && backend == "p100a") {
			if (!fP100ASupported.load()) error = "P100A driver unavailable or disabled after a fault; no fallback.";
			else {
				tiny = new AssistantP100A;
				std::string reason;
				if (!tiny->Load(path.String(), AssistantTinyTokenizerPath().String(), reason)) {
					error = reason.c_str(); delete tiny; tiny = NULL;
				}
			}
		}
		llama_model_params modelParams = llama_model_default_params();
		modelParams.n_gpu_layers = 0;
		modelParams.progress_callback = LoadProgress;
		modelParams.progress_callback_user_data = this;
		if (status == B_OK && backend == "cpu" && !fQuit.load()) model = llama_model_load_from_file(path.String(), modelParams);
		if (model != NULL) {
			fLoads.fetch_add(1);
			llama_context_params params = llama_context_default_params();
			params.n_ctx = 4096;
			params.n_batch = 64;
			params.n_ubatch = 64;
			params.n_threads = threads;
			params.n_threads_batch = params.n_threads;
			params.op_offload = false; // explicit CPU mode, even after P100A registration
			context = llama_init_from_model(model, params);
			if (context != NULL) llama_set_abort_callback(context, Abort, this);
		}
		BAutolock lock(&fLock);
		bool loaded = context != NULL || tiny != NULL || qwen != NULL;
		if (loaded && fPersistModel) {
			BMessage settings; LoadAssistantSettings(settings);
			status_t saved = UpdateAssistantHardwareSettings(settings, backend.String(), threads);
			settings.RemoveName("model_path"); settings.AddString("model_path", path);
			BString lastCPU = backend == "cpu" ? path : fStartupPath;
			settings.RemoveName("last_cpu_model_path"); settings.AddString("last_cpu_model_path", lastCPU);
			if (saved != B_OK || SaveAssistantSettings(settings) != B_OK)
				error = "Could not save model/hardware pair; previous selection kept.";
		}
		if (loaded && error.IsEmpty()) {
			delete fQwen; fQwen=qwen;
			if (backend == "cpu") {
				if (fContext != NULL) llama_free(fContext);
				if (fModel != NULL) llama_model_free(fModel);
				fContext = context; fModel = model; fStartupPath = path;
			} else { delete fTiny; fTiny = tiny; fLoads.fetch_add(1); }
			fModelPath = path; fHardware = backend; fThreads.store(threads); fReady.store(true);
		} else {
			delete qwen;
			delete tiny;
			if (context != NULL) llama_free(context);
			if (model != NULL) llama_model_free(model);
			if (error.IsEmpty()) error = "Model/context load failed. Previous model kept; choose another or retry.";
		}
		fModelError = error; fLoading = false; fPersistModel = false;
		fprintf(stderr, "Assistant service: model_loads=%d ready=%d model=%s error=%s\n",
			fLoads.load(), fReady.load(), fModelPath.String(), error.String());
	}

	bool Complete(Request* request, std::string& answer, std::string& error)
	{
		PioneerGgmlStats unexpected={};
		if (PioneerGgmlSnapshot(unexpected)) {
			error="CPU isolation check failed; no request was executed on either backend."; return false;
		}
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
	bool fLoading, fPersistModel;
	BString fPendingPath, fModelPath, fModelError;
	BString fHardware, fStartupPath;
	BString fPendingHardware;
	int32 fPendingThreads;
	AssistantP100A* fTiny;
	std::atomic<bool> fP100ASupported;
	AssistantP100AQwen* fQwen;
	std::atomic<bool> fP100AQwenSupported;
};

int main()
{
	AssistantService service;
	service.Run();
	return 0;
}
