/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT) || PIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Full-card prompt client requires its separate exact role,never CPU inference or hybrid Work
#endif
#include "P100AQwen27FullOffload.h"
#include "Qwen27Protocol.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>
#include <cmath>
#include <cstdio>

namespace {
namespace A = Assistant27;
#ifndef ASSISTANT_QWEN27_FULL_OFFLOAD_WORKER
const char* const Worker = "/boot/home/config/non-packaged/servers/HaikuAssistantQwen27FullOffload";
#else
const char* const Worker = ASSISTANT_QWEN27_FULL_OFFLOAD_WORKER; // isolated build-time stage/test only
#endif
double Now()
{
	timespec time = {};
	return clock_gettime(CLOCK_MONOTONIC, &time) ? -1
		: double(time.tv_sec) + double(time.tv_nsec) * 1e-9;
}

struct Channel {
	int fd = -1;
	pid_t child = -1;
	uint32_t id = 0;
	A::Receiver receiver;

	bool Open()
	{
		int pair[2];
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) return false;
		for (int item : pair) {
			if (fcntl(item, F_SETFD, FD_CLOEXEC)) {
				close(pair[0]); close(pair[1]); return false;
			}
		}
		child = fork();
		if (!child) {
			if (dup2(pair[1], 0) < 0 || dup2(pair[1], 1) < 0
				|| fcntl(0, F_SETFD, 0) < 0 || fcntl(1, F_SETFD, 0) < 0) _exit(126);
			for (int item : pair) if (item > 2) close(item);
			char arg[] = "--assistant-channel";
			char* args[] = {const_cast<char*>(Worker), arg, nullptr};
			execv(Worker, args); _exit(127);
		}
		close(pair[1]);
		if (child < 0) { close(pair[0]); return false; }
		fd = pair[0]; return true;
	}
	bool Send(unsigned kind, unsigned value = 0, const std::string& text = "")
	{
		if (fd < 0 || id == UINT32_MAX) return false;
		A::Frame frame; frame.kind = kind; frame.id = ++id; frame.value = value; frame.text = text;
		return A::Send(fd, frame);
	}
	bool Next(A::Frame& frame, double deadline, const std::function<bool()>* cancel = nullptr,
		bool* sent = nullptr)
	{
		double previous = Now();
		if (previous < 0) return false;
		for (;;) {
			const double now = Now();
			if (now < previous || now > deadline) return false;
			previous = now;
			if (cancel && sent && !*sent && (*cancel)()) {
				A::Frame stop; stop.kind = A::Cancel; stop.id = id;
				if (!A::Send(fd, stop)) return false;
				*sent = true; // monotonic request,not a signal/reset/cleanup claim
			}
			const auto result = receiver.Read(fd, frame);
			if (result == A::Receiver::Received) {
				const double received = Now();
				return frame.id == id && received >= previous && received <= deadline;
			}
			if (result != A::Receiver::Waiting) return false;
			pollfd item = {fd, POLLIN, 0};
			if (poll(&item, 1, 100) < 0 && errno != EINTR) return false;
		}
	}
	bool Exit(unsigned expected)
	{
		if (fd >= 0) { close(fd); fd = -1; }
		const double began = Now();
		if (child <= 0 || began < 0) return false;
		double previous = began;
		for (;;) {
			const double now = Now();
			if (now < previous || now > began + 5) return false;
			previous = now;
			int status = 0;
			const auto result = waitpid(child, &status, WNOHANG);
			if (result == child) {
				child = -1;
				return WIFEXITED(status) && unsigned(WEXITSTATUS(status)) == expected;
			}
			if (result < 0 && errno != EINTR) return false;
			poll(nullptr, 0, 10); // process bookkeeping only
		}
	}
	bool Unload()
	{
		A::Frame frame;
		const double began = Now();
		return began >= 0 && Send(A::Unload) && Next(frame, began + 120)
			&& frame.kind == A::Closed && !frame.value
			&& frame.text == "QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END" && Exit(0);
	}
	~Channel()
	{
		if (fd >= 0) close(fd);
		if (child > 0) { int status = 0; waitpid(child, &status, WNOHANG); }
	} // no destructor IO to device,END,signal,kill,retry,reset or fallback
};

