/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
#include "ToolBridge.h"
#include "ToolSession.h"
#include "ModelManager.h"
#include "HardwareWindow.h"
#include <Application.h>
#include <Button.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Roster.h>
#include <Path.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <TextControl.h>
#include <Alert.h>
#include <stdio.h>
#include <Window.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>

static const uint32 kSend = 'send', kStop = 'stop', kClear = 'cler';
static const uint32 kShow = 'show', kCheck = 'chek';
static const uint32 kApplyThreads = 'apth';
static const uint32 kOpenModels = 'omod';
static const uint32 kOpenHardware = 'ohdw';

static status_t
StartService()
{
	if (BMessenger(kServiceSignature).IsValid())
		return B_OK;
	const char* runtime = "/boot/home/develop/llama-c060ca974c77/build-pioneer/bin";
	if (access(runtime, R_OK) != 0)
		runtime = "/Haiku1/home/develop/llama-c060ca974c77/build-pioneer/bin";
	std::string libraries = std::string(runtime) + ":/boot/system/lib";
	setenv("LIBRARY_PATH", libraries.c_str(), 1);
	entry_ref ref;
	status_t result = get_ref_for_path(
		"/boot/home/config/non-packaged/servers/HaikuAssistantService", &ref);
	if (result == B_OK)
		result = be_roster->Launch(&ref);
	return result == B_ALREADY_RUNNING ? B_OK : result;
}

struct Turn { std::string user; std::string assistant; };

class AssistantWindow : public BWindow {
public:
	AssistantWindow()
		: BWindow(BRect(100, 100, 820, 680),
#ifdef ASSISTANT_P100A_PREVIEW
			"Haiku Assistant — P100A story preview",
#else
			"Haiku Assistant",
#endif
			B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS), fThreadsLoaded(false), fBusy(false), fRequest(0)
	{
		fTranscript = new BTextView("conversation");
		fTranscript->MakeEditable(false);
		fTranscript->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
		fInput = new BTextView("message");
		fInput->SetMaxBytes(2000);
		fInput->SetExplicitMinSize(BSize(300, 80));
		fSend = new BButton("Send", new BMessage(kSend));
		fStop = new BButton("Stop", new BMessage(kStop));
		fClear = new BButton("New chat", new BMessage(kClear));
		fStop->SetEnabled(false);
		fStatus = new BStringView("status", "Background model service starting...");
		fThreads = new BTextControl("cpu_threads", "CPU threads:", "32", NULL);
		fThreads->TextView()->SetMaxBytes(4);
		fThreads->SetEnabled(false);
		fApplyThreads = new BButton("Apply", new BMessage(kApplyThreads));
		fApplyThreads->SetEnabled(false);
		fModels = new BButton("Models…", new BMessage(kOpenModels));
		fThreadHint = new BStringView("thread_hint", "Saved; applies to next CPU request");
		BButton* hardware = new BButton("hardware_settings", "Hardware…", new BMessage(kOpenHardware));
		fModelLabel = new BStringView("active model", "Chat backend: CPU — starting service");
		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.AddGroup(B_HORIZONTAL).Add(fModelLabel).AddGlue().Add(hardware).Add(fModels).End()
			.AddGroup(B_HORIZONTAL)
				.Add(fThreads).Add(fApplyThreads)
				.Add(fThreadHint)
			.End()
			.Add(new BScrollView("history", fTranscript, 0, false, true), 1)
			.Add(new BScrollView("input", fInput, 0, false, true), 0)
			.AddGroup(B_HORIZONTAL)
				.Add(fClear).AddGlue().Add(fStop).Add(fSend)
			.End().Add(fStatus);
		AddShortcut(B_RETURN, B_CONTROL_KEY, new BMessage(kSend));
		BMessage check(kCheck);
		fCheck = new BMessageRunner(BMessenger(this), &check, 1000000);
		fInput->MakeFocus();
		fSession = new ToolSession;
		fSession->Run();
	}
	virtual ~AssistantWindow()
	{
		delete fCheck;
		if (fSession->Lock()) fSession->Quit();
	}
	virtual bool QuitRequested()
	{
		Cancel();
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kOpenHardware:
				if (fHardwareWindow.IsValid()) fHardwareWindow.SendMessage(kShowHardwareWindow);
				else {
					HardwareWindow* hardware = new HardwareWindow;
					fHardwareWindow = BMessenger(hardware); hardware->Show();
				}
				break;
			case kOpenModels:
				if (fModelWindow.IsValid()) fModelWindow.SendMessage(kShowModelManager);
				else {
					ModelManager* manager = new ModelManager;
					fModelWindow = BMessenger(manager); manager->Show();
				}
				break;
			case kApplyThreads: ApplyThreads(); break;
			case kToggle:
				if (!IsHidden() && IsActive()) {
					Hide();
					break;
				}
				// fall through
			case kShow:
				if (IsHidden()) Show();
				Activate();
				break;
			case kSend: Start(); break;
			case kStop: Cancel(); break;
			case kClear:
				if (!fBusy) { fTurns.clear(); fTranscript->SetText(""); }
				break;
			case kCheck:
			{
				BMessenger service(kServiceSignature);
				if (!service.IsValid()) {
					SetBusy(false);
					fStatus->SetText("Service stopped. Send will try to restart it.");
				} else if (!fBusy) {
					BMessage query(kServiceStatus), reply;
					if (service.SendMessage(&query, &reply, 100000, 100000) == B_OK) {
						bool ready = false;
						int32 threads;
						if (!fThreadsLoaded && reply.FindInt32("cpu_threads", &threads) == B_OK) {
							char value[24]; snprintf(value, sizeof(value), "%ld", (long)threads);
							fThreads->SetText(value); fThreadsLoaded = true;
							fThreads->SetEnabled(true); fApplyThreads->SetEnabled(true);
						}
						reply.FindBool("ready", &ready);
						bool loading = false;
						const char* path = ""; const char* error = "";
						reply.FindBool("loading", &loading);
						reply.FindString("model_path", &path); reply.FindString("model_error", &error);
						// A temporary empty path during service restart/loading is
						// not a successful model change; keep the conversation.
						if (ready && !loading && *path && fModelPath != path) {
							fModelPath = path; fTurns.clear(); fTranscript->SetText("");
						}
						const char* backend = "cpu";
						reply.FindString("backend_id", &backend);
						bool story = strcmp(backend, "p100a") == 0;
						fThreads->SetEnabled(!story && fThreadsLoaded);
						fApplyThreads->SetEnabled(!story && fThreadsLoaded);
						fThreadHint->SetText(story ? "P100A host path: 1 thread; CPU setting retained"
							: "Saved; applies to next CPU request");
						BString label(story ? "P100A + CPU (experimental story mode): "
							: (AssistantHardwareCanChat(backend) ? "CPU: " : "Hardware unavailable: "));
						label << (*path ? BPath(path).Leaf() : "no model loaded");
						fModelLabel->SetText(label.String());
						fSend->SetEnabled(ready && !loading);
						fStatus->SetText(loading ? "Verifying/loading model..."
							: (*error ? error : (ready ? "Model loaded. Ctrl+Space shows/hides this window."
							: "No model loaded. Open Models to install/select one.")));
					}
				}
				break;
			}
			case kUpdate:
			case kDone:
			{
				int64 id;
				if (!fBusy || message->FindInt64("id", &id) != B_OK || id != fRequest)
					break;
				const char* text;
				const char* status;
				if (message->FindString("text", &text) == B_OK) {
					std::string display = fDisplay + text;
					fTranscript->SetText(display.c_str());
					fTranscript->Select(fTranscript->TextLength(), fTranscript->TextLength());
					fTranscript->ScrollToSelection();
				}
				if (message->FindString("status", &status) == B_OK)
					fStatus->SetText(status);
				if (message->what == kDone) {
					bool success = false;
					message->FindBool("success", &success);
					if (success && message->FindString("text", &text) == B_OK)
						fTurns.push_back(Turn{fPending, text});
					SetBusy(false);
				}
				break;
			}
			default: BWindow::MessageReceived(message);
		}
	}
