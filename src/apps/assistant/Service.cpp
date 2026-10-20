/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
#include "Settings.h"
#include "ModelStore.h"
#include "P100ABackend.h"
#include "P100AQwen.h"
#include "P100AQwen27.h"
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
			fLoading(true), fPersistModel(false), fUnloading(false), fPendingUnload(false),
			fResidentModels(0), fProgressActive(true), fProgressValue(-1),
			fProgressPhase("Starting model service"), fTiny(NULL), fP100ASupported(false), fQwen(NULL), fP100AQwenSupported(false), fP100AAttentionSupported(false), fP100AFleetSupported(false), fQwen27(NULL), fP100AQwen27Supported(false)
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
		fModelPath = fPendingPath;
		fLoadingPath = fPendingPath; fLoadingHardware = fHardware;
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
		if (message->what == kLoadModel || message->what == kUnloadModel) {
			BAutolock lock(&fLock);
			status_t status = B_BUSY;
			if (!fLoading && !fUnloading && fRequest == NULL) {
				if (message->what == kLoadModel)
					status = fReady.load() ? B_OK : QueueSelection(fModelPath, fHardware.String(), fThreads.load());
				else {
					status = B_OK;
					if (fResidentModels != 0) {
						fUnloading = fPendingUnload = true;
						fReady.store(false);
						fProgressActive = true; fProgressValue = -1;
						fProgressPhase = "Releasing model memory and P100A session";
						release_sem(fWake);
					}
				}
			}
			BMessage reply(message->what);
			reply.AddInt32("status", status);
			reply.AddString("explanation", status == B_OK ? "Accepted; check status for completion."
				: status == B_BUSY ? "Finish/stop the current reply or wait for model loading/unloading, then try again."
				: "Could not load the selected model. Check Models, Hardware and the service status.");
			message->SendReply(&reply);
			return;
		}
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
				if (fLoading || fUnloading || fRequest != NULL) {
					status = B_BUSY; explanation = "Finish/stop the current reply or wait for model loading, then try again.";
				} else if (threadStatus == B_OK || threadStatus == B_NAME_NOT_FOUND) {
					BString path = AssistantHardwareIsP100AQwen27(id) ? AssistantModelPath(kAssistantQwen27Model)
						: AssistantHardwareIsP100AQwen(id) ? AssistantModelPath(0)
						: strcmp(id, "p100a") == 0 ? AssistantModelPath(kAssistantTinyModel)
						: (fHardware == "cpu" && !fModelPath.IsEmpty() ? fModelPath : fStartupPath);
					status = QueueSelection(path, id, threads);
					explanation = status == B_OK
						? "Verifying model/hardware pair; saved only after success. P100A modes are experimental and slow; first-use weight upload can take several minutes."
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
				if (AssistantHardwareIsP100AQwen27(fHardware.String()))
					// The isolated worker owns its pool: never change only
					// the saved/displayed count while leaving it at old threads.
					status = QueueSelection(fModelPath, fHardware.String(), threads);
				else {
					status = fLoading || fUnloading ? B_BUSY : SaveAssistantThreads(threads);
					if (status == B_OK) fThreads.store(threads);
				}
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
			reply.AddBool("unloading", fUnloading);
			reply.AddInt32("resident_models", fResidentModels);
			reply.AddBool("model_progress_active", fProgressActive);
			reply.AddFloat("model_progress", fProgressValue);
			reply.AddString("model_phase", fProgressPhase);
			reply.AddString("loading_model_path", fLoading ? fLoadingPath.String() : "");
			reply.AddString("loading_backend_id", fLoading ? fLoadingHardware.String() : "");
			reply.AddString("model_path", fModelPath);
			reply.AddString("model_error", fModelError);
			reply.AddString("backend", AssistantHardwareIsP100AQwen27(fHardware.String()) ? "P100A Work Qwen3.8-27B (experimental, slow; CPU attention/state; validation OFF)"
				: AssistantHardwareIsP100AThroughput(fHardware.String()) ? "P100A Throughput Qwen3-0.6B (up to120 cores; resident KV/attention; CPU validation OFF)"
				: AssistantHardwareIsP100AAttention(fHardware.String()) ? "P100A Attention Qwen3-0.6B (card attention; CPU validation OFF; experimental)"
				: AssistantHardwareIsP100AFast(fHardware.String()) ? "P100A Fast Qwen3-0.6B (CPU reference validation OFF; experimental)"
				: fHardware == "p100a-qwen" ? "P100A Qwen3-0.6B exact dual-reuse hybrid (experimental, slower than CPU)"
				: fHardware == "p100a" ? "P100A TinyStories experimental hybrid (not Qwen)"
				: (fHardware == "cpu" ? "CPU (llama.cpp)" : "Unavailable hardware; no fallback"));
			reply.AddString("backend_id", fHardware);
			reply.AddString("active_backend_id", fReady.load() ? fHardware.String() : "");
			for (size_t i = 0; i < kAssistantHardwareCount; i++) {
				BMessage hardware;
				hardware.AddString("id", kAssistantHardware[i].id);
				hardware.AddString("name", kAssistantHardware[i].name);
				const char* id=kAssistantHardware[i].id;
				hardware.AddBool("available", i == 0 || (i==1 && fP100ASupported.load() && AssistantModelPresent(kAssistantTinyModel))
					|| (AssistantHardwareIsP100AQwen27(id) && fP100AQwen27Supported.load() && AssistantModelPresent(kAssistantQwen27Model))
					|| (AssistantHardwareIsP100AQwen(id) && fP100AQwenSupported.load() && AssistantModelPresent(0)
						&& (!AssistantHardwareIsP100AAttention(id) || fP100AAttentionSupported.load())
						&& (!AssistantHardwareIsP100AThroughput(id) || fP100AFleetSupported.load())));
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
		if (AssistantHardwareIsP100AQwen(fHardware.String())) {
			const char* format=NULL;
			const char* pinnedModel=NULL;
			status_t tokensStatus=message->FindInt32("max_tokens",&request->tokens);
			if (backendStatus!=B_OK || message->FindString("model_path",&pinnedModel)!=B_OK || fModelPath!=pinnedModel
				|| message->FindString("prompt_format",&format)!=B_OK
				|| strcmp(format,AssistantHardwareQwenFormat(fHardware.String()))
				|| (tokensStatus!=B_OK && tokensStatus!=B_NAME_NOT_FOUND) || request->tokens<1 || request->tokens>16
				|| request->prompt.empty() || request->prompt.size()>512 || !fP100AQwenSupported.load()) {
				Reply(request,kDone,"","P100A Qwen requires explicit short-question mode, 512 bytes and 1-16 output tokens. No CPU fallback.");
				delete request; return;
			}
		}
		if (AssistantHardwareIsP100AQwen27(fHardware.String())) {
			const char* format=NULL; const char* pinnedModel=NULL;
			status_t tokensStatus=message->FindInt32("max_tokens",&request->tokens);
			if (backendStatus!=B_OK || message->FindString("model_path",&pinnedModel)!=B_OK || fModelPath!=pinnedModel
				|| message->FindString("prompt_format",&format)!=B_OK || strcmp(format,AssistantHardwareQwenFormat(fHardware.String()))
				|| tokensStatus!=B_OK || request->tokens<1 || request->tokens>32 || request->prompt.empty() || request->prompt.size()>256
				|| !fP100AQwen27Supported.load()) {
				Reply(request,kDone,"","P100A Qwen27B requires explicit Work mode,256-byte short question and1-32 output tokens. No CPU fallback.");
				delete request; return;
			}
		}
		if (!fReady.load() || fLoading || fUnloading || fRequest != NULL || request->prompt.size() > 12000) {
			Reply(request, kDone, "", (fLoading || fUnloading || fRequest != NULL)
				? "Service is loading, unloading or busy. Try again shortly."
				: !fReady.load() ? "No model is loaded. Press Load model first." : "Prompt is too long.");
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
		if (fLoading || fUnloading || fRequest != NULL) return B_BUSY;
		int index = AssistantModelIndex(path.String());
		if (index < 0 || !kAssistantModels[index].compatible
			|| !AssistantModelSupportsBackend(index,backend)
			|| threads < 1 || threads > AssistantMaxThreads()) return B_BAD_VALUE;
		if (strcmp(backend, "p100a") == 0 && !fP100ASupported.load()) return B_NOT_SUPPORTED;
		if (AssistantHardwareIsP100AQwen(backend) && !fP100AQwenSupported.load()) return B_NOT_SUPPORTED;
		if (AssistantHardwareIsP100AAttention(backend) && !fP100AAttentionSupported.load()) return B_NOT_SUPPORTED;
		if (AssistantHardwareIsP100AThroughput(backend) && !fP100AFleetSupported.load()) return B_NOT_SUPPORTED;
		if (AssistantHardwareIsP100AQwen27(backend) && !fP100AQwen27Supported.load()) return B_NOT_SUPPORTED;
		if (!AssistantModelPresent(index)) return B_ENTRY_NOT_FOUND;
		if (fReady.load() && fHardware == backend && fModelPath == path
			&& (!AssistantHardwareIsP100AQwen27(backend) || threads==fThreads.load())) {
			status_t status = SaveAssistantHardware(backend, threads);
			if (status == B_OK) fThreads.store(threads);
			return status;
		}
		fPendingPath = path; fPendingHardware = backend; fPendingThreads = threads;
		fLoadingPath = path; fLoadingHardware = backend;
		fLoading = true; fPersistModel = true; fModelError = "";
		fProgressActive = true; fProgressValue = 0; fProgressPhase = "Verifying model SHA-256";
		release_sem(fWake); return B_OK;
	}

	static int32 Worker(void* data)
	{
		static_cast<AssistantService*>(data)->RunWorker();
		return 0;
	}

	void Progress(float value, const char* phase)
	{
		BAutolock lock(&fLock);
		fProgressActive = true;
		fProgressValue = value < 0 ? -1 : std::min(1.0f, std::max(0.0f, value));
		fProgressPhase = phase;
	}

	static bool LoadProgress(float value, void* data)
	{
		AssistantService* self = static_cast<AssistantService*>(data);
		self->Progress(value, "Loading model weights into host memory");
		return !self->fQuit.load();
	}

	// Worker-only: no context or device ownership is released by the UI thread.
	bool ReleaseModels()
	{
		std::string error;
		bool clean = !fQwen || fQwen->Unload(error);
		bool clean27 = !fQwen27 || fQwen27->Unload(error); clean=clean && clean27;
		delete fQwen27; fQwen27=NULL;
		delete fQwen; fQwen = NULL;
		delete fTiny; fTiny = NULL;
		if (fContext) llama_free(fContext);
		if (fModel) llama_model_free(fModel);
		fContext = NULL; fModel = NULL;
		BAutolock lock(&fLock);
		fReady.store(false); fActiveThreads.store(0); fResidentModels = 0;
		fUnloading = false; fProgressActive = false;
		fModelError = error.c_str();
		if (!clean) { fP100ASupported.store(false); fP100AQwenSupported.store(false); fP100AQwen27Supported.store(false); }
		fprintf(stderr, "Assistant service: unloaded all resident models; clean=%d\n", clean);
		return clean;
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
		fP100AAttentionSupported.store(AssistantP100AQwen::Available(true));
		fP100AFleetSupported.store(AssistantP100AQwen::Available(true,true));
		fP100AQwen27Supported.store(AssistantP100AQwen27::Available());
		while (!fQuit.load()) {
			if (acquire_sem(fWake) != B_OK || fQuit.load()) break;
			BString path, backend;
			bool unload;
			int32 threads;
			Request* request;
			{
				BAutolock lock(&fLock);
				path = fPendingPath;
				backend = fPendingHardware; threads = fPendingThreads;
				fPendingPath = "";
				unload = fPendingUnload; fPendingUnload = false;
				request = fRequest;
			}
			if (unload) { ReleaseModels(); continue; }
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
						fP100AQwen27Supported.store(false);
					}
				} else if (AssistantHardwareIsP100AQwen(fHardware.String())) {
					fActiveThreads.store(1);
					success=fQwen && fQwen->Complete(request->prompt,request->tokens,answer,error,
						[&]() { return fQuit.load() || request->cancelled.load(); },
						[&](const std::string& text,const char* status) {
							PioneerGgmlStats stats = {};
							if (PioneerGgmlSnapshot(stats) && stats.tensors < 197)
								Progress(float(double(stats.uploads) / 163692), "Uploading P100A weights (measured first-reply progress)");
							else { BAutolock lock(&fLock); fProgressActive = false; }
							if (!strncmp(status,"P100A Qwen:",11)) request->summary=status;
							Reply(request,kUpdate,text,status);
						});
					if (fQwen && fQwen->Faulted()) {
						BAutolock lock(&fLock); fP100AQwenSupported.store(false); fReady.store(false); fModelError=error.c_str();
						fP100ASupported.store(false);
						fP100AQwen27Supported.store(false);
					}
				} else if (AssistantHardwareIsP100AQwen27(fHardware.String())) {
					fActiveThreads.store(fThreads.load());
					success=fQwen27 && fQwen27->Complete(request->prompt,request->tokens,answer,error,
						[&]() { return fQuit.load() || request->cancelled.load(); },
						[&](const std::string& text,const char* status) { request->summary=status; Reply(request,kUpdate,text,status); },
						[&](float value,const char* phase) { Progress(value,phase); });
					if (fQwen27 && fQwen27->Faulted()) {
						BAutolock lock(&fLock); fP100AQwen27Supported.store(false); fP100AQwenSupported.store(false);
						fP100ASupported.store(false); fReady.store(false); fModelError=error.c_str();
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
				fProgressActive = false;
			}
			Reply(request, kDone, answer, success
				? (fHardware != "cpu" ? request->summary.c_str()
					: "Ready. Model stays loaded in the background.") : error.c_str(), success);
			delete request;
		}
		ReleaseModels();
		llama_backend_free();
	}

	void LoadModel(const BString& path, const BString& backend, int32 threads)
	{
		{
			BAutolock lock(&fLock);
			if (!AssistantHardwareCanChat(backend.String())) {
				fLoading = false;
				fProgressActive = false;
				fModelError = "Saved hardware backend is unavailable. Open Hardware and explicitly choose CPU.";
				return;
			}
		}
		BString error;
		int index = AssistantModelIndex(path.String());
		const bool reuseQwen=fQwen && fReady.load() && path==fModelPath
			&& AssistantHardwareIsP100AQwen(fHardware.String()) && AssistantHardwareIsP100AQwen(backend.String());
		const bool previousValidation=!AssistantHardwareSkipsValidation(fHardware.String());
		const bool previousAttention=AssistantHardwareIsP100AAttention(fHardware.String());
		const bool previousThroughput=AssistantHardwareIsP100AThroughput(fHardware.String());
		const bool reuseQwen27=fQwen27 && fReady.load() && path==fModelPath && backend==fHardware;
		const int32 previousThreads=fThreads.load();
		status_t status = B_NOT_SUPPORTED;
		if (index >= 0 && kAssistantModels[index].compatible
			&& AssistantModelSupportsBackend(index,backend.String()))
			// The isolated27B worker authenticates the FULL pinned file on
			// the SAME open description passed to llama, before any BEGIN.
			// A duplicate parent pathname hash adds minutes but cannot bind
			// that file description. Never bypass the worker's authentication.
			status = reuseQwen || AssistantHardwareIsP100AQwen27(backend.String()) ? B_OK : VerifyAssistantModel(index, path.String(), fQuit, error, BMessenger(),
				[&](float value) { Progress(value, "Verifying model SHA-256"); });
		else error = "Model is outside the supported catalog. Open Models to choose a supported Qwen3 model.";
		llama_model* model = NULL;
		llama_context* context = NULL;
		AssistantP100A* tiny = NULL;
		AssistantP100AQwen* qwen=NULL;
		AssistantP100AQwen27* qwen27=NULL;
		if (status==B_OK && AssistantHardwareIsP100AQwen27(backend.String())) {
			if (!fP100AQwen27Supported.load()) error="Qwen27B worker unavailable or faulted; no CPU fallback.";
			else if (reuseQwen27) {
				Progress(-1,"Changing CPU pool; retaining Qwen27B context and warm P100A weights");
				std::string reason;
				if (fQwen27->SetThreads(threads,reason)) qwen27=fQwen27;
				else {
					error=reason.c_str();
					if (fQwen27->Faulted()) {
						fReady.store(false); fP100AQwen27Supported.store(false); fP100AQwenSupported.store(false); fP100ASupported.store(false);
					}
				}
			}
			else {
				qwen27=new AssistantP100AQwen27; std::string reason;
				if (!qwen27->Load(path.String(),threads,reason,[&](float value,const char* phase) { Progress(value,phase); })) {
					error=reason.c_str();
					if (qwen27->Faulted()) { fP100AQwen27Supported.store(false); fP100AQwenSupported.store(false); fP100ASupported.store(false); }
					delete qwen27; qwen27=NULL;
				}
			}
		}
		if (status==B_OK && AssistantHardwareIsP100AQwen(backend.String())) {
			if (!fP100AQwenSupported.load() || (AssistantHardwareIsP100AAttention(backend.String()) && !fP100AAttentionSupported.load())
				|| (AssistantHardwareIsP100AThroughput(backend.String()) && !fP100AFleetSupported.load())) error="P100A exact Qwen/attention/multicore driver unavailable or disabled after a fault; no fallback.";
			else {
				std::string reason;
				bool validate=!AssistantHardwareSkipsValidation(backend.String());
				bool attention=AssistantHardwareIsP100AAttention(backend.String());
				bool throughput=AssistantHardwareIsP100AThroughput(backend.String());
				if (reuseQwen) {
					Progress(-1, attention!=previousAttention || throughput!=previousThroughput?"Rebuilding execution graph; device weights reload on first reply":"Preparing Qwen mode; retaining resident P100A weights");
					if (fQwen->SetMode(validate,attention,reason,throughput)) qwen=fQwen;
					else error=reason.c_str();
				} else {
					qwen=new AssistantP100AQwen;
					if (!qwen->Load(path.String(),reason,validate,[&](float value,const char* phase) {
						Progress(value,phase); return !fQuit.load();
					},attention,throughput)) { error=reason.c_str(); delete qwen; qwen=NULL; }
				}
			}
		}
		if (status == B_OK && backend == "p100a") {
			if (!fP100ASupported.load()) error = "P100A driver unavailable or disabled after a fault; no fallback.";
			else {
				Progress(-1, "Preparing TinyStories model and tokenizer");
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
			Progress(-1, "Preparing CPU context and KV cache");
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
		// A27B worker loads host-only, so the previous card session can stay
		// intact until its replacement is ready. NEVER permit two live card
		// sessions:explicit clean unload BEFORE committing the new pair.
		bool previousReleased=false;
		bool loaded = context != NULL || tiny != NULL || qwen != NULL || qwen27 != NULL;
		if (loaded && error.IsEmpty() && ((qwen27 && fQwen) || (fQwen27 && !reuseQwen27))) {
			std::string reason; bool clean=true;
			if (fQwen27) { clean=fQwen27->Unload(reason); delete fQwen27; fQwen27=NULL; previousReleased=true; }
			if (qwen27 && fQwen) { bool oldClean=fQwen->Unload(reason); clean=clean && oldClean; delete fQwen; fQwen=NULL; previousReleased=true; }
			if (!clean) {
				error="Previous P100A session could not close safely; all card modes disabled. "; error<<reason.c_str();
				fP100ASupported.store(false); fP100AQwenSupported.store(false); fP100AQwen27Supported.store(false);
			}
		}
		BAutolock lock(&fLock);
		if (loaded && error.IsEmpty() && fPersistModel) {
			BMessage settings; LoadAssistantSettings(settings);
			status_t saved = UpdateAssistantHardwareSettings(settings, backend.String(), threads);
			settings.RemoveName("model_path"); settings.AddString("model_path", path);
			BString lastCPU = backend == "cpu" ? path : fStartupPath;
			settings.RemoveName("last_cpu_model_path"); settings.AddString("last_cpu_model_path", lastCPU);
			if (saved != B_OK || SaveAssistantSettings(settings) != B_OK)
				error = "Could not save model/hardware pair; previous selection kept.";
		}
		if (loaded && error.IsEmpty()) {
			fQwen27=qwen27;
			if (!reuseQwen) { delete fQwen; fQwen=qwen; }
			if (backend == "cpu") {
				if (fContext != NULL) llama_free(fContext);
				if (fModel != NULL) llama_model_free(fModel);
				fContext = context; fModel = model; fStartupPath = path;
			} else if (!reuseQwen && !reuseQwen27) { delete fTiny; fTiny = tiny; fLoads.fetch_add(1); }
			fModelPath = path; fHardware = backend; fThreads.store(threads); fReady.store(true);
		} else {
			if (!reuseQwen27) delete qwen27;
			else if (qwen27) {
				std::string restoreError;
				if (!fQwen27->SetThreads(previousThreads,restoreError)) {
					fReady.store(false); fP100AQwen27Supported.store(false); fP100AQwenSupported.store(false); fP100ASupported.store(false);
					error="Qwen27B thread restoration failed; backend disabled. "; error<<restoreError.c_str();
				}
			}
			if (previousReleased) fReady.store(false); // previous selection saved,but resident card context was released
			if (reuseQwen) {
				std::string restoreError;
				if (!fQwen->SetMode(previousValidation,previousAttention,restoreError,previousThroughput) || fQwen->Faulted()) {
					fReady.store(false); fP100AQwenSupported.store(false); fP100AAttentionSupported.store(false);
					error="P100A mode restoration failed; model unavailable, no CPU fallback. "; error<<restoreError.c_str();
				}
			}
			else delete qwen;
			delete tiny;
			if (context != NULL) llama_free(context);
			if (model != NULL) llama_model_free(model);
			if (error.IsEmpty()) error = "Model/context load failed. Previous model kept; choose another or retry.";
		}
		fModelError = error; fLoading = false; fPersistModel = false;
		fResidentModels = (fModel != NULL) + (fTiny != NULL) + (fQwen != NULL) + (fQwen27 != NULL);
		fProgressActive = false;
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
	bool fUnloading, fPendingUnload;
	int32 fResidentModels;
	bool fProgressActive;
	float fProgressValue;
	BString fProgressPhase;
	BString fLoadingPath, fLoadingHardware;
	BString fPendingPath, fModelPath, fModelError;
	BString fHardware, fStartupPath;
	BString fPendingHardware;
	int32 fPendingThreads;
	AssistantP100A* fTiny;
	std::atomic<bool> fP100ASupported;
	AssistantP100AQwen* fQwen;
	std::atomic<bool> fP100AQwenSupported;
	std::atomic<bool> fP100AAttentionSupported;
	std::atomic<bool> fP100AFleetSupported;
	AssistantP100AQwen27* fQwen27;
	std::atomic<bool> fP100AQwen27Supported;
};

int main()
{
	AssistantService service;
	service.Run();
	return 0;
}
