/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_MODEL_STORE_H
#define HAIKU_ASSISTANT_MODEL_STORE_H

#include "ModelCatalog.h"
#include <Messenger.h>
#include <String.h>
#include <atomic>

static const uint32 kModelProgress = 'mprg';
static const uint32 kModelInstalled = 'mins';

BString AssistantModelPath(size_t index);
int AssistantModelIndex(const char* path);
bool AssistantModelPresent(size_t index);
status_t VerifyAssistantModel(size_t index, const char* path,
	const std::atomic<bool>& cancelled, BString& error,
	const BMessenger& target = BMessenger());
status_t InstallAssistantModel(size_t index, const std::atomic<bool>& cancelled,
	const BMessenger& target, BString& error);

#endif
