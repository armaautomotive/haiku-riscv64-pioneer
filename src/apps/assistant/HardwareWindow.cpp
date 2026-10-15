/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "HardwareWindow.h"
#include "Protocol.h"

#include <Application.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <MessageRunner.h>
#include <Path.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <stdlib.h>

static const uint32 kHardwareSelected = 'hsel', kApplyHardware = 'happ', kPollHardware = 'hpol';

HardwareWindow::HardwareWindow(bool quitAppOnClose)
	: BWindow(BRect(180, 160, 900, 650), "Haiku Assistant — Hardware",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS), fQuitAppOnClose(quitAppOnClose),
		fThreadsLoaded(false), fServiceAvailable(false), fBusy(false)
{
	fActive = new BStringView("active_hardware", "Checking resident service...");
	fList = new BListView("hardware_backends");
	fList->SetSelectionMessage(new BMessage(kHardwareSelected));
	fList->SetExplicitMinSize(BSize(600, 65));
	for (size_t i = 0; i < kAssistantHardwareCount; i++) {
		fAvailable[i] = false;
		fDescriptions[i] = kAssistantHardware[i].description;
		fList->AddItem(new BStringItem(kAssistantHardware[i].name));
	}
	fDetails = new BTextView("hardware_details");
	fDetails->MakeEditable(false); fDetails->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	// A stable, scrollable viewport avoids the read-only BTextView's cached
	// height-for-width clipping when switching from the short CPU description.
	BScrollView* explanation = new BScrollView("hardware_explanation", fDetails, 0, false, true);
	explanation->SetExplicitMinSize(BSize(600, 150));
	fThreads = new BTextControl("hardware_cpu_threads", "CPU threads:", "", NULL);
	fThreads->TextView()->SetMaxBytes(4);
	fApply = new BButton("apply_hardware", "Use selected hardware", new BMessage(kApplyHardware));
	BButton* close = new BButton("close_hardware", "Close", new BMessage(B_QUIT_REQUESTED));
	fStatus = new BStringView("hardware_status", "Unavailable backends never fall back silently to CPU.");
	BLayoutBuilder::Group<>(this, B_VERTICAL).SetInsets(B_USE_WINDOW_INSETS)
		.Add(fActive)
		.Add(new BScrollView("hardware_list", fList, 0, false, true))
		.Add(explanation, 1)
		.Add(fThreads)
		.Add(new BStringView("hardware_hint", "Saved with the model choice. Finish any active reply before changing hardware."))
		.AddGroup(B_HORIZONTAL).Add(fApply).AddGlue().Add(close).End()
		.Add(fStatus);
	fList->Select(0); Poll(); Details();
	BMessage poll(kPollHardware); fPoll = new BMessageRunner(BMessenger(this), &poll, 1000000);
}

HardwareWindow::~HardwareWindow()
{
	delete fPoll;
	while (BListItem* item = fList->RemoveItem((int32)0)) delete item;
}

bool
HardwareWindow::QuitRequested()
{
	if (fQuitAppOnClose) be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
HardwareWindow::Details()
{
	int32 index = fList->CurrentSelection();
	bool valid = index >= 0 && (size_t)index < kAssistantHardwareCount;
	bool available = valid && fAvailable[index] && fServiceAvailable;
	if (valid) {
		BString text(kAssistantHardware[index].name);
		text << (available ? " — available\n\n" : " — unavailable\n\n") << fDescriptions[index];
		if (text != fDetails->Text()) fDetails->SetText(text.String());
	}
	fApply->SetEnabled(available && !fBusy);
	fThreads->SetEnabled(available && index == 0 && fThreadsLoaded);
}

void
HardwareWindow::Poll()
{
	BMessage query(kServiceStatus), reply;
	fServiceAvailable = BMessenger(kServiceSignature).SendMessage(&query, &reply, 100000, 100000) == B_OK;
	for (size_t i = 0; i < kAssistantHardwareCount; i++) {
		fAvailable[i] = false;
		fDescriptions[i] = kAssistantHardware[i].description;
	}
	if (fServiceAvailable) {
		BMessage hardware;
		for (int32 i = 0; reply.FindMessage("hardware", i, &hardware) == B_OK; i++) {
			const char* id = NULL;
			hardware.FindString("id", &id);
			int index = AssistantHardwareIndex(id);
			if (index < 0) continue;
			hardware.FindBool("available", &fAvailable[index]);
			const char* description;
			if (hardware.FindString("description", &description) == B_OK) fDescriptions[index] = description;
		}
		bool loading = false, busy = false, ready = false;
		reply.FindBool("loading", &loading); reply.FindBool("busy", &busy); reply.FindBool("ready", &ready);
		fBusy = loading || busy;
		int32 threads, maximum = 0;
		reply.FindInt32("max_threads", &maximum);
		if (!fThreadsLoaded && reply.FindInt32("cpu_threads", &threads) == B_OK) {
			BString value; value << threads; fThreads->SetText(value.String()); fThreadsLoaded = true;
		}
		const char* selected = ""; const char* active = "";
		reply.FindString("backend_id", &selected); reply.FindString("active_backend_id", &active);
		BString status;
		status.SetToFormat("Selected: %s   |   Running: %s   |   %ld logical CPUs available",
			*selected ? selected : "unknown (update service)",
			ready && *active ? active : (loading ? "loading model" : "no model ready"), (long)maximum);
		fActive->SetText(status.String());
	} else {
		fActive->SetText("Service unavailable. Open Haiku Assistant to start it.");
		fThreadsLoaded = false;
	}
	for (size_t i = 0; i < kAssistantHardwareCount; i++) {
		BString label(kAssistantHardware[i].name);
		label << (fServiceAvailable ? (fAvailable[i] ? "  [available]" : "  [unavailable]") : "  [service unavailable]");
		static_cast<BStringItem*>(fList->ItemAt(i))->SetText(label.String()); fList->InvalidateItem(i);
	}
	Details();
}

void
HardwareWindow::Apply()
{
	int32 index = fList->CurrentSelection();
	if (index < 0 || (size_t)index >= kAssistantHardwareCount) return;
	if (!fServiceAvailable || !fAvailable[index]) {
		fStatus->SetText("This backend is unavailable. No hardware change or CPU fallback was made."); return;
	}
	char* end;
	long threads = strtol(fThreads->Text(), &end, 10);
	if (end == fThreads->Text() || *end != '\0' || threads < 1 || threads > 4096) {
		fStatus->SetText("Enter a whole CPU-thread count from 1 to the available logical CPU count."); return;
	}
	BMessage request(kConfigureHardware), reply;
	request.AddString("backend_id", kAssistantHardware[index].id);
	request.AddInt32("cpu_threads", threads);
	status_t result = BMessenger(kServiceSignature).SendMessage(&request, &reply, 1000000, 2000000);
	if (result == B_OK && reply.FindInt32("status", &result) != B_OK) result = B_ERROR;
	const char* explanation;
	if (reply.FindString("explanation", &explanation) == B_OK) fStatus->SetText(explanation);
	else fStatus->SetText("Could not save hardware settings. Check the service and thread count; previous choice kept.");
	Poll();
}

void
HardwareWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kShowHardwareWindow: Activate(); break;
		case kHardwareSelected: Details(); break;
		case kPollHardware: Poll(); break;
		case kApplyHardware: Apply(); break;
		default: BWindow::MessageReceived(message);
	}
}
