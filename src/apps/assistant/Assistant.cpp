/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
#include "ToolBridge.h"
#include "ToolSession.h"
#include "ModelManager.h"
#include "HardwareWindow.h"
#include "ServiceLauncher.h"
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
#include <stdio.h>
#include <Window.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>

static const uint32 kSend = 'send', kStop = 'stop', kClear = 'cler';
static const uint32 kShow = 'show', kCheck = 'chek';
static const uint32 kOpenModels = 'omod';
static const uint32 kOpenHardware = 'ohdw';

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
			B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS), fBusy(false), fRequest(0)
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
		fModels = new BButton("Models…", new BMessage(kOpenModels));
		BButton* hardware = new BButton("hardware_settings", "Hardware…", new BMessage(kOpenHardware));
		fModelLabel = new BStringView("active model", "Chat backend: CPU — starting service");
		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.AddGroup(B_HORIZONTAL).Add(fModelLabel).AddGlue().Add(hardware).Add(fModels).End()
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
					fSend->SetEnabled(false);
					fStatus->SetText("Service stopped. Open Models and click Load model to restart it.");
				} else if (!fBusy) {
					BMessage query(kServiceStatus), reply;
					if (service.SendMessage(&query, &reply, 100000, 100000) == B_OK) {
						bool ready = false;
						reply.FindBool("ready", &ready);
						bool loading = false;
						bool unloading = false, serviceBusy = false;
						const char* path = ""; const char* error = "";
						reply.FindBool("loading", &loading);
						reply.FindBool("unloading", &unloading); reply.FindBool("busy", &serviceBusy);
						bool changing = loading || unloading;
						fSend->SetEnabled(!fBusy && !serviceBusy && ready && !changing);
						reply.FindString("model_path", &path); reply.FindString("model_error", &error);
						const char* backend = "cpu";
						reply.FindString("backend_id", &backend);
						// A temporary empty path during service restart/loading is
						// not a successful model change; keep the conversation.
						if (ready && !changing && *path && (fModelPath != path || fBackend != backend)) {
							fModelPath = path; fBackend=backend; fTurns.clear(); fTranscript->SetText("");
						}
						bool story = strcmp(backend, "p100a") == 0;
						bool qwen27=AssistantHardwareIsP100AQwen27(backend);
						bool cardQwen=AssistantHardwareIsP100AQwen(backend) || qwen27;
						bool fast=AssistantHardwareIsP100AFast(backend);
						bool attention=AssistantHardwareIsP100AAttention(backend);
						bool throughput=AssistantHardwareIsP100AThroughput(backend);
						BString label(qwen27?"P100A27B Work (experimental, slow; validation OFF): ":throughput?"P100A Throughput (multicore; validation OFF): ":attention?"P100A Attention (experimental): ":fast?"P100A Fast (CPU validation OFF): ":cardQwen ? "P100A Qwen (experimental, slower than CPU): " : story ? "P100A + CPU (experimental story mode): "
							: (AssistantHardwareCanChat(backend) ? "CPU: " : "Hardware unavailable: "));
						label << (*path ? BPath(path).Leaf() : "no model selected");
						if (!ready && !changing) label << " (unloaded)";
						fModelLabel->SetText(label.String());
						fStatus->SetText(changing ? (unloading ? "Unloading models safely..." : "Verifying/loading model...")
							: (*error ? error : serviceBusy ? "Another client is generating a reply."
							: ready ? (cardQwen ? "Host model loaded; P100A weights upload on first reply and stay resident."
							: "Model loaded. Ctrl+Space shows/hides this window.")
							: "Model unloaded. Open Models to load it or choose another."));
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
		bool wasBusy = fBusy;
		fBusy = busy;
		fSend->SetEnabled(!busy);
		fStop->SetEnabled(busy);
		fClear->SetEnabled(!busy);
		fInput->MakeEditable(!busy);
		if (!busy) { fSend->SetEnabled(false); if (wasBusy) PostMessage(kCheck); }
	}
	void Start()
	{
		if (fBusy || fInput->TextLength() == 0) return;
		fService = BMessenger(fSession);
		if (!BMessenger(kServiceSignature).IsValid() || !fService.IsValid()) {
			fStatus->SetText("Service stopped. Open Models and click Load model to restart it.");
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
		if (!ready || loading) { fStatus->SetText("Model is not ready. Open Models to load it or check its status."); return; }
		if (!AssistantHardwareCanChat(backend)) {
			fStatus->SetText("Hardware backend unavailable. Open Hardware; an updated service is required."); return;
		}
#ifdef ASSISTANT_P100A_PREVIEW
		if (strcmp(backend, "p100a") != 0) {
			fStatus->SetText("This preview is for P100A stories only. Select P100A in Hardware first."); return;
		}
#endif
		if (fModelPath != path || fBackend != backend) { fModelPath = path; fBackend=backend; fTurns.clear(); fTranscript->SetText(""); }
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
		bool qwen27=AssistantHardwareIsP100AQwen27(backend);
		bool cardQwen=AssistantHardwareIsP100AQwen(backend) || qwen27;
		bool fast=AssistantHardwareIsP100AFast(backend);
		bool attention=AssistantHardwareIsP100AAttention(backend);
		bool throughput=AssistantHardwareIsP100AThroughput(backend);
		if (cardQwen) {
			prompt=fPending;
			fDisplay=qwen27?"P100A Qwen3.8-27B Work — experimental, currently about0.13 tok/sec.\n"
				"CPU attention/state remain; CPU validation OFF. First upload roughly12 minutes.\n"
				"Single short question; no history/tools. Stop waits for a complete token/upload.\n"
				:throughput?"P100A Throughput Qwen3-0.6B — up to120 row workers; CPU validation OFF.\n"
				"Whole projections + resident card KV/attention. Hybrid host norms; eight tokens, no tools/history.\n"
				:attention?"P100A Attention Qwen3-0.6B — matrices and attention run on card; CPU validation OFF.\n"
				"Experimental PIO-staged KV; may be slower than Fast. Eight tokens; no tools/history.\n"
				:fast?"P100A Fast Qwen3-0.6B — CPU validation OFF; experimental, no CPU-speedup guarantee.\n"
				"CPU attention remains. Device safety checks retained. Eight output tokens; no tools/history.\n"
				:"Experimental P100A Qwen3-0.6B — slower than CPU; first upload takes several minutes.\n"
				"Single short question; no history or system tools. Eight checked output tokens.\n";
			fDisplay+="\nYou: "+fPending+"\n\nQwen (P100A): ";
			fService=BMessenger(kServiceSignature);
		}
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
		if (cardQwen) { request.AddString("prompt_format",AssistantHardwareQwenFormat(backend)); request.AddInt32("max_tokens",8); }
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
	BButton* fModels;
	BStringView* fModelLabel;
	BMessenger fModelWindow;
	BMessenger fHardwareWindow;
	std::string fModelPath;
	std::string fBackend;
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
		StartAssistantService();
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
