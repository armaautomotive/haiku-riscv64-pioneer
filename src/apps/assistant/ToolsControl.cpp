/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "ToolsProtocol.h"
#include <Application.h>
#include <Entry.h>
#include <Messenger.h>
#include <Roster.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool Integer(const char* text, int32& value)
{
	char* end;
	errno = 0;
	long parsed = strtol(text, &end, 10);
	if (errno != 0 || end == text || *end != 0 || parsed < 0 || parsed > INT32_MAX) return false;
	value = parsed; return true;
}
static bool Number(const char* text, float& value)
{
	char* end;
	errno = 0;
	value = strtof(text, &end);
	return errno == 0 && end != text && *end == 0 && isfinite(value);
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: haiku-tools list_tools | system_info | list_apps | list_windows\n"
			"       haiku-tools launch_app Terminal|StyledEdit|DeskCalc|WebPositive\n"
			"       haiku-tools focus_window TEAM TOKEN\n"
			"       haiku-tools move_window TEAM TOKEN X Y\n"
			"       haiku-tools resize_window TEAM TOKEN WIDTH HEIGHT\n");
		return 2;
	}
	BMessage request(kToolCall);
	request.AddString("tool", argv[1]);
	if (strcmp(argv[1], "launch_app") == 0) {
		if (argc != 3) return 2;
		request.AddString("application", argv[2]);
	} else if (strcmp(argv[1], "focus_window") == 0 || strcmp(argv[1], "move_window") == 0
		|| strcmp(argv[1], "resize_window") == 0) {
		bool focus = strcmp(argv[1], "focus_window") == 0;
		int32 team, token;
		if (argc != (focus ? 4 : 6) || !Integer(argv[2], team) || !Integer(argv[3], token)) return 2;
		request.AddInt32("team", team);
		request.AddInt32("token", token);
		if (!focus) {
			float a, b;
			if (!Number(argv[4], a) || !Number(argv[5], b)) return 2;
			request.AddFloat("a", a); request.AddFloat("b", b);
		}
	} else if (argc != 2) return 2;
	BApplication app("application/x-vnd.Haiku-AssistantToolsControl");
	BMessenger service(kToolsSignature);
	if (!service.IsValid()) {
		entry_ref ref;
		status_t status = get_ref_for_path(
			"/boot/home/config/non-packaged/servers/HaikuAssistantTools", &ref);
		if (status == B_OK) status = be_roster->Launch(&ref);
		if (status != B_OK && status != B_ALREADY_RUNNING) {
			fprintf(stderr, "Cannot start tools service: %s\n", strerror(status)); return 1;
		}
		service = BMessenger(kToolsSignature);
	}
	if (strcmp(argv[1], "--stop-service") == 0)
		return service.SendMessage(B_QUIT_REQUESTED) == B_OK ? 0 : 1;
	BMessage reply;
	status_t status = service.SendMessage(&request, &reply, 1000000, 65000000);
	if (status != B_OK) { fprintf(stderr, "Tools service: %s\n", strerror(status)); return 1; }
	const char* text = "Invalid service response.";
	reply.FindString("text", &text);
	printf("%s\n", text);
	if (reply.what != kToolResult || reply.FindInt32("status", &status) != B_OK) return 1;
	return status == B_OK ? 0 : 1;
}
