/* SPDX-License-Identifier: MIT */
#ifndef ASSISTANT_P100A_QWEN27_FULL_OFFLOAD_H
#define ASSISTANT_P100A_QWEN27_FULL_OFFLOAD_H
#include <functional>
#include <string>

// Separate, one-request full-card interface. NOT the hybrid Work adapter.
// The controlled worker authenticates the model and exact driver capability;
// this client has no device/model API or CPU inference/fallback path. Text
// becomes public only after device END, known worker close AND actual exit0.
// A failed/unknown call is close-only; it does not authorize another trial,
// reset, driver exchange, power operation or accelerator-resource disposal.
class AssistantP100AQwen27FullOffload {
public:
	enum Outcome { Completed, Rejected, Cancelled, Unknown };
	struct Reply {
		Outcome outcome = Unknown;
		bool cleanWorkerExit = false;
		unsigned outputTokens = 0;
		std::string answer, status, error;
	};
	static Reply Run(const std::string& question, unsigned tokens,
		const std::function<bool()>& cancelled,
		const std::function<void(float, const char*)>& progress);
};
#endif
