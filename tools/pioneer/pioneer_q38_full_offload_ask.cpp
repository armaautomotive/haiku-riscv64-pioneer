/* SPDX-License-Identifier: MIT */
// One real question through the separate full-card worker. External guarded
// driver/artifact/idle admission remains mandatory;this CLI cannot exchange
// drivers or authorize cleanup. No CPU inference or fallback is linked.
#include "../../src/apps/assistant/P100AQwen27FullOffload.h"
#include <cstdio>
#include <cstring>
#include <string>
int main(int argc, char** argv)
{
	if (argc != 5 || std::strcmp(argv[1], "--full-card-question")
		|| std::strcmp(argv[3], "--tokens")) return 2;
	const std::string number = argv[4];
	unsigned tokens = 0;
	if (number.empty() || number.size() > 2 || number[0] == '0') return 2;
	for (char digit : number) {
		if (digit < '0' || digit > '9') return 2;
		tokens = tokens * 10 + unsigned(digit - '0');
	}
	if (!tokens || tokens > 32 || !argv[2][0] || std::strlen(argv[2]) > 256) return 2;
	setvbuf(stdout, nullptr, _IOLBF, 0);
	const auto reply = AssistantP100AQwen27FullOffload::Run(argv[2], tokens,
		[] { return false; }, [](float part, const char* text) {
			printf("FULL_CARD_ASK_PROGRESS %.3f %s\n", double(part), text);
		});
	if (reply.outcome != AssistantP100AQwen27FullOffload::Completed) {
		printf("FULL_CARD_ASK_NOT_COMPLETED outcome%u clean_worker_exit%u %s\n",
			unsigned(reply.outcome), unsigned(reply.cleanWorkerExit), reply.error.c_str());
		return 1;
	}
	printf("FULL_CARD_ASK_DONE %s\n", reply.status.c_str());
	printf("FULL_CARD_ASK_ANSWER tokens%u bytes%zu hex=", reply.outputTokens, reply.answer.size());
	for (unsigned char byte : reply.answer) printf("%02x", unsigned(byte));
	putchar('\n');
	puts("PASS_FULL_CARD_ASK clean_worker_exit1 CPU_INFERENCE_CONTEXTS0;NO_FALLBACK;NOT_BROAD_QUALITY_OR_SPEED_PROOF");
	return 0;
}
