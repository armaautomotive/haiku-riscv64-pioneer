#ifndef HAIKU_ASSISTANT_TOOL_BRIDGE_H
#define HAIKU_ASSISTANT_TOOL_BRIDGE_H
#include <Message.h>
#include <string>
enum ToolParseResult { kNotToolCall, kValidToolCall, kInvalidToolCall };
const char* AssistantToolInstructions();
ToolParseResult ParseAssistantToolCall(const std::string& output, BMessage& request);
std::string EscapeToolData(const std::string& text);
#endif
