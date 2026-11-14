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

// Explicit multi-request owner for the resident-worker build. Load retains
// one worker/model session; Complete closes only the current request; Unload
// is the normal model-END boundary. Destruction makes one bounded Unload
// attempt if still loaded; callers must not treat a lost channel as success.
class AssistantP100AQwen27FullOffloadSession {
public:
	using Reply = AssistantP100AQwen27FullOffload::Reply;
	explicit AssistantP100AQwen27FullOffloadSession(bool requireResidentWorker = true);
	~AssistantP100AQwen27FullOffloadSession();
	static bool Available();
	bool Load(const std::function<void(float, const char*)>& progress,
		std::string& error);
	Reply Complete(const std::string& question, unsigned tokens,
		const std::function<bool()>& cancelled,
		const std::function<void(float, const char*)>& progress);
	bool Unload(std::string& error);
	bool Loaded() const;
	bool Faulted() const;
	bool RejectedByWorker() const;
private:
	struct Data;
	Data* fData;
	AssistantP100AQwen27FullOffloadSession(const AssistantP100AQwen27FullOffloadSession&);
	AssistantP100AQwen27FullOffloadSession& operator=(const AssistantP100AQwen27FullOffloadSession&);
};
#endif
