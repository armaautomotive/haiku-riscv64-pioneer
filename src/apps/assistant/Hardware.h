/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_HARDWARE_H
#define HAIKU_ASSISTANT_HARDWARE_H

#include <stddef.h>
#include <string.h>

struct AssistantHardware {
	const char* id;
	const char* name;
	const char* description;
};

static const AssistantHardware kAssistantHardware[] = {
	{"cpu", "CPU (llama.cpp)",
		"Runs supported Qwen3 chat models on the system CPUs.\n"
		"CPU threads apply to the next request; they do not change CPU affinity."},
	{"p100a", "Tenstorrent P100A",
		"Unavailable for Assistant chat: the general P100A inference backend is unfinished.\n"
		"The experimental TinyStories CPU/P100A hybrid is a separate diagnostic,\n"
		"not a backend for the installed Qwen models. Card presence alone is not enough.\n"
		"Choosing hardware never downloads firmware or changes PCI/power settings."}
};
static const size_t kAssistantHardwareCount = sizeof(kAssistantHardware) / sizeof(kAssistantHardware[0]);

static inline int AssistantHardwareIndex(const char* id)
{
	if (id != NULL)
		for (size_t i = 0; i < kAssistantHardwareCount; i++)
			if (strcmp(id, kAssistantHardware[i].id) == 0) return (int)i;
	return -1;
}

// Actual execution capability, not PCI device detection. Enable a future
// backend only together with its model compatibility checks and executor.
static inline bool AssistantHardwareCanChat(const char* id)
{
	return id != NULL && strcmp(id, "cpu") == 0;
}

#endif