bool Progress(const A::Frame& frame, const std::function<void(float, const char*)>& progress)
{
	if (frame.kind != A::Progress || (frame.value > 1000 && frame.value != UINT32_MAX)) return false;
	progress(frame.value == UINT32_MAX ? -1 : float(frame.value) / 1000, frame.text.c_str());
	return true;
}

bool Summary(const std::string& text, unsigned requested, unsigned& tokens, bool requireResident)
{
	double compute = 0, total = 0;
	int end = -1;
	const bool resident = text.compare(0, sizeof("P100A full-offload resident candidate:") - 1,
		"P100A full-offload resident candidate:") == 0;
	const char* format = resident
		? "P100A full-offload resident candidate: %u output tokens; prompt+generation %lfs,total %lfs. All inference on card; CPU vocabulary/text only. Request closed cleanly; weights retained until explicit model END. Not a warm-token speed measurement.%n"
		: "P100A full-offload candidate: %u output tokens; prompt+generation %lfs,total %lfs. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement.%n";
	return resident == requireResident && std::sscanf(text.c_str(), format,
		&tokens, &compute, &total, &end) == 3 && end >= 0 && size_t(end) == text.size()
		&& tokens <= requested && std::isfinite(compute) && std::isfinite(total)
		&& compute > 0 && total >= compute;
} // returned DATA shape only;not proof of physical device END or ownership
}

struct AssistantP100AQwen27FullOffloadSession::Data {
	Channel channel;
	bool loaded = false;
	bool faulted = false;
	bool rejected = false;
	bool requireResident = true;
};

AssistantP100AQwen27FullOffloadSession::AssistantP100AQwen27FullOffloadSession(bool requireResidentWorker)
	:fData(new Data)
{
	fData->requireResident = requireResidentWorker;
}

bool AssistantP100AQwen27FullOffloadSession::Available()
{
	return access(Worker, X_OK) == 0;
}

AssistantP100AQwen27FullOffloadSession::~AssistantP100AQwen27FullOffloadSession()
{
	// Normal destruction is an explicit bounded END/exit attempt. Faulted
	// sessions are never retried or reset; their state remains unknown.
	if (fData->loaded && !fData->faulted) {
		std::string ignored;
		Unload(ignored);
	}
	delete fData;
}

bool AssistantP100AQwen27FullOffloadSession::Load(
	const std::function<void(float, const char*)>& progress, std::string& error)
{
	if (fData->loaded || fData->faulted || !progress) {
		error = "Full-card session is already loaded, faulted, or has no progress callback.";
		return false;
	}
	error = "Full-card worker/channel failed; explicit close only,no retry/reset or CPU fallback.";
	const double began = Now();
	if (began < 0 || !fData->channel.Open() || !fData->channel.Send(A::Load, 1)) {
		fData->faulted = true; return false;
	}
	A::Frame frame;
	bool ready = false;
	while (fData->channel.Next(frame, began + 1800)) {
		if (Progress(frame, progress)) continue;
		if (frame.kind == A::Error && frame.value == A::Rejected) {
			if (!fData->channel.Exit(1)) { fData->faulted = true; return false; }
			fData->rejected = true; error = frame.text; return false;
		}
		ready = frame.kind == A::Ready && !frame.value
			&& frame.text == (fData->requireResident ? "QWEN27_FULL_OFFLOAD_RESIDENT_VOCAB_READY"
				: "QWEN27_FULL_OFFLOAD_VOCAB_READY");
		break;
	}
	if (!ready) { fData->faulted = true; return false; } // hybrid readiness is NEVER accepted
	fData->loaded = true; error.clear(); return true;
}

