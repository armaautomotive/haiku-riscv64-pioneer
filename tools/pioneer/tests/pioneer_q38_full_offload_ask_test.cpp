/* SPDX-License-Identifier: MIT */
#include "../../../src/apps/assistant/P100AQwen27FullOffload.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
using C = AssistantP100AQwen27FullOffload;
int main()
{
	const char* value = getenv("ASK_TEST_CASE"); assert(value);
	const int mode = atoi(value);
	bool cancel = mode == 33;
	bool loaded = false;
	auto reply = C::Run(mode == 32 ? "" : "What is 2 + 2?", 4,
		[&] { return cancel || (mode == 34 && loaded); }, [&](float part, const char* text) {
			assert(part == -1 || (part >= 0 && part <= 1));
			if (std::string(text) == "ASK_TEST_CANCEL_POINT") cancel = true;
			if (std::string(text) == "ASK_TEST_PRE_BEGIN") loaded = true;
		});
	const bool good = mode == 0 || mode == 1;
	const bool rejected = mode == 2 || mode == 4 || mode == 32;
	const bool cancelled = mode == 3 || mode == 30 || mode == 31 || mode == 33 || mode == 34;
	assert(reply.outcome == (good ? C::Completed : rejected ? C::Rejected : cancelled ? C::Cancelled : C::Unknown));
	const bool closed = good || mode == 2 || mode == 3 || mode == 30 || mode == 31 || mode == 34;
	assert(reply.cleanWorkerExit == closed);
	assert(reply.answer == (mode == 0 ? "4" : ""));
	assert(reply.outputTokens == (mode == 0 ? 1u : 0u));
	assert(reply.error.empty() == good);
	puts("PASS_FAKE_FULL_CARD_PROMPT_CLIENT_NO_HARDWARE");
}
