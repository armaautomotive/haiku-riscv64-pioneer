/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "ToolsProtocol.h"
#include <Alert.h>
#include <Application.h>
#include <Entry.h>
#include <Invoker.h>
#include <List.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <Window.h>
#include <MessengerPrivate.h>
#include <WindowInfo.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

static const uint32 kDecision = 'tdec', kExpired = 'texp';

static void
Respond(BMessage* request, status_t status, const char* text, BMessage* data = NULL)
{
	BMessage result(kToolResult);
	int64 id;
	if (request->FindInt64("id", &id) == B_OK) result.AddInt64("id", id);
	result.AddInt32("status", status);
	result.AddString("text", text);
	if (data != NULL) result.AddMessage("data", data);
	request->SendReply(&result, (BHandler*)NULL, 1000000);
}

static const char*
AppSignature(const char* name)
{
	if (strcmp(name, "Terminal") == 0) return "application/x-vnd.Haiku-Terminal";
	if (strcmp(name, "StyledEdit") == 0) return "application/x-vnd.Haiku-StyledEdit";
	if (strcmp(name, "DeskCalc") == 0) return "application/x-vnd.Haiku-DeskCalc";
	if (strcmp(name, "WebPositive") == 0) return "application/x-vnd.Haiku-WebPositive";
	return NULL;
}

class ToolsService : public BApplication {
public:
	ToolsService() : BApplication(kToolsSignature), fPending(NULL), fTimer(NULL),
		fNonce(0), fToken(-1), fTeam(-1), fPort(-1), fHandler(-1) {}
	virtual bool QuitRequested()
	{
		Finish(B_CANCELED, "Tools service stopped.");
		return true;
	}
	virtual void MessageReceived(BMessage* request)
	{
		if (request->what == kToolCancel) {
			int64 id, pendingID;
			if (fPending != NULL && request->FindInt64("id", &id) == B_OK
				&& fPending->FindInt64("id", &pendingID) == B_OK && id == pendingID
				&& request->ReturnAddress() == fPending->ReturnAddress())
				Finish(B_CANCELED, "Tool request cancelled; no action executed.");
			return;
		}
		if (request->what == kDecision || request->what == kExpired) {
			uint64 nonce;
			if (fPending == NULL || request->FindUInt64("nonce", &nonce) != B_OK
				|| nonce != fNonce) return;
			int32 index = 0;
			request->FindInt32("which", &index);
			if (request->what == kExpired || index != 1) {
				Finish(B_CANCELED, "Action denied or approval expired; nothing changed.");
				return;
			}
			Execute();
			return;
		}
		if (request->what != kToolCall) {
			BApplication::MessageReceived(request);
			return;
		}
		const char* tool;
		if (request->FlattenedSize() > 8192 || request->FindString("tool", &tool) != B_OK) {
			Respond(request, B_BAD_VALUE, "Invalid tool request."); return;
		}
		if (strcmp(tool, "list_tools") == 0) {
			Respond(request, B_OK, "Read-only: system_info, list_apps, list_windows.\n"
				"Approval required: move_window, resize_window, focus_window, launch_app.\n"
				"Launch allowlist: Terminal, StyledEdit, DeskCalc, WebPositive. No arguments or shell.");
		} else if (strcmp(tool, "system_info") == 0) {
			SystemInfo(request);
		} else if (strcmp(tool, "list_apps") == 0 || strcmp(tool, "list_windows") == 0) {
			List(request, strcmp(tool, "list_windows") == 0);
		} else if (strcmp(tool, "move_window") == 0 || strcmp(tool, "resize_window") == 0
			|| strcmp(tool, "focus_window") == 0 || strcmp(tool, "launch_app") == 0) {
			Prepare(request, tool);
		} else Respond(request, B_BAD_VALUE, "Unknown tool; nothing executed.");
	}

private:
	void SystemInfo(BMessage* request)
	{
		system_info info;
		status_t status = get_system_info(&info);
		if (status != B_OK) { Respond(request, status, "System query failed."); return; }
		utsname os;
		memset(&os, 0, sizeof(os));
		uname(&os);
		uint64 total = info.max_pages * (uint64)B_PAGE_SIZE;
		uint64 used = info.used_pages * (uint64)B_PAGE_SIZE;
		BMessage data;
		data.AddInt32("cpu_count", info.cpu_count);
		data.AddUInt64("memory_total_bytes", total);
		data.AddUInt64("memory_used_bytes", used);
		data.AddUInt64("memory_free_bytes", total >= used ? total - used : 0);
		data.AddInt64("uptime_us", system_time());
		data.AddString("os", os.sysname);
		data.AddString("release", os.release);
		data.AddString("architecture", os.machine);
		BString text;
		text.SetToFormat("%s %s (%s)\nCPUs: %lu\nMemory: %.2f GiB total (%llu bytes), %llu bytes used\n"
			"Uptime: %.1f seconds", os.sysname, os.release, os.machine,
			(unsigned long)info.cpu_count, total / 1073741824.0, (unsigned long long)total,
			(unsigned long long)used, system_time() / 1000000.0);
		Respond(request, B_OK, text.String(), &data);
	}

