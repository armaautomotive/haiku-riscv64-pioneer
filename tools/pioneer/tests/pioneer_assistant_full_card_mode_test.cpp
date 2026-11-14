/* SPDX-License-Identifier: MIT */
#include "../../../src/apps/assistant/Hardware.h"
#include "../../../src/apps/assistant/ModelCatalog.h"
#include <cassert>
#include <cstring>

int main()
{
	const char* full = "p100a-qwen27-full-resident";
	const char* work = "p100a-qwen27-work";
	assert(kAssistantHardwareCount == 8);
	assert(AssistantHardwareIndex(full) == 7 && AssistantHardwareCanChat(full));
	assert(AssistantHardwareIsP100AQwen27Full(full));
	assert(!AssistantHardwareIsP100AQwen27(full));
	assert(AssistantHardwareIsP100AQwen27(work) && !AssistantHardwareIsP100AQwen27Full(work));
	assert(AssistantHardwareSkipsValidation(full));
	assert(std::strcmp(AssistantHardwareQwenFormat(full), "qwen27-p100a-full-resident") == 0);
	assert(AssistantModelSupportsBackend(kAssistantQwen27Model, full));
	for (size_t i = 0; i < kAssistantModelCount; i++)
		assert(AssistantModelSupportsBackend(i, full) == (i == kAssistantQwen27Model));
	assert(!AssistantModelSupportsBackend(kAssistantQwen27Model, "cpu"));
	return 0;
}
