/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "ToolSession.h"
#include "ToolBridge.h"
#include "Protocol.h"
#include "ToolsProtocol.h"
#include <Entry.h>
#include <Roster.h>
static const uint32 kWatchSession = 'tsck';

ToolSession::ToolSession()
	: BLooper("assistant tool session"), fOuterID(0), fInnerID(0), fBusy(false),
		fWaitingTool(false), fToolCount(0), fDeadline(0)
{
	BMessage check(kWatchSession);
	fWatch = new BMessageRunner(BMessenger(this), &check, 1000000);
}

ToolSession::~ToolSession() { delete fWatch; Stop(); }

void ToolSession::Publish(uint32 what, const std::string& text, const char* status, bool success)
{
	BMessage reply(what);
	reply.AddInt64("id", fOuterID);
	reply.AddString("text", text.c_str());
	reply.AddString("status", status);
	reply.AddBool("success", success);
	reply.AddInt32("tool_calls", fToolCount);
	fClient.SendMessage(&reply, (BHandler*)NULL, 100000);
}

void ToolSession::Fail(const char* status)
{
	fBusy = false;
	Publish(kDone, fVisible, status);
}

void ToolSession::Stop()
{
	if (!fBusy) return;
	BMessage cancel(fWaitingTool ? kToolCancel : kCancel);
	cancel.AddInt64("id", fInnerID);
	cancel.AddMessenger("target", BMessenger(this));
	(fWaitingTool ? fTools : fModel).SendMessage(&cancel, this, 100000);
	Fail("Stopped. An already-approved action may have completed.");
}

void ToolSession::SendModel()
{
	if (fPrompt.size() > 11000) { Fail("Tool conversation limit reached. Start a new chat."); return; }
	fWaitingTool = false;
	fDeadline = system_time() + 180000000;
	fInnerID = system_time();
	BMessage request(kGenerate);
	request.AddInt64("id", fInnerID);
	request.AddMessenger("target", BMessenger(this));
	request.AddString("prompt", fPrompt.c_str());
	if (fModel.SendMessage(&request, this, 100000) != B_OK) Fail("Model service is unavailable.");
}

void ToolSession::MessageReceived(BMessage* message)
{
	if (message->what == kWatchSession) {
		if (fBusy && (system_time() > fDeadline || !fClient.IsValid()
			|| !(fWaitingTool ? fTools : fModel).IsValid())) Stop();
		return;
	}
	if (message->what == kCancel) { Stop(); return; }
	if (message->what == kGenerate) {
		if (fBusy) return;
		const char* prompt;
		if (message->FindMessenger("target", &fClient) != B_OK
			|| message->FindInt64("id", &fOuterID) != B_OK
			|| message->FindString("prompt", &prompt) != B_OK) return;
		fPrompt = prompt; fVisible.clear(); fToolCount = 0;
		if (fWatch->InitCheck() != B_OK) {
			Fail("Cannot start tool-session watchdog. No request executed."); return;
		}
		fModel = BMessenger(kServiceSignature);
		fBusy = true;
		SendModel();
		return;
	}
	if (message->what != kUpdate && message->what != kDone && message->what != kToolResult) {
		BLooper::MessageReceived(message); return;
	}
	int64 id;
	if (!fBusy || message->FindInt64("id", &id) != B_OK || id != fInnerID) return;
	if (message->what == kToolResult) {
		if (!fWaitingTool) return;
		const char* result;
		int32 status;
		if (message->FindInt32("status", &status) != B_OK || message->FindString("text", &result) != B_OK) {
			Fail("Malformed tools-service response."); return;
		}
		std::string data(result);
		if (data.size() > 1200) data = data.substr(0, 1200) + "\n[Tool result truncated]";
		fVisible += data + "\n\n";
		Publish(kUpdate, fVisible, "Tool returned. Preparing answer...");
		if (status != B_OK) { Fail("Tool denied, expired, cancelled, or failed. No automatic retry."); return; }
		fPrompt += fCall + "<|im_end|>\n<|im_start|>user\n<tool_response>\n"
			+ EscapeToolData(data) + "\n</tool_response><|im_end|>\n"
			"<|im_start|>assistant\n<think>\n\n</think>\n\n";
		SendModel();
		return;
	}
	if (fWaitingTool) return;
	const char* text = "";
	const char* status = "Generating...";
	message->FindString("text", &text);
	message->FindString("status", &status);
	if (message->what == kUpdate) {
		fDeadline = system_time() + 180000000;
		std::string partial(text);
		Publish(kUpdate, fVisible + (partial.find("<tool_call>") != std::string::npos
			? "[Preparing a tool request...]" : partial), status);
		return;
	}
	bool success = false;
	message->FindBool("success", &success);
	if (!success) { Fail(status); return; }
	BMessage call;
	ToolParseResult parsed = ParseAssistantToolCall(text, call);
	if (parsed == kNotToolCall) {
		fBusy = false;
		Publish(kDone, fVisible + text, fToolCount > 0
			? "Ready. Tool results shown above; model summary may contain errors."
			: "Ready. No system tools used; machine-state claims are unverified.", true);
		return;
	}
	if (parsed != kValidToolCall) { Fail("Invalid model tool request blocked; no action executed."); return; }
	if (++fToolCount > 3) { Fail("Three-tool limit reached. Ask a follow-up to continue."); return; }
	fTools = BMessenger(kToolsSignature);
	if (!fTools.IsValid()) {
		entry_ref ref;
		status_t launched = get_ref_for_path("/boot/home/config/non-packaged/servers/HaikuAssistantTools", &ref);
		if (launched == B_OK) launched = be_roster->Launch(&ref);
		if (launched != B_OK && launched != B_ALREADY_RUNNING) { Fail("Could not launch tools service."); return; }
		fTools = BMessenger(kToolsSignature);
	}
	fInnerID = system_time();
	call.AddInt64("id", fInnerID);
	fCall = text;
	const char* name = "";
	call.FindString("tool", &name);
	fVisible += std::string("[Tool: ") + name + "]\n";
	fWaitingTool = true;
	fDeadline = system_time() + 70000000;
	Publish(kUpdate, fVisible, "Tool request sent. Desktop actions need your approval.");
	if (fTools.SendMessage(&call, this, 100000) != B_OK) Fail("Tools service is unavailable.");
}
