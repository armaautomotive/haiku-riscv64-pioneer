/* SPDX-License-Identifier: MIT */
#ifndef HAIKU_ASSISTANT_P100A_QWEN_H
#define HAIKU_ASSISTANT_P100A_QWEN_H
#include <functional>
#include <string>
class AssistantP100AQwen {
public:
	AssistantP100AQwen();
	~AssistantP100AQwen();
	static bool Available(bool attention=false,bool throughput=false); // Read-only ABI; no BEGIN/compute.
	bool Load(const char* path,std::string& error,bool validate=true,
		const std::function<bool(float,const char*)>& progress = std::function<bool(float,const char*)>(),bool attention=false,bool throughput=false);
	// Idle inference worker only. Frees host contexts/model and proves END
	// before relinquishing an existing resident device session.
	bool Unload(std::string& error);
	// Worker-only idle mode change; retains card weights and session.
	bool SetValidation(bool enabled,std::string& error);
	// Changing attention retains the host model, but closes the device graph
	// and requires a new first-use upload. Idle inference worker only.
	bool SetMode(bool validate,bool attention,std::string& error,bool throughput=false);
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