private:
	void ApplyThreads()
	{
		const char* text = fThreads->Text();
		char* end;
		long threads = strtol(text, &end, 10);
		BMessage request(kConfigureThreads), reply;
		status_t result = B_BAD_VALUE;
		if (end != text && *end == '\0' && threads >= 1 && threads <= 4096) {
			request.AddInt32("cpu_threads", (int32)threads);
			result = BMessenger(kServiceSignature).SendMessage(&request, &reply, 1000000, 2000000);
			if (result == B_OK && reply.FindInt32("status", &result) != B_OK) result = B_ERROR;
		}
		if (result != B_OK) {
			(new BAlert("CPU threads", "Could not save CPU threads. Enter a whole number from 1 to the available CPU count, and make sure the service is running and settings are writable.", "OK"))->Go(NULL);
		} else fStatus->SetText("CPU threads saved. Applies to the next model request; no reload needed.");
	}
	void Cancel()
	{
		if (!fBusy) return;
		BMessage stop(kCancel);
		stop.AddInt64("id", fRequest);
		stop.AddMessenger("target", BMessenger(this));
		fService.SendMessage(&stop, (BHandler*)NULL, 100000);
		fStatus->SetText("Stopping...");
	}
	void SetBusy(bool busy)
	{
		fBusy = busy;
		fSend->SetEnabled(!busy);
		fStop->SetEnabled(busy);
		fClear->SetEnabled(!busy);
		fInput->MakeEditable(!busy);
	}
	void Start()
	{
		if (fBusy || fInput->TextLength() == 0) return;
		status_t result = StartService();
		fService = BMessenger(fSession);
		if (result != B_OK || !fService.IsValid()) {
			fStatus->SetText("Could not start service. Check installation and try again.");
			return;
		}
		BMessage query(kServiceStatus), state;
		bool ready = false, loading = true;
		const char* path = "";
		const char* backend = "";
		if (BMessenger(kServiceSignature).SendMessage(&query, &state, 100000, 100000) == B_OK) {
			state.FindBool("ready", &ready); state.FindBool("loading", &loading);
			state.FindString("model_path", &path);
			state.FindString("backend_id", &backend);
		}
		if (!ready || loading) { fStatus->SetText("Model is not ready. Open Models to check its status."); return; }
		if (!AssistantHardwareCanChat(backend)) {
			fStatus->SetText("Hardware backend unavailable. Open Hardware; an updated service is required."); return;
		}
#ifdef ASSISTANT_P100A_PREVIEW
		if (strcmp(backend, "p100a") != 0) {
			fStatus->SetText("This preview is for P100A stories only. Select P100A in Hardware first."); return;
		}
#endif
		if (fModelPath != path) { fModelPath = path; fTurns.clear(); fTranscript->SetText(""); }
		fPending = fInput->Text();
		size_t bytes = fPending.size();
		for (size_t i = 0; i < fTurns.size(); i++)
			bytes += fTurns[i].user.size() + fTurns[i].assistant.size();
		while (bytes > 3000 && !fTurns.empty()) {
			bytes -= fTurns[0].user.size() + fTurns[0].assistant.size();
			fTurns.erase(fTurns.begin());
		}
		std::string prompt = std::string("<|im_start|>system\n")
			+ AssistantToolInstructions() + "<|im_end|>\n";
		fDisplay.clear();
		for (size_t i = 0; i < fTurns.size(); i++) {
			const Turn& turn = fTurns[i];
			prompt += "<|im_start|>user\n" + EscapeToolData(turn.user) + "<|im_end|>\n"
				"<|im_start|>assistant\n" + EscapeToolData(turn.assistant) + "<|im_end|>\n";
			fDisplay += "You: " + turn.user + "\n\nQwen: " + turn.assistant + "\n\n";
		}
		prompt += "<|im_start|>user\n" + EscapeToolData(fPending)
			+ " /no_think<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
		fDisplay += "You: " + fPending + "\n\nQwen: ";
		bool story = strcmp(backend, "p100a") == 0;
		if (story) {
			// This base model is not instruction-tuned. No chat history,
			// ChatML, tools, system instructions or hidden CPU fallback.
			prompt = fPending;
			fDisplay = "Experimental TinyStories — real P100A projections + CPU attention/verification.\n"
				"Not Qwen chat. Up to 128 printable ASCII characters / 16 prompt tokens; 8 output tokens.\n\n"
				"Story beginning: " + fPending + "\n\nContinuation: ";
			fService = BMessenger(kServiceSignature);
		}
		fRequest = system_time();
		BMessage request(kGenerate);
		request.AddInt64("id", fRequest);
		request.AddMessenger("target", BMessenger(this));
		request.AddString("prompt", prompt.c_str());
		request.AddString("model_path", fModelPath.c_str());
		request.AddString("backend_id", backend);
		if (story) { request.AddString("prompt_format", "story"); request.AddInt32("max_tokens", 8); }
		if (fService.SendMessage(&request, (BHandler*)NULL, 100000) != B_OK) {
			fStatus->SetText("Service did not accept the request.");
			return;
		}
		fInput->SetText("");
		fTranscript->SetText(fDisplay.c_str());
		fStatus->SetText("Request sent to resident model...");
		SetBusy(true);
	}
	BTextView* fTranscript;
	BTextView* fInput;
	BButton* fSend;
	BButton* fStop;
	BButton* fClear;
	BStringView* fStatus;
	BTextControl* fThreads;
	BButton* fApplyThreads;
	BButton* fModels;
	BStringView* fModelLabel;
	BStringView* fThreadHint;
	BMessenger fModelWindow;
	BMessenger fHardwareWindow;
	std::string fModelPath;
	bool fThreadsLoaded;
	BMessageRunner* fCheck;
	ToolSession* fSession;
	bool fBusy;
	int64 fRequest;
	BMessenger fService;
	std::vector<Turn> fTurns;
	std::string fDisplay;
	std::string fPending;
};

class AssistantApp : public BApplication {
public:
	AssistantApp() : BApplication(
#ifdef ASSISTANT_P100A_PREVIEW
		"application/x-vnd.Haiku-AssistantP100A"
#else
		kAssistantSignature
#endif
	) {}
	virtual void ReadyToRun()
	{
		StartService();
		AssistantWindow* window = new AssistantWindow();
		fWindow = BMessenger(window);
		window->Show();
	}
	virtual void ArgvReceived(int32 argc, char** argv)
	{
		fWindow.SendMessage(kShow);
		if (argc == 2 && strcmp(argv[1], "--models") == 0) PostMessage(kOpenModels);
		if (argc == 2 && strcmp(argv[1], "--hardware") == 0) PostMessage(kOpenHardware);
	}
	virtual void RefsReceived(BMessage*) { fWindow.SendMessage(kShow); }
	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == kToggle || message->what == kOpenModels || message->what == kOpenHardware)
			fWindow.SendMessage(message);
		else BApplication::MessageReceived(message);
	}
private:
	BMessenger fWindow;
};

int main()
{
	AssistantApp app;
	app.Run();
	return 0;
}
