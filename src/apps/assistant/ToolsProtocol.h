#ifndef HAIKU_ASSISTANT_TOOLS_PROTOCOL_H
#define HAIKU_ASSISTANT_TOOLS_PROTOCOL_H
#include <SupportDefs.h>
static const char* const kToolsSignature = "application/x-vnd.Haiku-AssistantTools";
static const uint32 kToolCall = 'tool';
static const uint32 kToolResult = 'tres';
static const uint32 kToolCancel = 'tcan';
#endif
