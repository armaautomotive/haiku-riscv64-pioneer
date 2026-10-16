/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_SERVICE_LAUNCHER_H
#define HAIKU_ASSISTANT_SERVICE_LAUNCHER_H

#include "Protocol.h"
#include <Entry.h>
#include <Messenger.h>
#include <Roster.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

static inline status_t StartAssistantService()
{
	if (BMessenger(kServiceSignature).IsValid()) return B_OK;
	const char* runtime = "/boot/home/develop/llama-c060ca974c77/build-pioneer/bin";
	if (access(runtime, R_OK) != 0)
		runtime = "/Haiku1/home/develop/llama-c060ca974c77/build-pioneer/bin";
	std::string libraries = std::string(runtime) + ":/boot/system/lib";
	setenv("LIBRARY_PATH", libraries.c_str(), 1);
	entry_ref ref;
	status_t result = get_ref_for_path(
		"/boot/home/config/non-packaged/servers/HaikuAssistantService", &ref);
	if (result == B_OK) result = be_roster->Launch(&ref);
	return result == B_ALREADY_RUNNING ? B_OK : result;
}

#endif
