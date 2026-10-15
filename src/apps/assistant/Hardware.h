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
		"Experimental TinyStories-260K story completion only — NOT Qwen chat.\n"
		"Selecting P100A also selects TinyStories (install it in Models first).\n"
		"Real card projections; CPU attention/KV/norms and independent verification.\n"
		"Short ASCII story beginnings; 8 output tokens; roughly 15 seconds/token.\n"
		"Stop waits for a completed batch. No CPU fallback or firmware changes."}
};
static const size_t kAssistantHardwareCount = sizeof(kAssistantHardware) / sizeof(kAssistantHardware[0]);

static inline int AssistantHardwareIndex(const char* id)
{
	if (id != NULL)
		for (size_t i = 0; i < kAssistantHardwareCount; i++)
			if (strcmp(id, kAssistantHardware[i].id) == 0) return (int)i;
	return -1;
}

// Recognized execution implementations. Service capability messages additionally
// require a matching driver ABI, model bundle and a non-faulted runtime.
static inline bool AssistantHardwareCanChat(const char* id)
{
	return AssistantHardwareIndex(id) >= 0;
}

#endif
