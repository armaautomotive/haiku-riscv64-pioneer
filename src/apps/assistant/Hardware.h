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
	{"p100a", "P100A — TinyStories (legacy experiment)",
		"Experimental TinyStories-260K story completion only — NOT Qwen chat.\n"
		"Selecting P100A also selects TinyStories (install it in Models first).\n"
		"Real card projections; CPU attention/KV/norms and independent verification.\n"
		"Short ASCII story beginnings; 8 output tokens; roughly 15 seconds/token.\n"
		"Stop waits for a completed batch. No CPU fallback or firmware changes."},
	{"p100a-qwen", "P100A — Qwen3 0.6B (experimental, slow)",
		"Runs the verified Qwen3-0.6B Q8_0 model on two P100A cores.\n"
		"Currently SLOWER than CPU: about 0.15 output tokens/second.\n"
		"First request uploads weights (several minutes); later requests reuse them.\n"
		"Short questions only: 512 UTF-8 bytes / 192 prompt tokens, 8 output tokens.\n"
		"CPU attention + independent validation; no tools/history or CPU fallback.\n"
		"Stop waits for the current checked transfer/matvec. Other models unsupported."},
	{"p100a-qwen-fast", "P100A Fast — Qwen3 0.6B (no CPU validation)",
		"Experimental Qwen3-0.6B Q8_0 on the same two P100A cores.\n"
		"Skips the independent CPU model run and logit/token comparison.\n"
		"CPU attention/KV/norms and activation preprocessing still run.\n"
		"Device completion/stop, coverage, finite-output and recovery checks remain.\n"
		"Same short-question/eight-token limits; no history, tools or CPU fallback.\n"
		"Fast means validation off, NOT a faster-than-CPU guarantee.\n"
		"First upload takes several minutes; checked/Fast switching keeps warm weights."},
	{"p100a-qwen-attention", "P100A Attention — Qwen3 0.6B (experimental)",
		"Qwen3-0.6B Q8_0 matrices AND attention execute on the P100A.\n"
		"QK/PV use Tensix; softmax runs on the card's control core.\n"
		"Host still stores KV and handles norms, embeddings and preprocessing.\n"
		"No independent CPU validation or CPU attention fallback.\n"
		"PIO-staged KV prototype: NOT a speedup; may be slower than Fast.\n"
		"Same short-question/eight-token limits, no history/tools.\n"
		"Switching attention modes reloads device weights; upload can take 15+ minutes."}
};
static const size_t kAssistantHardwareCount = sizeof(kAssistantHardware) / sizeof(kAssistantHardware[0]);

static inline int AssistantHardwareIndex(const char* id)
{
	if (id != NULL)
		for (size_t i = 0; i < kAssistantHardwareCount; i++)
			if (strcmp(id, kAssistantHardware[i].id) == 0) return (int)i;
	return -1;
}

static inline bool AssistantHardwareIsP100AQwen(const char* id)
{
	return id != NULL && (!strcmp(id,"p100a-qwen") || !strcmp(id,"p100a-qwen-fast") || !strcmp(id,"p100a-qwen-attention"));
}

static inline bool AssistantHardwareIsP100AFast(const char* id)
{
	return id != NULL && !strcmp(id,"p100a-qwen-fast");
}

static inline bool AssistantHardwareIsP100AAttention(const char* id)
{
	return id != NULL && !strcmp(id,"p100a-qwen-attention");
}

static inline bool AssistantHardwareSkipsValidation(const char* id)
{
	return AssistantHardwareIsP100AFast(id) || AssistantHardwareIsP100AAttention(id);
}

static inline const char* AssistantHardwareQwenFormat(const char* id)
{
	return AssistantHardwareIsP100AAttention(id)?"qwen-p100a-attention"
		:AssistantHardwareIsP100AFast(id)?"qwen-p100a-fast"
		:(id && !strcmp(id,"p100a-qwen")?"qwen-p100a":NULL);
}

// Recognized execution implementations. Service capability messages additionally
// require a matching driver ABI, model bundle and a non-faulted runtime.
static inline bool AssistantHardwareCanChat(const char* id)
{
	return AssistantHardwareIndex(id) >= 0;
}

#endif
