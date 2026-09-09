/*
 * Copyright 2026. Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>

#include <atomic>
#include <string>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

static const uint32 kSend = 'send';
static const uint32 kStop = 'stop';
static const uint32 kClear = 'cler';
static const uint32 kUpdate = 'updt';
static const uint32 kDone = 'done';
static const uint32 kShow = 'show';

struct Turn {
	std::string user;
	std::string assistant;
};

// Only the worker owns the child PID and descriptors. The UI requests cancellation.
struct Generation {
	BMessenger target;
	std::atomic<bool> cancel;
	std::string prompt;
	std::string runtime;
	std::string model;

	Generation(BWindow* window)
		: target(window), cancel(false)
	{
		const char* value = getenv("LLAMA_RUNTIME");
		runtime = value != NULL ? value
			: "/boot/home/develop/llama-c060ca974c77/build-pioneer/bin/llama-completion";
		value = getenv("LLAMA_MODEL");
		model = value != NULL ? value : "/boot/home/models/Qwen3-0.6B-Q8_0.gguf";
	}
};

static void
SendResult(Generation* job, uint32 what, const std::string& text,
	const std::string& status, bool success = false)
{
	BMessage message(what);
	message.AddString("text", text.c_str());
	message.AddString("status", status.c_str());
	message.AddBool("success", success);
	job->target.SendMessage(&message, (BHandler*)NULL, 100000);
}

static int32
Generate(void* data)
{
	Generation* job = static_cast<Generation*>(data);
	if (access(job->runtime.c_str(), X_OK) != 0
		|| access(job->model.c_str(), R_OK) != 0) {
		SendResult(job, kDone, "Runtime or model is unavailable.\nRuntime: "
			+ job->runtime + "\nModel: " + job->model,
			"Check LLAMA_RUNTIME and LLAMA_MODEL.");
		return 0;
	}
	int output[2];
	int errors[2];
	if (pipe(output) != 0) {
		SendResult(job, kDone, "", strerror(errno));
		return 0;
	}
	if (pipe(errors) != 0) {
		int error = errno;
		close(output[0]);
		close(output[1]);
		SendResult(job, kDone, "", strerror(error));
		return 0;
	}
	posix_spawn_file_actions_t actions;
	int error = posix_spawn_file_actions_init(&actions);
	bool initialized = error == 0;
	if (error == 0)
		error = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO,
			"/dev/null", O_RDONLY, 0);
	if (error == 0)
		error = posix_spawn_file_actions_adddup2(&actions, output[1], STDOUT_FILENO);
	if (error == 0)
		error = posix_spawn_file_actions_adddup2(&actions, errors[1], STDERR_FILENO);
	int descriptors[] = {output[0], output[1], errors[0], errors[1]};
	for (unsigned i = 0; error == 0 && i < 4; i++)
		error = posix_spawn_file_actions_addclose(&actions, descriptors[i]);

	const char* arguments[] = {job->runtime.c_str(), "-m", job->model.c_str(),
		"-t", "4", "-tb", "4", "-c", "4096", "-b", "64", "-ub", "64",
		"-n", "256", "--no-conversation", "--no-display-prompt", "--simple-io",
		"-p", job->prompt.c_str(), NULL};
	pid_t child = -1;
	if (error == 0 && !job->cancel.load()) {
		error = posix_spawn(&child, job->runtime.c_str(), &actions, NULL,
			const_cast<char* const*>(arguments), environ);
	}
	if (initialized)
		posix_spawn_file_actions_destroy(&actions);
	close(output[1]);
	close(errors[1]);
	if (error != 0 || child < 0) {
		close(output[0]);
		close(errors[0]);
		SendResult(job, kDone, "", error != 0 ? strerror(error) : "Stopped.");
		return 0;
	}

	std::string answer;
	std::string diagnostics;
	bool killed = false;
	bool overflow = false;
	int childStatus = 0;
	bool reaped = false;
	bigtime_t lastUpdate = 0;
	pollfd streams[2] = {{output[0], POLLIN, 0}, {errors[0], POLLIN, 0}};
	while (!reaped || streams[0].fd >= 0 || streams[1].fd >= 0) {
		if ((job->cancel.load() || overflow) && !killed && !reaped) {
			kill(child, SIGKILL);
			killed = true;
		}
		int ready = poll(streams, 2, 100);
		if (ready < 0 && errno != EINTR) {
			overflow = true;
			break;
		}
		for (int i = 0; i < 2; i++) {
			if (streams[i].fd < 0 || streams[i].revents == 0)
				continue;
			char buffer[2048];
			ssize_t size = read(streams[i].fd, buffer, sizeof(buffer));
			if (size > 0) {
				std::string& destination = i == 0 ? answer : diagnostics;
				destination.append(buffer, size);
				if (i == 1 && destination.size() > 8192)
					destination.erase(0, destination.size() - 8192);
				if (i == 0 && destination.size() > 65536) {
					destination.resize(65536);
					overflow = true;
				}
			} else if (size == 0 || (errno != EINTR && errno != EAGAIN)) {
				close(streams[i].fd);
				streams[i].fd = -1;
			}
		}
		// Send complete snapshots; a split UTF-8 token is repaired on the next update.
		if (system_time() - lastUpdate >= 100000) {
			SendResult(job, kUpdate, answer,
				answer.empty() ? "Loading model / processing prompt..." : "Generating...");
			lastUpdate = system_time();
		}
		if (!reaped) {
			pid_t result = waitpid(child, &childStatus, WNOHANG);
			if (result == child || (result < 0 && errno != EINTR)) {
				reaped = true;
				if (result < 0)
					overflow = true;
			}
		}
	}
	for (int i = 0; i < 2; i++) {
		if (streams[i].fd >= 0)
			close(streams[i].fd);
	}
	if (!reaped) {
		kill(child, SIGKILL);
		while (waitpid(child, &childStatus, 0) < 0 && errno == EINTR) {}
	}
	bool success = !job->cancel.load() && !overflow
		&& WIFEXITED(childStatus) && WEXITSTATUS(childStatus) == 0;
	const std::string endMarker = " [end of text]\n";
	size_t end = answer.rfind(endMarker);
	if (end != std::string::npos
		&& answer.find_first_not_of("\r\n", end + endMarker.size()) == std::string::npos)
		answer.erase(end);
	std::string status = "Ready. Replies use recent conversation context.";
	if (job->cancel.load())
		status = "Stopped. Partial reply was not added to model context.";
	else if (!success) {
		status = "Inference failed; details appear above.";
		answer += "\n\n[Inference failed]\n" + diagnostics;
	}
	SendResult(job, kDone, answer, status, success);
	return 0;
}

class AssistantWindow : public BWindow {
public:
	AssistantWindow()
		: BWindow(BRect(100, 100, 820, 680), "Haiku Assistant",
			B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
			fJob(NULL), fWorker(-1)
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
		fStatus = new BStringView("status", "Local Qwen chat. No system tools enabled.");
		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.Add(new BScrollView("history", fTranscript, 0, false, true), 1)
			.Add(new BScrollView("input", fInput, 0, false, true), 0)
			.AddGroup(B_HORIZONTAL)
				.Add(fClear).AddGlue().Add(fStop).Add(fSend)
			.End()
			.Add(fStatus);
		AddShortcut(B_RETURN, B_CONTROL_KEY, new BMessage(kSend));
		fInput->MakeFocus();
	}

	virtual bool QuitRequested()
	{
		if (fJob != NULL) {
			fJob->cancel.store(true);
			status_t result;
			wait_for_thread(fWorker, &result);
			delete fJob;
			fJob = NULL;
		}
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case kShow:
				if (IsHidden())
					Show();
				Activate();
				break;
			case kSend:
				Start();
				break;
			case kStop:
				if (fJob != NULL) {
					fJob->cancel.store(true);
					fStatus->SetText("Stopping...");
				}
				break;
			case kClear:
				if (fJob == NULL) {
					fTurns.clear();
					fTranscript->SetText("");
					fStatus->SetText("New chat. No conversation is saved to disk.");
				}
				break;
			case kUpdate:
			case kDone:
			{
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
				if (message->what == kDone && fJob != NULL) {
					status_t result;
					wait_for_thread(fWorker, &result);
					delete fJob;
					fJob = NULL;
					bool success = false;
					message->FindBool("success", &success);
					if (success && message->FindString("text", &text) == B_OK)
						fTurns.push_back(Turn{fPending, text});
					SetBusy(false);
				}
				break;
			}
			default:
				BWindow::MessageReceived(message);
		}
	}

private:
	void SetBusy(bool busy)
	{
		fSend->SetEnabled(!busy);
		fStop->SetEnabled(busy);
		fClear->SetEnabled(!busy);
		fInput->MakeEditable(!busy);
		if (!busy)
			fInput->MakeFocus();
	}

	void Start()
	{
		if (fJob != NULL || fInput->TextLength() == 0)
			return;
		fPending = fInput->Text();
		size_t bytes = fPending.size();
		for (size_t i = 0; i < fTurns.size(); i++)
			bytes += fTurns[i].user.size() + fTurns[i].assistant.size();
		while (bytes > 6000 && !fTurns.empty()) {
			bytes -= fTurns[0].user.size() + fTurns[0].assistant.size();
			fTurns.erase(fTurns.begin());
		}
		fJob = new Generation(this);
		fJob->prompt = "<|im_start|>system\nYou are a helpful Haiku assistant. "
			"You cannot inspect or operate this computer. Be concise."
			"<|im_end|>\n";
		fDisplay.clear();
		for (size_t i = 0; i < fTurns.size(); i++) {
			const Turn& turn = fTurns[i];
			fJob->prompt += "<|im_start|>user\n" + turn.user
				+ "<|im_end|>\n<|im_start|>assistant\n" + turn.assistant
				+ "<|im_end|>\n";
			fDisplay += "You: " + turn.user + "\n\nQwen: " + turn.assistant + "\n\n";
		}
		fJob->prompt += "<|im_start|>user\n" + fPending
			+ " /no_think<|im_end|>\n<|im_start|>assistant\n<think>\n\n</think>\n\n";
		fDisplay += "You: " + fPending + "\n\nQwen: ";
		fWorker = spawn_thread(Generate, "assistant inference", B_NORMAL_PRIORITY, fJob);
		status_t error = fWorker < 0 ? fWorker : resume_thread(fWorker);
		if (error < B_OK) {
			if (fWorker >= 0)
				kill_thread(fWorker);
			delete fJob;
			fJob = NULL;
			fStatus->SetText(strerror(error));
			return;
		}
		fInput->SetText("");
		fTranscript->SetText(fDisplay.c_str());
		fStatus->SetText("Starting local inference...");
		SetBusy(true);
	}

	BTextView* fTranscript;
	BTextView* fInput;
	BButton* fSend;
	BButton* fStop;
	BButton* fClear;
	BStringView* fStatus;
	Generation* fJob;
	thread_id fWorker;
	std::vector<Turn> fTurns;
	std::string fDisplay;
	std::string fPending;
};

class AssistantApp : public BApplication {
public:
	AssistantApp() : BApplication("application/x-vnd.Haiku-Assistant") {}
	virtual void ReadyToRun()
	{
		AssistantWindow* window = new AssistantWindow();
		fWindow = BMessenger(window);
		window->Show();
	}
	virtual void ArgvReceived(int32, char**) { fWindow.SendMessage(kShow); }
	virtual void RefsReceived(BMessage*) { fWindow.SendMessage(kShow); }

private:
	BMessenger fWindow;
};

int
main()
{
	// GUI launches may have closed standard descriptors. Keep pipe FDs above 2.
	for (int fd = 0; fd < 3; fd++) {
		if (fcntl(fd, F_GETFD) >= 0)
			continue;
		int opened = open("/dev/null", O_RDWR);
		if (opened < 0)
			return 1;
		if (opened != fd) {
			int result = dup2(opened, fd);
			close(opened);
			if (result < 0)
				return 1;
		}
	}
	AssistantApp app;
	app.Run();
	return 0;
}
