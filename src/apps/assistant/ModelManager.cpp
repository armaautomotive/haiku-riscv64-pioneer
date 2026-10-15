/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "ModelManager.h"
#include "ModelStore.h"
#include "Protocol.h"

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

static const uint32 kChoose = 'mcho', kInstall = 'midl', kUse = 'muse';
static const uint32 kCancelDownload = 'mccl', kPoll = 'mpol';

ModelManager::ModelManager()
	: BWindow(BRect(150, 140, 900, 670), "Haiku Assistant — Models",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS), fWorker(-1), fDownloadIndex(-1),
		fDownloading(false), fSelecting(false), fCancelled(false)
{
	fList = new BListView("models");
	fList->SetSelectionMessage(new BMessage(kChoose));
	fList->SetExplicitMinSize(BSize(600, 105));
	for (size_t i = 0; i < kAssistantModelCount; i++) fList->AddItem(new BStringItem(""));
	fDetails = new BTextView("model details");
	fDetails->MakeEditable(false);
	fDetails->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fDetails->SetExplicitMinSize(BSize(600, 170));
	fStatus = new BStringView("model status", "Select a model. Downloads never start automatically.");
	fProgress = new BStatusBar("model progress"); fProgress->SetMaxValue(100);
	fInstall = new BButton("install_model", "Install from Hugging Face", new BMessage(kInstall));
	fSelect = new BButton("use_model", "Use for new chats", new BMessage(kUse));
	fCancel = new BButton("cancel_model_download", "Cancel download", new BMessage(kCancelDownload));
	fCancel->SetEnabled(false);
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(new BStringView("backend", "Chat backend: CPU. P100A full-model chat support is still in development."))
		.Add(new BScrollView("model list", fList, 0, false, true))
		.Add(fDetails, 1)
		.Add(fProgress)
		.AddGroup(B_HORIZONTAL).Add(fInstall).Add(fSelect).AddGlue().Add(fCancel).End()
		.Add(fStatus);
	Refresh(); fList->Select(0); Details();
	BMessage poll(kPoll); fPoll = new BMessageRunner(BMessenger(this), &poll, 1000000);
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
	text.SetToFormat("%s\nDownload: %.2f GiB   |   License: Apache-2.0\nSource: https://huggingface.co/%s\n",
		model.name, (double)model.bytes / (1024.0 * 1024 * 1024), model.repository);
	text << "File: " << AssistantModelPath(index) << "\n";
	text << "Installation needs twice the download size plus 64 MiB free.\n";
	if (model.compatible)
		text << "Qwen3 / CPU backend. Larger models use more RAM and are slower;\n"
			"0.6B has been tested for chat. Other sizes need on-device inference testing.\n"
			"SHA-256 verification is required before installation and activation.";
	else
		text << "Not available for installation/selection yet: the installed llama runtime\n"
			"does not support this architecture, and the P100A chat backend is unfinished.\n"
			"P100A TinyStories experiments are not a general chat backend.";
	fDetails->SetText(text.String());
	bool present = AssistantModelPresent(index);
	fInstall->SetEnabled(!fDownloading && model.compatible && !present);
	fSelect->SetEnabled(!fDownloading && model.compatible && present);
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
	if (index < 0 || fDownloading) return;
	BMessage request(kSelectModel), reply;
	request.AddInt32("model_index", index);
	status_t status = BMessenger(kServiceSignature).SendMessage(&request, &reply, 1000000, 1000000);
	if (status == B_OK && reply.FindInt32("status", &status) != B_OK) status = B_ERROR;
	if (status == B_OK) {
		fSelecting = true;
		fStatus->SetText("Verifying and loading selected model. Selection is saved only on success.");
	} else if (status == B_BUSY)
		fStatus->SetText("Service is busy or loading. Finish/stop the reply, then try again.");
	else fStatus->SetText("Could not select model. Open Assistant to start the updated service; check the model file.");
}

void
ModelManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kShowModelManager: Activate(); break;
		case kChoose: Details(); break;
		case kInstall: Install(); break;
		case kUse: Select(); break;
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
			if (fDownloading) break;
			BMessage query(kServiceStatus), reply;
			if (BMessenger(kServiceSignature).SendMessage(&query, &reply, 100000, 100000) != B_OK) {
				SetTitle("Haiku Assistant — Models (service unavailable)");
				if (fSelecting) fStatus->SetText("Model service stopped. Reopen Assistant to restart it.");
				fSelecting = false;
				break;
			}
			Refresh(); Details();
			bool loading = false; const char* path = ""; const char* error = "";
			reply.FindBool("loading", &loading); reply.FindString("model_path", &path);
			reply.FindString("model_error", &error);
			BString title("Haiku Assistant — Models");
			if (loading) { title << " (verifying/loading)"; fSelecting = true; fStatus->SetText("Verifying SHA-256 and loading model; chat is temporarily unavailable."); }
			else if (*path != '\0') {
				title << " — active: " << BPath(path).Leaf();
				if (fSelecting) {
					BString status("Active on CPU: "); status << BPath(path).Leaf();
					fStatus->SetText(*error ? error : status.String());
				}
			}
			if (!loading) { if (fSelecting && *error) fStatus->SetText(error); fSelecting = false; }
			SetTitle(title.String());
			break;
		}
		default: BWindow::MessageReceived(message);
	}
}
