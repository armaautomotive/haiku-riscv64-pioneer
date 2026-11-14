/* SPDX-License-Identifier: MIT */
#include "../../../src/apps/assistant/P100AQwen27FullOffload.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
using C = AssistantP100AQwen27FullOffloadSession;
int main()
{
	const char* path = getenv("ASK_TEST_TRACE"); assert(path);
	assert(setenv("ASK_TEST_CASE", "35", 1) == 0);
	C session; std::string error; unsigned loads = 0;
	assert(!session.Load({}, error) && !session.Loaded());
	assert(session.Load([&](float, const char*) {}, error));
	assert(session.Loaded() && !session.Faulted());
	for (unsigned i = 0; i < 2; i++) {
		auto reply = session.Complete(i ? "second" : "first", 2, [] { return false; },
			[&](float value, const char*) { assert(value == -1 || (value >= 0 && value <= 1)); });
		assert(reply.outcome == AssistantP100AQwen27FullOffload::Completed);
		assert(!reply.cleanWorkerExit && reply.outputTokens == 1 && reply.answer == (i ? "two" : "one"));
		assert(session.Loaded() && !session.Faulted()); loads++;
	}
	assert(loads == 2);
	assert(session.Unload(error) && !session.Loaded() && !session.Faulted());
	puts("PASS_FAKE_RESIDENT_CLIENT_TWO_REQUESTS_ONE_WORKER_ONE_MODEL_END;NO_DEVICE_OR_PERFORMANCE_RESULT");
}
