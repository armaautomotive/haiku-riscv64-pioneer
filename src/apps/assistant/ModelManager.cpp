/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "ModelManager.h"
#include "ModelStore.h"
#include "Protocol.h"
#include "ServiceLauncher.h"
#include "Hardware.h"

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MessageRunner.h>
#include <Path.h>
#include <ScrollView.h>
#include <StatusBar.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextView.h>
#include <stdio.h>

static const uint32 kChoose = 'mcho', kInstall = 'midl', kUse = 'muse';
static const uint32 kCancelDownload = 'mccl', kPoll = 'mpol';

ModelManager::ModelManager()
	: BWindow(BRect(150, 100, 950, 780), "Haiku Assistant — Models",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS), fWorker(-1), fDownloadIndex(-1),
		fDownloading(false), fSelecting(false), fServiceBusy(true), fCancelled(false)
{
	fList = new BListView("models");
	fList->SetSelectionMessage(new BMessage(kChoose));
	fList->SetExplicitMinSize(BSize(600, 105));
	for (size_t i = 0; i < kAssistantModelCount; i++) fList->AddItem(new BStringItem(""));
	fDetails = new BTextView("model details");
	fDetails->MakeEditable(false);
	fDetails->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	BScrollView* details = new BScrollView("model explanation", fDetails, 0, false, true);
	details->SetExplicitMinSize(BSize(600, 170));
	fStatus = new BStringView("model status", "Select a model. Downloads never start automatically.");
	fProgress = new BStatusBar("model progress"); fProgress->SetMaxValue(100);
	fInstall = new BButton("install_model", "Install from Hugging Face", new BMessage(kInstall));
	fSelect = new BButton("use_model", "Use for new chats", new BMessage(kUse));
	fCancel = new BButton("cancel_model_download", "Cancel download", new BMessage(kCancelDownload));
	fCancel->SetEnabled(false);
	fMemoryModel = new BStringView("memory_model", "Chat model: checking service...");
	fMemoryStatus = new BStringView("memory_status", "Checking resident model memory...");
	fLoadProgress = new BStatusBar("model_loading_progress"); fLoadProgress->SetMaxValue(100);
	fLoadProgress->Hide();
	fLoad = new BButton("load_model", "Load model", new BMessage(kLoadModel));
	fUnload = new BButton("unload_model", "Unload model", new BMessage(kUnloadModel));
	fLoad->SetToolTip("Reload the chat model named above. Use the catalog's Use button to switch models.");
	fUnload->SetToolTip("Release all model/context caches and P100A weights; keep files, selection and chat.");
	fLoad->SetEnabled(false); fUnload->SetEnabled(false);
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(new BStringView("backend", "Qwen3: CPU; Qwen3 0.6B also has a slow experimental P100A option in Hardware…"))
		.Add(new BScrollView("model list", fList, 0, false, true))
		.Add(details, 1)
		.Add(fProgress)
		.AddGroup(B_HORIZONTAL).Add(fInstall).Add(fSelect).AddGlue().Add(fCancel).End()
		.Add(fStatus)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fMemoryModel)
		.AddGroup(B_HORIZONTAL).Add(fLoad).Add(fUnload).AddGlue().End()
		.Add(fLoadProgress)
		.Add(fMemoryStatus);
	Refresh(); fList->Select(0); Details();
	BMessage poll(kPoll); fPoll = new BMessageRunner(BMessenger(this), &poll, 1000000);
	PostMessage(kPoll);
}

ModelManager::~ModelManager()
{
	delete fPoll;
	fCancelled.store(true);
	if (fWorker >= 0) { status_t result; wait_for_thread(fWorker, &result); }
	while (BListItem* item = fList->RemoveItem((int32)0)) delete item;
}

bool
ModelManager::QuitRequested()
{
	if (!fDownloading) return true;
	fCancelled.store(true);
	fStatus->SetText("Cancelling download; close again when it finishes.");
	return false;
}

void
ModelManager::Refresh()
{
	for (size_t i = 0; i < kAssistantModelCount; i++) {
		BString label(kAssistantModels[i].name);
		label << (AssistantModelPresent(i) ? "  [installed; verified before use]" : "  [not installed]");
		static_cast<BStringItem*>(fList->ItemAt(i))->SetText(label.String());
		fList->InvalidateItem(i);
	}
}

