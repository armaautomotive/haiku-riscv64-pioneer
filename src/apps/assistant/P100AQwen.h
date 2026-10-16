/* SPDX-License-Identifier: MIT */
#ifndef HAIKU_ASSISTANT_P100A_QWEN_H
#define HAIKU_ASSISTANT_P100A_QWEN_H
#include <functional>
#include <string>
class AssistantP100AQwen {
public:
	AssistantP100AQwen();
	~AssistantP100AQwen();
	static bool Available(); // Read-only resident ABI; no BEGIN or compute.
	bool Load(const char* path,std::string& error,bool validate=true);
	// Worker-only idle mode change; retains card weights and session.
	bool SetValidation(bool enabled,std::string& error);
	bool Complete(const std::string& question,int tokens,std::string& answer,
		std::string& error,const std::function<bool()>& cancelled,
		const std::function<void(const std::string&,const char*)>& progress);
	bool Faulted() const { return fFaulted; }
private:
	struct Data;
	Data* fData;
	bool fFaulted;
	AssistantP100AQwen(const AssistantP100AQwen&);
	AssistantP100AQwen& operator=(const AssistantP100AQwen&);
};
#endif