	void List(BMessage* request, bool windows)
	{
		BList teams;
		be_roster->GetAppList(&teams);
		BMessage data;
		BString text;
		int count = 0;
		for (int i = 0; i < teams.CountItems() && count < 64; i++) {
			team_id team = (team_id)(addr_t)teams.ItemAt(i);
			app_info app;
			if (be_roster->GetRunningAppInfo(team, &app) != B_OK) continue;
			if (!windows) {
				BMessage item;
				item.AddInt32("team", team);
				item.AddString("signature", app.signature);
				data.AddMessage("items", &item);
				text << team << " " << app.signature << "\n";
				count++;
				continue;
			}
			int32 size = 0;
			int32* tokens = get_token_list(team, &size);
			for (int j = 0; tokens != NULL && j < size && count < 64; j++) {
				client_window_info* info = get_window_info(tokens[j]);
				if (info == NULL) continue;
				if (info->feel == B_NORMAL_WINDOW_FEEL && info->show_hide_level == 0) {
					BString title(info->name);
					title.Truncate(256);
					BMessage item;
					item.AddInt32("team", team);
					item.AddInt32("token", tokens[j]);
					item.AddString("title", title.String());
					item.AddRect("frame", BRect(info->window_left, info->window_top,
						info->window_right, info->window_bottom));
					data.AddMessage("items", &item);
					text << "team=" << team << " token=" << tokens[j] << " " << title
						<< " [" << info->window_left << "," << info->window_top << ","
						<< info->window_right << "," << info->window_bottom << "]\n";
					count++;
				}
				free(info);
			}
			free(tokens);
		}
		data.AddBool("possibly_truncated", count == 64);
		Respond(request, B_OK, text.String(), &data);
	}

