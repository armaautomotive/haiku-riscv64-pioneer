/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_SETTINGS_H
#define HAIKU_ASSISTANT_SETTINGS_H
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <String.h>
#include <OS.h>
#include <stdio.h>

static inline int32 AssistantMaxThreads()
{
	system_info info;
	return get_system_info(&info) == B_OK && info.cpu_count > 0
		? (int32)info.cpu_count : 1;
}
static inline status_t AssistantSettingsPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	return status == B_OK ? path.Append("HaikuAssistant_settings") : status;
}
static inline int32 LoadAssistantThreads()
{
	int32 maximum = AssistantMaxThreads();
	int32 threads = maximum < 32 ? maximum : 32;
	BPath path;
	if (AssistantSettingsPath(path) != B_OK) return threads;
	BFile file(path.Path(), B_READ_ONLY);
	BMessage settings;
	int32 saved;
	if (file.InitCheck() == B_OK && settings.Unflatten(&file) == B_OK
		&& settings.FindInt32("cpu_threads", &saved) == B_OK && saved > 0)
		threads = saved > maximum ? maximum : saved;
	return threads;
}
static inline void LoadAssistantSettings(BMessage& settings)
{
	BPath path;
	if (AssistantSettingsPath(path) != B_OK) return;
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() == B_OK && settings.Unflatten(&file) != B_OK) settings.MakeEmpty();
}
// Only the service writes settings. Its looper/worker serialize updates with
// the service lock, preserving fields owned by other settings controls.
static inline status_t SaveAssistantSettings(const BMessage& settings)
{
	BPath path;
	status_t status = AssistantSettingsPath(path);
	if (status != B_OK) return status;
	BString temporary(path.Path()); temporary << ".new";
	BFile file(temporary.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK) return file.InitCheck();
	status = settings.Flatten(&file);
	if (status == B_OK) status = file.Sync();
	file.Unset();
	if (status == B_OK && rename(temporary.String(), path.Path()) != 0) status = B_IO_ERROR;
	return status;
}
static inline status_t SaveAssistantThreads(int32 threads)
{
	BMessage settings;
	LoadAssistantSettings(settings);
	settings.RemoveName("cpu_threads");
	settings.AddInt32("cpu_threads", threads);
	return SaveAssistantSettings(settings);
}
static inline BString LoadAssistantModel()
{
	BMessage settings;
	LoadAssistantSettings(settings);
	const char* path;
	return settings.FindString("model_path", &path) == B_OK ? BString(path) : BString();
}
static inline status_t SaveAssistantModel(const char* path)
{
	BMessage settings;
	LoadAssistantSettings(settings);
	settings.RemoveName("model_path");
	settings.AddString("model_path", path);
	return SaveAssistantSettings(settings);
}
#endif
