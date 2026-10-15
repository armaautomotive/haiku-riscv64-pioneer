/* Copyright 2026. Distributed under the terms of the MIT License. */
// Native Haiku regression tests; all fixtures are private temporary files.
#include "ModelStore.h"
#include "ModelStorePrivate.h"
#include "Settings.h"
#include <Application.h>
#undef NDEBUG
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct VerifyWork {
	std::atomic<bool> cancelled;
	BString error;
	VerifyWork() : cancelled(false) {}
};

static int32 VerifyOnWorker(void* data)
{
	VerifyWork* work = static_cast<VerifyWork*>(data);
	return VerifyAssistantModel(0, AssistantModelPath(0).String(), work->cancelled, work->error);
}

static int32 InstallOnWorker(void* data)
{
	VerifyWork* work = static_cast<VerifyWork*>(data);
	return InstallAssistantModel(2, work->cancelled, BMessenger(), work->error);
}

int main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-AssistantModelStoreTest");
	std::atomic<bool> cancelled(false);
	BString error;
	if (argc == 4 && strcmp(argv[1], "--expect-settings") == 0) {
		assert(LoadAssistantModel() == argv[2]);
		assert(LoadAssistantThreads() == atoi(argv[3]));
		puts("PASS: saved model path and CPU-thread setting preserved together");
		return 0;
	}
	if (argc == 2 && strcmp(argv[1], "--cancel-download") == 0) {
		assert(!AssistantModelPresent(2));
		VerifyWork work;
		thread_id worker = spawn_thread(InstallOnWorker, "model install worker test", B_NORMAL_PRIORITY, &work);
		assert(worker >= 0 && resume_thread(worker) == B_OK);
		snooze(1000000); work.cancelled.store(true);
		status_t result; assert(wait_for_thread(worker, &result) == B_OK && result == B_CANCELED);
		assert(!AssistantModelPresent(2));
		puts("PASS: live HTTPS download cancellation; no model published");
		return 0;
	}
	assert(AssistantModelIndex(AssistantModelPath(0).String()) == 0);
	assert(AssistantModelIndex("/tmp/not-a-catalog-model.gguf") == -1);
	assert(AssistantModelPath(kAssistantModelCount).IsEmpty());
	assert(!AssistantModelPresent(kAssistantModelCount));
	assert(InstallAssistantModel(3, cancelled, BMessenger(), error) == B_NOT_SUPPORTED);
	assert(InstallAssistantModel(kAssistantModelCount, cancelled, BMessenger(), error) == B_NOT_SUPPORTED);
	assert(VerifyAssistantModel(kAssistantModelCount, "/tmp/missing", cancelled, error) == B_BAD_VALUE);
	char fixture[] = "/tmp/assistant-model-test-XXXXXX";
	int fd = mkstemp(fixture); assert(fd >= 0);
	assert(write(fd, "GGUF", 4) == 4);
	BString published(fixture); published << ".published";
	assert(AssistantModelStorePrivate::PublishModel(fixture, published.String(), 4,
		cancelled, BMessenger(), error) == B_OK);
	assert(AssistantModelStorePrivate::PublishModel(fixture, published.String(), 4,
		cancelled, BMessenger(), error) == B_FILE_EXISTS);
	int check = open(published.String(), O_RDONLY); assert(check >= 0);
	char magic[4]; assert(read(check, magic, 4) == 4 && memcmp(magic, "GGUF", 4) == 0);
	close(check); unlink(published.String());
	cancelled.store(true);
	assert(AssistantModelStorePrivate::PublishModel(fixture, published.String(), 4,
		cancelled, BMessenger(), error) == B_CANCELED);
	struct stat st; assert(lstat(published.String(), &st) != 0);
	cancelled.store(false);
	assert(VerifyAssistantModel(0, fixture, cancelled, error) == B_BAD_DATA);
	assert(ftruncate(fd, kAssistantModels[0].bytes) == 0);
	cancelled.store(true);
	assert(VerifyAssistantModel(0, fixture, cancelled, error) == B_CANCELED);
	cancelled.store(false);
	assert(VerifyAssistantModel(0, fixture, cancelled, error) == B_BAD_DATA);
	close(fd);
	BString alias(fixture); alias << ".link";
	assert(symlink(fixture, alias.String()) == 0);
	assert(VerifyAssistantModel(0, alias.String(), cancelled, error) != B_OK);
	unlink(alias.String()); unlink(fixture);
	assert(VerifyAssistantModel(0, "/tmp", cancelled, error) != B_OK);
	if (AssistantModelPresent(0)) {
		VerifyWork work;
		thread_id worker = spawn_thread(VerifyOnWorker, "verify model stack test", B_NORMAL_PRIORITY, &work);
		assert(worker >= 0 && resume_thread(worker) == B_OK);
		status_t result; assert(wait_for_thread(worker, &result) == B_OK && result == B_OK);
		assert(InstallAssistantModel(0, cancelled, BMessenger(), error) == B_FILE_EXISTS);
	}
	puts("PASS: catalog bounds, path allowlist, unsupported install, size/hash rejection, cancellation, symlink rejection, existing-file protection, installed model verification");
	return 0;
}