void
ModelManager::Details()
{
	int32 index = fList->CurrentSelection();
	if (index < 0) return;
	const AssistantModel& model = kAssistantModels[index];
	BString text;
	text.SetToFormat("%s\nDownload: %.2f MiB   |   License: %s\nSource: https://huggingface.co/%s\n",
		model.name, (double)model.bytes / (1024.0 * 1024),
		(size_t)index == kAssistantTinyModel ? "MIT" : "Apache-2.0", model.repository);
	text << "File: " << AssistantModelPath(index) << "\n";
	text << "Installation needs twice the download size plus 64 MiB free.\n";
	if ((size_t)index == kAssistantTinyModel)
		text << "Includes the pinned 6 KiB tokenizer; both files require SHA-256 verification.\n"
			"Use selects P100A AND TinyStories together. This tiny base model continues\n"
			"short ASCII stories; it is NOT a chat assistant or a Qwen accelerator.\n"
			"All projections run on-card; attention/KV/norms and verification use CPU.\n"
			"8 output tokens; roughly 15 seconds per prompt/output token. Stop is batch-safe.";
	else if (model.compatible)
		text << "Qwen3 / CPU backend. Larger models use more RAM and are slower;\n"
			"0.6B has been tested for chat. Other sizes need on-device inference testing.\n"
			"SHA-256 verification is required before installation and activation.";
	else
		text << "Not available for installation/selection yet: the installed llama runtime\n"
			"does not support this architecture, and the P100A chat backend is unfinished.\n"
			"P100A TinyStories experiments are not a general chat backend.";
	if (index==0) text << "\nTo run this exact model on P100A, open Hardware and select P100A — Qwen3 0.6B.\n"
		"Validated and Fast (no CPU reference) targets are available; neither guarantees a CPU speedup.\n"
		"First request uploads weights for several minutes; warm Qwen mode changes reuse them.";
	fDetails->SetText(text.String());
	fSelect->SetLabel((size_t)index == kAssistantTinyModel ? "Use on P100A (story mode)" : "Use on CPU (chat)");
	bool present = AssistantModelPresent(index);
	fInstall->SetEnabled(!fDownloading && model.compatible && !present);
	fSelect->SetEnabled(!fDownloading && !fServiceBusy && model.compatible && present);
}

void
ModelManager::Install()
{
	if (fDownloading) return;
	fDownloadIndex = fList->CurrentSelection();
	if (fDownloadIndex < 0 || !kAssistantModels[fDownloadIndex].compatible) return;
	fCancelled.store(false); fDownloading = true;
	fInstall->SetEnabled(false); fSelect->SetEnabled(false); fCancel->SetEnabled(true);
	fProgress->Reset(); fProgress->SetMaxValue(100);
	fStatus->SetText("Connecting to Hugging Face over verified HTTPS...");
	fWorker = spawn_thread(Download, "assistant model download", B_NORMAL_PRIORITY, this);
	status_t status = fWorker < 0 ? fWorker : resume_thread(fWorker);
	if (status != B_OK) {
		if (fWorker >= 0) kill_thread(fWorker);
		fWorker = -1; fDownloading = false; fCancel->SetEnabled(false);
		fStatus->SetText("Could not start download worker."); Details();
	}
}

int32
ModelManager::Download(void* data)
{
	ModelManager* self = static_cast<ModelManager*>(data);
	BString error;
	status_t result = InstallAssistantModel(self->fDownloadIndex, self->fCancelled,
		BMessenger(self), error);
	BMessage done(kModelInstalled); done.AddInt32("status", result); done.AddString("error", error);
	BMessenger(self).SendMessage(&done, (BHandler*)NULL, 1000000);
	return result;
}

void
ModelManager::Select()
{
	int32 index = fList->CurrentSelection();
	if (index < 0 || fDownloading || fServiceBusy) return;
	BMessage request(kSelectModel), reply;
	request.AddInt32("model_index", index);
	status_t status = BMessenger(kServiceSignature).SendMessage(&request, &reply, 1000000, 1000000);
	if (status == B_OK && reply.FindInt32("status", &status) != B_OK) status = B_ERROR;
	if (status == B_OK) {
		fSelecting = true; fServiceBusy = true; Details();
		fStatus->SetText("Verifying and loading selected model. Selection is saved only on success.");
	} else if (status == B_BUSY)
		fStatus->SetText("Service is busy or loading. Finish/stop the reply, then try again.");
	else fStatus->SetText("Could not select model. Open Assistant to start the updated service; check the model file.");
}

void
ModelManager::ModelMemory(uint32 action)
{
	if (fDownloading) return;
	if (action == kLoadModel && !BMessenger(kServiceSignature).IsValid()) {
		fMemoryStatus->SetText(StartAssistantService() == B_OK
			? "Starting model service..." : "Could not start the model service.");
		PostMessage(kPoll); return; // A new service queues the saved model itself.
	}
	BMessage request(action), reply;
	status_t result = B_ERROR;
	if (BMessenger(kServiceSignature).SendMessage(&request, &reply, 100000, 1000000) != B_OK
		|| reply.FindInt32("status", &result) != B_OK || result != B_OK) {
		const char* explanation = "Could not change model memory. Stop the reply or wait for loading; an updated service is required.";
		reply.FindString("explanation", &explanation);
		(new BAlert("Model memory", explanation, "OK"))->Go(NULL);
	} else {
		fLoad->SetEnabled(false); fUnload->SetEnabled(false);
		fMemoryStatus->SetText(action == kLoadModel ? "Loading selected chat model..." : "Releasing resident model memory...");
	}
	PostMessage(kPoll);
}

