/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_P100A_BACKEND_H
#define HAIKU_ASSISTANT_P100A_BACKEND_H
#include <functional>
#include <string>

// Deliberately fixed-shape: this is not a ggml/Qwen accelerator backend.
class AssistantP100A {
public:
	AssistantP100A();
	~AssistantP100A();
	static bool Available(); // Driver ABI metadata only; never executes a tile.
	bool Load(const char* model, const char* tokenizer, std::string& error);
	bool Complete(const std::string& prompt, int tokens, std::string& answer,
		std::string& error, const std::function<bool()>& cancelled,
		const std::function<void(const std::string&, const char*)>& progress);
	bool Faulted() const { return fFaulted; }
private:
	struct Data;
	Data* fData;
	bool fFaulted;
	AssistantP100A(const AssistantP100A&);
	AssistantP100A& operator=(const AssistantP100A&);
};
#endif
