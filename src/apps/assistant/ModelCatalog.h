/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_MODEL_CATALOG_H
#define HAIKU_ASSISTANT_MODEL_CATALOG_H

#include <stddef.h>
#include <stdint.h>

struct AssistantModel {
	const char* name;
	const char* repository;
	const char* revision;
	const char* filename;
	uint64_t bytes;
	const char* sha256;
	bool compatible;
};

// Data only: no downloaded code, model scripts or remote chat templates.
// Revisions and LFS hashes from the publishers' Hub API, 2026-10-15.
static const AssistantModel kAssistantModels[] = {
	{"Qwen3 0.6B — Q8_0", "Qwen/Qwen3-0.6B-GGUF",
		"23749fefcc72300e3a2ad315e1317431b06b590a", "Qwen3-0.6B-Q8_0.gguf",
		639446688ULL, "9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031", true},
	{"Qwen3 1.7B — Q8_0", "Qwen/Qwen3-1.7B-GGUF",
		"90862c4b9d2787eaed51d12237eafdfe7c5f6077", "Qwen3-1.7B-Q8_0.gguf",
		1834426016ULL, "061b54daade076b5d3362dac252678d17da8c68f07560be70818cace6590cb1a", true},
	{"Qwen3 4B — Q4_K_M", "Qwen/Qwen3-4B-GGUF",
		"bc640142c66e1fdd12af0bd68f40445458f3869b", "Qwen3-4B-Q4_K_M.gguf",
		2497280256ULL, "7485fe6f11af29433bc51cab58009521f205840f5b4ae3a32fa7f92e8534fdf5", true},
	{"Qwen3.8 27B — Q4_K_M (not supported yet)", "ggml-org/Qwen3.8-27B-GGUF",
		"71bc7b627595dc8a91039addd9c791ae548d6747", "Qwen3.8-27B-Q4_K_M.gguf",
		18973870528ULL, "c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747", false}
};
static const size_t kAssistantModelCount = sizeof(kAssistantModels) / sizeof(kAssistantModels[0]);

#endif
