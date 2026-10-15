/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_MODEL_STORE_PRIVATE_H
#define HAIKU_ASSISTANT_MODEL_STORE_PRIVATE_H

#include "ModelStore.h"

namespace AssistantModelStorePrivate {
	// Caller must first verify the complete, privately owned staging file.
	status_t PublishModel(const char* source, const char* destination, off_t size,
		const std::atomic<bool>& cancelled, const BMessenger& target, BString& error);
}

#endif
