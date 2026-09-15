#ifndef HAIKU_ASSISTANT_PROTOCOL_H
#define HAIKU_ASSISTANT_PROTOCOL_H

#include <SupportDefs.h>

static const char* const kAssistantSignature = "application/x-vnd.Haiku-Assistant";
static const char* const kServiceSignature = "application/x-vnd.Haiku-AssistantService";
static const uint32 kGenerate = 'agen';
static const uint32 kCancel = 'acan';
static const uint32 kServiceStatus = 'asta';
static const uint32 kConfigureThreads = 'athr';
static const uint32 kUpdate = 'updt';
static const uint32 kDone = 'done';
static const uint32 kToggle = 'atog';

#endif