AssistantP100AQwen27FullOffloadSession::Reply
AssistantP100AQwen27FullOffloadSession::Complete(const std::string& question, unsigned tokens,
	const std::function<bool()>& cancelled, const std::function<void(float, const char*)>& progress)
{
	Reply reply;
	if (!fData->loaded || fData->faulted || !Assistant27::Question(question) || !tokens
		|| tokens > Assistant27::MaxTokens || !cancelled || !progress) {
		reply.outcome = AssistantP100AQwen27FullOffload::Rejected;
		reply.error = "Full-card session requires a loaded worker,1-256 question bytes and1-32 tokens; no request executed.";
		return reply;
	}
	if (cancelled()) { reply.outcome = AssistantP100AQwen27FullOffload::Cancelled; reply.error = "Stopped before accelerator BEGIN."; return reply; }
	reply.error = "Full-card worker/channel failed; explicit close only,no retry/reset or CPU fallback.";
	const double generate = Now();
	if (generate < 0 || !fData->channel.Send(A::Generate, tokens, question)) {
		fData->faulted = true; return reply;
	}
	bool sent = false, text = false, done = false;
	std::string answer, status;
	AssistantP100AQwen27FullOffload::Outcome outcome = AssistantP100AQwen27FullOffload::Unknown;
	unsigned generated = 0;
	A::Frame frame;
	while (fData->channel.Next(frame, generate + 7200, &cancelled, &sent)) {
		if (frame.kind == A::Progress) {
			if (text || !Progress(frame, progress)) { fData->faulted = true; return reply; }
			continue;
		}
		if (frame.kind == A::Text && !frame.value && !text) { answer = frame.text; text = true; continue; }
		if (frame.kind != A::Done || frame.value > A::Rejected) { fData->faulted = true; return reply; }
		if (frame.value == A::Complete) {
			if (!text || !Summary(frame.text, tokens, generated, fData->requireResident)) { fData->faulted = true; return reply; }
			outcome = AssistantP100AQwen27FullOffload::Completed;
		} else {
			if (text) { fData->faulted = true; return reply; }
			outcome = frame.value == A::Cancelled ? AssistantP100AQwen27FullOffload::Cancelled
				: AssistantP100AQwen27FullOffload::Rejected;
		}
		status = frame.text; done = true; break;
	}
	if (!done) { fData->faulted = true; return reply; }
	reply.outcome = outcome; reply.cleanWorkerExit = false; reply.status = status;
	if (outcome == AssistantP100AQwen27FullOffload::Completed && !sent) {
		reply.answer = answer; reply.outputTokens = generated; reply.error.clear();
	} else if (outcome == AssistantP100AQwen27FullOffload::Completed) {
		// A cancellation that races a clean completed reply still suppresses text.
		reply.outcome = AssistantP100AQwen27FullOffload::Cancelled; reply.error = "Stopped; completed text suppressed after clean worker exit.";
	} else reply.error = status;
	return reply;
}

bool AssistantP100AQwen27FullOffloadSession::Unload(std::string& error)
{
	if (fData->faulted || !fData->loaded || !fData->channel.Unload()) {
		fData->faulted = true;
		error = "Full-card model END/worker exit was not verified; no retry/reset or CPU fallback.";
		return false;
	}
	fData->loaded = false; error.clear(); return true;
}

bool AssistantP100AQwen27FullOffloadSession::Loaded() const { return fData->loaded; }
bool AssistantP100AQwen27FullOffloadSession::Faulted() const { return fData->faulted; }
bool AssistantP100AQwen27FullOffloadSession::RejectedByWorker() const { return fData->rejected; }

AssistantP100AQwen27FullOffload::Reply
AssistantP100AQwen27FullOffload::Run(const std::string& question, unsigned tokens,
	const std::function<bool()>& cancelled, const std::function<void(float, const char*)>& progress)
{
	Reply reply;
	if (!Assistant27::Question(question) || !tokens || tokens > Assistant27::MaxTokens
		|| !cancelled || !progress) {
		reply.outcome = Rejected;
		reply.error = "Full-card Qwen27B requires1-256 question bytes and1-32 tokens; no request executed.";
		return reply;
	}
	if (cancelled()) { reply.outcome = Cancelled; reply.error = "Stopped before worker launch."; return reply; }
#ifdef PIONEER_Q38_FULL_OFFLOAD_RESIDENT_CLIENT
	AssistantP100AQwen27FullOffloadSession session(true);
#else
	AssistantP100AQwen27FullOffloadSession session(false);
#endif
	if (!session.Load(progress, reply.error)) {
		reply.outcome = session.RejectedByWorker() ? Rejected : Unknown;
		return reply;
	}
	reply = session.Complete(question, tokens, cancelled, progress);
	std::string unloadError;
	if (!session.Unload(unloadError)) {
		if (reply.outcome == Completed) { reply.answer.clear(); reply.outputTokens = 0; }
		reply.outcome = Unknown; reply.cleanWorkerExit = false; reply.error = unloadError;
		return reply;
	}
	reply.cleanWorkerExit = true;
	return reply;
}
