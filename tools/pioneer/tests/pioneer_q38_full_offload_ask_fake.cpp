/* SPDX-License-Identifier: MIT */
// FAKE channel peer only. It performs no model/device work and cannot certify END.
#include "../../../src/apps/assistant/Qwen27Protocol.h"
#include <cstdio>
#include <cstdlib>
#include <poll.h>
namespace A = Assistant27;
static FILE* trace = nullptr;
static bool Read(A::Frame& frame)
{
	A::Receiver receiver;
	for (;;) {
		const auto result = receiver.Read(0, frame);
		if (result == A::Receiver::Received) {
			fprintf(trace, "read %u %u %u\n", frame.kind, frame.id, frame.value); fflush(trace); return true;
		}
		if (result != A::Receiver::Waiting) { fputs("channel_closed\n", trace); fflush(trace); return false; }
		pollfd item = {0, POLLIN, 0};
		if (poll(&item, 1, 1000) < 0 && errno != EINTR) return false;
	}
}
static bool Reply(unsigned kind, unsigned id, unsigned value = 0, const char* text = "")
{
	A::Frame frame; frame.kind = kind; frame.id = id; frame.value = value; frame.text = text;
	return A::Send(1, frame);
}
int main(int argc, char** argv)
{
	if (argc != 2 || strcmp(argv[1], "--assistant-channel")) return 2;
	const char* value = getenv("ASK_TEST_CASE");
	const char* path = getenv("ASK_TEST_TRACE");
	if (!value || !path || !(trace = fopen(path, "w"))) return 2;
	setvbuf(trace, nullptr, _IOLBF, 0);
	const int mode = atoi(value);
	A::Frame frame;
	if (!Read(frame) || frame.kind != A::Load || frame.id != 1 || frame.value != 1 || !frame.text.empty()) return 2;
	if (mode == 4) { Reply(A::Error, 1, A::Rejected, "Synthetic unavailable full-card driver"); return 1; }
	if (mode == 7) { Reply(A::Progress, 1, 1001, "bad value"); Read(frame); return 0; }
	if (mode == 34 && !Reply(A::Progress, 1, UINT32_MAX, "ASK_TEST_PRE_BEGIN")) return 0;
	if (!Reply(A::Ready, mode == 6 ? 2 : 1, mode == 26 ? 1 : 0,
		mode == 5 ? "QWEN27_WORK_HOST_READY_497" : mode == 35 ? "QWEN27_FULL_OFFLOAD_RESIDENT_VOCAB_READY" : "QWEN27_FULL_OFFLOAD_VOCAB_READY")) return 0;
	if (mode == 5 || mode == 6 || mode == 26) { Read(frame); return 0; }
	if (mode == 35) {
		for (unsigned request = 0; request < 2; request++) {
			if (!Read(frame) || frame.kind != A::Generate || frame.id != request + 2
				|| frame.value != 2 || frame.text != (request ? "second" : "first")) return 2;
			if (!Reply(A::Text, frame.id, 0, request ? "two" : "one")) return 0;
			const char* summary = "P100A full-offload resident candidate: 1 output tokens; prompt+generation 1.000s,total 1.500s. All inference on card; CPU vocabulary/text only. Request closed cleanly; weights retained until explicit model END. Not a warm-token speed measurement.";
			if (!Reply(A::Done, frame.id, A::Complete, summary)) return 0;
		}
		if (!Read(frame) || frame.kind != A::Unload || frame.id != 4 || frame.value || !frame.text.empty()) return 2;
		Reply(A::Closed, 4, 0, "QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END"); return 0;
	}
	if (!Read(frame)) return 0;
	if (mode == 34) {
		if (frame.kind != A::Unload || frame.id != 2 || frame.value || !frame.text.empty()) return 2;
		Reply(A::Closed, 2, 0, "QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END"); return 0;
	}
	if (frame.kind != A::Generate || frame.id != 2 || frame.value != 4 || frame.text != "What is 2 + 2?") return 2;
	if (mode == 8) { Reply(A::Error, 2, A::Fatal, "Synthetic unknown"); return 0; }
	if (mode == 9) return 0;
	if (mode == 24) { send(1, "partial", 7, 0); return 0; }
	if (mode == 30 || mode == 31) {
		if (!Reply(A::Progress, 2, UINT32_MAX, "ASK_TEST_CANCEL_POINT") || !Read(frame)
			|| frame.kind != A::Cancel || frame.id != 2 || frame.value || !frame.text.empty()) return 2;
	}
	if (mode == 2 || mode == 3 || mode == 30) {
		if (!Reply(A::Done, 2, mode == 2 ? A::Rejected : A::Cancelled, "Synthetic clean short result")) return 0;
	} else {
		if (mode == 11) { Reply(A::Done, 2); Read(frame); return 0; }
		if (!Reply(A::Text, mode == 10 ? 3 : 2, mode == 27 ? 1 : 0, mode == 1 ? "" : "4")) return 0;
		if (mode == 10 || mode == 27) { Read(frame); return 0; }
		if (mode == 12) { Reply(A::Text, 2, 0, "duplicate"); Read(frame); return 0; }
		if (mode == 13) { Reply(A::Progress, 2, 0, "late"); Read(frame); return 0; }
		if (mode == 29) { Reply(A::Done, 2, A::Cancelled, "invalid text before cancellation"); Read(frame); return 0; }
		char summary[512];
		snprintf(summary, sizeof(summary),
			"P100A full-offload candidate: %u output tokens; prompt+generation %ss,total %ss. All inference on card; CPU vocabulary/text only. Clean END; weights released. Not a warm-token speed measurement.%s",
			mode == 1 ? 0 : mode == 15 ? 5 : 1, mode == 16 ? "nan" : "2.000", mode == 17 ? "1.000" : "4.000",
			mode == 18 ? "extra" : "");
		if (!Reply(A::Done, 2, mode == 28 ? A::Fatal : A::Complete,
			mode == 14 ? "P100A hybrid Work;CPU attention/state" : summary)) return 0;
		if (mode == 14 || mode == 15 || mode == 16 || mode == 17 || mode == 18 || mode == 28) { Read(frame); return 0; }
		if (mode == 25) Reply(A::Done, 2, A::Complete, summary);
	}
	if (!Read(frame)) return 0;
	if (frame.kind != A::Unload || frame.id != 3 || frame.value || !frame.text.empty()) return 2;
	if (mode == 23) return 0;
	Reply(A::Closed, mode == 20 ? 4 : 3, mode == 21 ? 1 : 0,
		mode == 19 ? "QWEN27_KNOWN_CLEAN_END" : "QWEN27_FULL_OFFLOAD_KNOWN_CLEAN_END");
	return mode == 22 ? 1 : 0;
}