void
ModelManager::PollService()
{
	BMessage query(kServiceStatus), reply;
	if (BMessenger(kServiceSignature).SendMessage(&query, &reply, 100000, 100000) != B_OK) {
		fServiceBusy = true;
		fLoad->SetEnabled(!fDownloading); fUnload->SetEnabled(false);
		if (!fLoadProgress->IsHidden()) fLoadProgress->Hide();
		fMemoryStatus->SetText("Service stopped. Load model restarts it with your saved selection.");
		SetTitle("Haiku Assistant — Models");
		Details(); return;
	}
	bool ready = false, loading = false, unloading = false, busy = false, progress = false;
	float value = -1; int32 resident = 0;
	const char* path = ""; const char* error = ""; const char* backend = ""; const char* phase = "";
	reply.FindBool("ready", &ready); reply.FindBool("loading", &loading);
	reply.FindBool("unloading", &unloading); reply.FindBool("busy", &busy);
	bool lifecycle = reply.FindInt32("resident_models", &resident) == B_OK;
	reply.FindBool("model_progress_active", &progress); reply.FindFloat("model_progress", &value);
	reply.FindString("model_phase", &phase); reply.FindString("model_path", &path);
	reply.FindString("model_error", &error); reply.FindString("backend_id", &backend);
	if (loading) {
		const char* pendingPath = ""; const char* pendingBackend = "";
		if (reply.FindString("loading_model_path", &pendingPath) == B_OK && *pendingPath) path = pendingPath;
		if (reply.FindString("loading_backend_id", &pendingBackend) == B_OK && *pendingBackend) backend = pendingBackend;
	}
	fServiceBusy = loading || unloading || busy;
	fLoad->SetEnabled(lifecycle && !fDownloading && !fServiceBusy && !ready);
	fUnload->SetEnabled(lifecycle && !fDownloading && !fServiceBusy && resident > 0);
	if (progress) {
		char percent[24] = "";
		if (value >= 0) snprintf(percent, sizeof(percent), "%.0f%%", value * 100);
		fLoadProgress->SetTo(value < 0 ? 0 : value * 100, phase, percent);
		if (fLoadProgress->IsHidden()) fLoadProgress->Show();
	} else if (!fLoadProgress->IsHidden()) fLoadProgress->Hide();
	BString model("Chat model: "); model << (*path ? BPath(path).Leaf() : "none") << " | " << backend;
	model << (unloading ? " (unloading)" : loading ? " (loading)" : ready ? " (loaded)" : " (unloaded)");
	fMemoryModel->SetText(model.String());
	BString status;
	if (loading || unloading) status = phase;
	else if (*error) status = error;
	else if (busy) status = "Model in use. Stop the reply in Assistant before unloading.";
	else if (ready) {
		status << "Resident models: " << resident << ". Files and chat are kept on unload.";
		if (AssistantHardwareIsP100AQwen(backend)) status << " P100A upload begins on first reply.";
	} else status = "No resident model. Load restores this chat model; Use above switches models.";
	fMemoryStatus->SetText(status.String());
	if (fSelecting && !loading) {
		if (!fDownloading) fStatus->SetText(*error ? error : ready ? "Selected model loaded successfully." : "No model is ready.");
		fSelecting = false;
	}
	if (!fDownloading) Refresh();
	Details();
	SetTitle("Haiku Assistant — Models");
}

void
ModelManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kShowModelManager: Activate(); break;
		case kChoose: Details(); break;
		case kInstall: Install(); break;
		case kUse: Select(); break;
		case kLoadModel:
		case kUnloadModel: ModelMemory(message->what); break;
		case kCancelDownload: fCancelled.store(true); fStatus->SetText("Cancelling..."); break;
		case kModelProgress:
		{
			int64 bytes = 0, total = 0; const char* phase = "Working";
			message->FindInt64("bytes", &bytes); message->FindInt64("total", &total);
			message->FindString("phase", &phase);
			float value = total > 0 ? 100.0 * bytes / total : 0;
			fProgress->Update(value - fProgress->CurrentValue(), phase);
			BString text; text.SetToFormat("%s: %.1f / %.1f MiB", phase, bytes / 1048576.0, total / 1048576.0);
			fStatus->SetText(text.String()); break;
		}
		case kModelInstalled:
		{
			status_t result = B_ERROR; const char* error = "Download failed.";
			message->FindInt32("status", &result); message->FindString("error", &error);
			status_t workerResult; wait_for_thread(fWorker, &workerResult); fWorker = -1;
			fDownloading = false; fCancel->SetEnabled(false);
			Refresh(); Details();
			if (result == B_OK) { fProgress->Update(100 - fProgress->CurrentValue()); fStatus->SetText("Installed and verified. Choose Use for new chats to activate it."); }
			else fStatus->SetText(error);
			break;
		}
		case kPoll:
		{
			PollService();
			break;
		}
		default: BWindow::MessageReceived(message);
	}
}