	void Prepare(BMessage* request, const char* tool)
	{
		if (fPending != NULL) { Respond(request, B_BUSY, "Another action awaits approval."); return; }
		fAction = tool;
		BString description;
		if (fAction == "launch_app") {
			const char* name;
			if (request->FindString("application", &name) != B_OK || AppSignature(name) == NULL) {
				Respond(request, B_NOT_ALLOWED, "Application is not in the launch allowlist."); return;
			}
			fApp = name;
			if (be_roster->FindApp(AppSignature(name), &fAppRef) != B_OK) {
				Respond(request, B_ENTRY_NOT_FOUND, "Application is not installed."); return;
			}
			description << "Launch " << name << " with no command-line arguments?";
		} else {
			if (request->FindInt32("team", &fTeam) != B_OK
				|| request->FindInt32("token", &fToken) != B_OK) {
				Respond(request, B_BAD_VALUE, "A team and window token are required."); return;
			}
			client_window_info* info = get_window_info(fToken);
			if (info == NULL || info->team != fTeam || info->feel != B_NORMAL_WINDOW_FEEL
				|| info->show_hide_level != 0 || fTeam == Team()
				|| (fAction == "move_window" && (info->flags & B_NOT_MOVABLE) != 0)
				|| (fAction == "resize_window" && (info->flags & B_NOT_RESIZABLE) != 0)) {
				free(info); Respond(request, B_BAD_VALUE, "Target is not a visible normal window."); return;
			}
			fPort = info->client_port;
			fHandler = info->client_token;
			fTitle = info->name;
			BRect frame(info->window_left, info->window_top, info->window_right, info->window_bottom);
			fOriginalFrame = frame;
			free(info);
			float a, b;
			if (fAction != "focus_window") {
				if (request->FindFloat("a", &a) != B_OK || request->FindFloat("b", &b) != B_OK
					|| !isfinite(a) || !isfinite(b)) {
					Respond(request, B_BAD_VALUE, "Finite coordinates or dimensions are required."); return;
				}
				if (fAction == "move_window") frame.OffsetTo(a, b);
				else { frame.right = frame.left + a; frame.bottom = frame.top + b; }
				BRect screen = BScreen().Frame();
				screen.InsetBy(8, 28);
				if (frame.Width() < 64 || frame.Height() < 48 || !screen.Contains(frame)) {
					Respond(request, B_BAD_VALUE, "Requested frame must fit on-screen with a title-bar margin."); return;
				}
			}
			fFrame = frame;
			description << fAction << "\nWindow: " << fTitle << "\nTeam: " << fTeam
				<< "  Token: " << fToken;
			if (fAction != "focus_window")
				description << "\nFrame: " << frame.left << ", " << frame.top << " to "
					<< frame.right << ", " << frame.bottom;
		}
		description << "\n\nRequesting local team: " << request->ReturnAddress().Team()
			<< "\nPermission applies to this action only; expires in 60 seconds.";
		int random = open("/dev/urandom", O_RDONLY);
		ssize_t bytes = random >= 0 ? read(random, &fNonce, sizeof(fNonce)) : -1;
		if (random >= 0) close(random);
		if (bytes != sizeof(fNonce)) {
			Respond(request, B_ERROR, "Cannot secure the approval response; action refused."); return;
		}
		// A copy drops Haiku's synchronous reply metadata. Retain the original
		// until consent/timeout, then reply and delete it exactly once.
		fPending = DetachCurrentMessage();
		if (fPending == NULL) {
			Respond(request, B_ERROR, "Cannot retain approval request; action refused."); return;
		}
		BMessage expired(kExpired);
		expired.AddUInt64("nonce", fNonce);
		fTimer = new BMessageRunner(BMessenger(this), &expired, 60000000, 1);
		if (fTimer->InitCheck() != B_OK) {
			Finish(B_ERROR, "Cannot start approval timeout; action refused."); return;
		}
		BMessage* decision = new BMessage(kDecision);
		decision->AddUInt64("nonce", fNonce);
		BAlert* alert = new BAlert("Assistant action approval", description.String(),
			"Deny", "Allow once", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->SetDefaultButton(alert->ButtonAt(0));
		fDialog = BMessenger(alert);
		if (alert->Go(new BInvoker(decision, this)) != B_OK)
			Finish(B_ERROR, "Cannot display approval dialog; action refused.");
	}

	void Execute()
	{
		status_t result;
		if (fAction == "launch_app") {
			entry_ref current;
			if (be_roster->FindApp(AppSignature(fApp.String()), &current) != B_OK
				|| current != fAppRef) { Finish(B_ERROR, "Application changed since approval; retry."); return; }
			result = be_roster->Launch(&fAppRef);
			if (result == B_ALREADY_RUNNING) result = B_OK;
		} else {
			client_window_info* info = get_window_info(fToken);
			bool valid = info != NULL && info->team == fTeam && info->client_port == fPort
				&& info->client_token == fHandler && fTitle == info->name
				&& info->feel == B_NORMAL_WINDOW_FEEL && info->show_hide_level == 0
				&& fOriginalFrame == BRect(info->window_left, info->window_top,
					info->window_right, info->window_bottom);
			free(info);
			if (!valid) { Finish(B_BAD_VALUE, "Window changed or closed; action refused."); return; }
			BRect screen = BScreen().Frame();
			screen.InsetBy(8, 28);
			if (fAction != "focus_window" && !screen.Contains(fFrame)) {
				Finish(B_BAD_VALUE, "Screen changed; requested frame is no longer safe."); return;
			}
			BMessenger target;
			BMessenger::Private(target).SetTo(fTeam, fPort, fHandler);
			BMessage action(B_SET_PROPERTY), reply;
			if (fAction == "focus_window") {
				action.AddSpecifier("Active"); action.AddBool("data", true);
			} else { action.AddSpecifier("Frame"); action.AddRect("data", fFrame); }
			result = target.SendMessage(&action, &reply, 1000000, 2000000);
			if (result == B_OK && reply.FindInt32("error", &result) != B_OK) result = B_ERROR;
		}
		Finish(result, result == B_OK ? "Approved action completed." : "Action failed or target did not respond.");
	}

	void Finish(status_t status, const char* text)
	{
		delete fTimer; fTimer = NULL;
		if (fDialog.IsValid()) {
			BMessage close(B_QUIT_REQUESTED);
			fDialog.SendMessage(&close, (BHandler*)NULL, 100000);
		}
		fDialog = BMessenger();
		if (fPending != NULL) {
			fprintf(stderr, "Assistant tools: action=%s status=%ld\n", fAction.String(), (long)status);
			Respond(fPending, status, text); delete fPending; fPending = NULL;
		}
		fNonce = 0;
	}
	BMessage* fPending;
	BMessageRunner* fTimer;
	BMessenger fDialog;
	uint64 fNonce;
	BString fAction, fApp, fTitle;
	entry_ref fAppRef;
	int32 fToken, fTeam, fPort, fHandler;
	BRect fFrame, fOriginalFrame;
};

int main()
{
	ToolsService service;
	service.Run();
	return 0;
}
