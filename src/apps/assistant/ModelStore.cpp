/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "ModelStore.h"
#include "ModelStorePrivate.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <SHA256.h>
#include <Volume.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <vector>

static BString ResourcePath(const AssistantModel& model)
{
	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path) != B_OK) return BString();
	path.Append("models");
	path.Append(model.filename);
	return BString(path.Path());
}

BString AssistantModelPath(size_t index)
{
	return index < kAssistantModelCount ? ResourcePath(kAssistantModels[index]) : BString();
}

BString AssistantTinyTokenizerPath() { return ResourcePath(kAssistantTinyTokenizer); }

int
AssistantModelIndex(const char* path)
{
	if (path == NULL) return -1;
	for (size_t i = 0; i < kAssistantModelCount; i++) {
		if (AssistantModelPath(i) == path) return (int)i;
		// Existing SD-session fallback; never write here from the installer.
		BString legacy("/Haiku1/home/models/");
		legacy << kAssistantModels[i].filename;
		if (legacy == path) return (int)i;
	}
	return -1;
}

bool
AssistantModelPresent(size_t index)
{
	if (index >= kAssistantModelCount) return false;
	struct stat st;
	if (index == kAssistantTinyModel && (lstat(AssistantTinyTokenizerPath().String(), &st) != 0
		|| !S_ISREG(st.st_mode) || st.st_size != (off_t)kAssistantTinyTokenizer.bytes)) return false;
	return lstat(AssistantModelPath(index).String(), &st) == 0
		&& S_ISREG(st.st_mode) && st.st_size == (off_t)kAssistantModels[index].bytes;
}

static void
Progress(const BMessenger& target, off_t bytes, off_t total, const char* phase)
{
	BMessage message(kModelProgress);
	message.AddInt64("bytes", bytes);
	message.AddInt64("total", total);
	message.AddString("phase", phase);
	target.SendMessage(&message, (BHandler*)NULL, 10000);
}

static status_t
VerifyResource(const AssistantModel& model, const char* path, const std::atomic<bool>& cancelled,
	BString& error, const BMessenger& target)
{
	int fd = open(path, O_RDONLY | O_NOFOLLOW);
	struct stat st;
	if (fd < 0) { error = "Cannot open model file."; return B_FILE_ERROR; }
	if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size != (off_t)model.bytes) {
		close(fd); error = "Model size is wrong or file is not a regular file.";
		return B_BAD_DATA;
	}
	SHA256 hash;
	// spawn_thread workers have a 256 KiB stack; keep the large hash buffer
	// on the heap (the same verifier is also used by the download worker).
	std::vector<char> buffer(256 * 1024);
	off_t bytes = 0;
	bigtime_t last = 0;
	status_t result = B_OK;
	while (bytes < st.st_size) {
		if (cancelled.load()) { result = B_CANCELED; break; }
		ssize_t n = read(fd, buffer.data(), buffer.size());
		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) { result = B_IO_ERROR; break; }
		hash.Update(buffer.data(), n);
		bytes += n;
		if (system_time() - last > 250000) {
			Progress(target, bytes, st.st_size, "Verifying SHA-256"); last = system_time();
		}
	}
	close(fd);
	if (result != B_OK) {
		error = result == B_CANCELED ? "Cancelled; installed models unchanged." : "Could not read model.";
		return result;
	}
	char digest[65];
	const uint8* raw = hash.Digest();
	for (int i = 0; i < 32; i++) snprintf(digest + i * 2, 3, "%02x", raw[i]);
	if (strcmp(digest, model.sha256) != 0) {
		error = "SHA-256 mismatch. File was not activated or installed."; return B_BAD_DATA;
	}
	return B_OK;
}

status_t VerifyAssistantModel(size_t index, const char* path,
	const std::atomic<bool>& cancelled, BString& error, const BMessenger& target)
{
	if (index >= kAssistantModelCount) return B_BAD_VALUE;
	status_t status = VerifyResource(kAssistantModels[index], path, cancelled, error, target);
	if (status == B_OK && index == kAssistantTinyModel)
		status = VerifyResource(kAssistantTinyTokenizer, AssistantTinyTokenizerPath().String(), cancelled, error, target);
	return status;
}

// Bounded streaming sink: errors/cancellation cannot fill the disk with an
// untrusted HTTP response. It is never the final, visible model file.
class ModelDownload : public BDataIO {
public:
	ModelDownload(int fd, uint64_t limit, const std::atomic<bool>& cancelled,
		const BMessenger& target)
		: fFD(fd), fLimit(limit), fBytes(0), fCancelled(cancelled), fTarget(target), fLast(0) {}
	virtual ssize_t Read(void*, size_t) { return B_NOT_ALLOWED; }
	virtual ssize_t Write(const void* buffer, size_t size)
	{
		if (fCancelled.load()) return B_CANCELED;
		if (size > fLimit - fBytes) return B_FILE_TOO_LARGE;
		size_t done = 0;
		while (done < size) {
			ssize_t n = write(fFD, (const char*)buffer + done, size - done);
			if (n < 0 && errno == EINTR) continue;
			if (n <= 0) return B_IO_ERROR;
			done += n;
		}
		fBytes += size;
		if (system_time() - fLast > 250000) {
			Progress(fTarget, fBytes, fLimit, "Downloading"); fLast = system_time();
		}
		return size;
	}
private:
	int fFD;
	uint64_t fLimit, fBytes;
	const std::atomic<bool>& fCancelled;
	BMessenger fTarget;
	bigtime_t fLast;
};

// The current Pioneer libbnetapi lacks TLS. Use the installed HTTPS-capable
// wget without a shell, user config, cookies or HSTS writes. Follow redirects
// here, not inside wget, so every hop must remain HTTPS. Never execute model
// content. Drain stdout/stderr concurrently and bound both untrusted streams.
static status_t
DownloadURL(const BString& url, ModelDownload& sink, const std::atomic<bool>& cancelled,
	BString& location, int& code)
{
	if (url.FindFirst("https://") != 0 || url.Length() > 16384) return B_BAD_VALUE;
	int output[2], headers[2];
	if (pipe(output) != 0) return B_IO_ERROR;
	if (pipe(headers) != 0) { close(output[0]); close(output[1]); return B_IO_ERROR; }
	pid_t child = fork();
	if (child == 0) {
		dup2(output[1], STDOUT_FILENO); dup2(headers[1], STDERR_FILENO);
		close(output[0]); close(output[1]); close(headers[0]); close(headers[1]);
		execl("/boot/system/bin/wget", "wget", "--no-config", "--no-hsts", "--no-cookies",
			"--timeout=20", "--tries=1", "--max-redirect=0", "--server-response",
			"--no-verbose", "--output-document=-", "--", url.String(), (char*)NULL);
		_exit(127);
	}
	close(output[1]); close(headers[1]);
	if (child < 0) { close(output[0]); close(headers[0]); return B_ERROR; }
	pollfd pipes[2] = {{output[0], POLLIN, 0}, {headers[0], POLLIN, 0}};
	BString response;
	status_t status = B_OK;
	char buffer[32768];
	while (pipes[0].fd >= 0 || pipes[1].fd >= 0) {
		if (cancelled.load()) { status = B_CANCELED; break; }
		int result = poll(pipes, 2, 100);
		if (result < 0 && errno == EINTR) continue;
		if (result < 0) { status = B_IO_ERROR; break; }
		for (int i = 0; i < 2; i++) {
			if (pipes[i].fd < 0 || pipes[i].revents == 0) continue;
			ssize_t n = read(pipes[i].fd, buffer, sizeof(buffer));
			if (n < 0 && errno == EINTR) continue;
			if (n <= 0) { close(pipes[i].fd); pipes[i].fd = -1; continue; }
			if (i == 0) {
				ssize_t written = sink.Write(buffer, n);
				if (written != n) { status = written < 0 ? written : B_IO_ERROR; break; }
			} else {
				if (response.Length() + n > 65536) { status = B_BAD_DATA; break; }
				response.Append(buffer, n);
			}
		}
		if (status != B_OK) break;
	}
	for (int i = 0; i < 2; i++) if (pipes[i].fd >= 0) close(pipes[i].fd);
	if (status != B_OK) kill(child, SIGTERM);
	int waitStatus = 0;
	bigtime_t stopping = system_time();
	while (waitpid(child, &waitStatus, WNOHANG) == 0) {
		if (cancelled.load() || status != B_OK) {
			kill(child, system_time() - stopping > 2000000 ? SIGKILL : SIGTERM);
			if (cancelled.load()) status = B_CANCELED;
		}
		snooze(10000);
	}
	if (status != B_OK) return status;
	// Only parse server-response header lines, not wget's diagnostic text.
	int32 start = 0;
	while (start < response.Length()) {
		int32 end = response.FindFirst('\n', start);
		if (end < 0) end = response.Length();
		BString line; response.CopyInto(line, start, end - start);
		if (line.FindFirst("  HTTP/") == 0) {
			int parsed = 0;
			if (sscanf(line.String(), "  HTTP/%*s %d", &parsed) == 1) { code = parsed; location = ""; }
		} else if (line.IFindFirst("  Location: ") == 0) {
			line.Remove(0, 12); line.Trim(); location = line;
		}
		start = end + 1;
	}
	if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308)
		return location.IsEmpty() ? B_BAD_DATA : B_OK;
	return WIFEXITED(waitStatus) && WEXITSTATUS(waitStatus) == 0 && code == 200 ? B_OK : B_IO_ERROR;
}

status_t
AssistantModelStorePrivate::PublishModel(const char* source, const char* destination, off_t size,
	const std::atomic<bool>& cancelled, const BMessenger& target, BString& error)
{
	// BFS has neither hard links nor an atomic no-replace rename. Reserve the
	// destination with O_EXCL and copy the already verified private staging
	// file. A partial destination is never listed as installed (wrong length)
	// and the service independently verifies every model before loading it.
	int output = open(destination, O_WRONLY | O_CREAT | O_EXCL, 0600);
	if (output < 0) {
		error = "Could not create model; existing files were not overwritten."; return B_FILE_EXISTS;
	}
	int input = open(source, O_RDONLY | O_NOFOLLOW);
	status_t status = input >= 0 ? B_OK : B_IO_ERROR;
	std::vector<char> buffer(256 * 1024);
	off_t copied = 0;
	bigtime_t last = 0;
	while (status == B_OK && copied < size) {
		if (cancelled.load()) { status = B_CANCELED; break; }
		size_t chunk = (size - copied) < (off_t)buffer.size() ? size - copied : buffer.size();
		ssize_t n = read(input, buffer.data(), chunk);
		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) { status = B_IO_ERROR; break; }
		ssize_t done = 0;
		while (done < n) {
			ssize_t written = write(output, buffer.data() + done, n - done);
			if (written < 0 && errno == EINTR) continue;
			if (written <= 0) { status = B_IO_ERROR; break; }
			done += written;
		}
		copied += done;
		if (system_time() - last > 250000) {
			Progress(target, copied, size, "Installing verified model"); last = system_time();
		}
	}
	if (status == B_OK && fsync(output) != 0) status = B_IO_ERROR;
	if (input >= 0) close(input);
	close(output);
	if (status != B_OK) {
		unlink(destination);
		error = status == B_CANCELED ? "Cancelled; installed models unchanged." : "Could not copy/sync verified model.";
	}
	return status;
}

static status_t
InstallResource(const AssistantModel& model, const char* remotePath, bool resumeBundle,
	const std::atomic<bool>& cancelled, const BMessenger& target, BString& error)
{
	BString destination = ResourcePath(model);
	BPath directory(destination.String()); directory.GetParent(&directory);
	status_t status = create_directory(directory.Path(), 0755);
	if (status != B_OK) { error = "Cannot create model folder."; return status; }
	struct stat st;
	if (lstat(destination.String(), &st) == 0 || errno != ENOENT) {
		// A previously completed half of the two-file bundle may be reused,
		// but only after hash verification. Never overwrite an existing file.
		if (resumeBundle) return VerifyResource(model, destination.String(), cancelled, error, target);
		error = "A file already exists at this model path. It was not overwritten."; return B_FILE_EXISTS;
	}
	if (cancelled.load()) { error = "Cancelled; installed models unchanged."; return B_CANCELED; }
	if (time(NULL) < 1735689600) {
		error = "System clock is incorrect. Set Date & Time / network time before HTTPS downloads.";
		return B_NOT_ALLOWED;
	}
	if (stat(directory.Path(), &st) != 0
		|| BVolume(st.st_dev).FreeBytes() < (off_t)model.bytes * 2 + 64 * 1024 * 1024) {
		error = "Not enough free space (twice model size plus 64 MiB required)."; return B_DEVICE_FULL;
	}
	BString temporary(destination); temporary << ".part-XXXXXX";
	int fd = mkstemp(temporary.LockBuffer(temporary.Length())); temporary.UnlockBuffer();
	if (fd < 0) { error = "Cannot create temporary download."; return B_FILE_ERROR; }
	BString url("https://huggingface.co/");
	url << model.repository << "/resolve/" << model.revision << "/" << remotePath;
	status = B_ERROR;
	for (int redirect = 0; redirect <= 5 && !cancelled.load(); redirect++) {
		if (url.FindFirst("https://") != 0 || ftruncate(fd, 0) != 0 || lseek(fd, 0, SEEK_SET) < 0) break;
		ModelDownload sink(fd, model.bytes, cancelled, target);
		int code = 0;
		BString location;
		status_t transport = DownloadURL(url, sink, cancelled, location, code);
		if (transport == B_OK && code == 200) { status = B_OK; break; }
		if (transport == B_OK && !location.IsEmpty()) {
			if (location.FindFirst('/') == 0 && location.FindFirst("//") != 0) {
				int32 slash = url.FindFirst('/', 8);
				if (slash >= 0) url.Truncate(slash);
				url << location;
			} else url = location;
			continue;
		}
		error.SetToFormat("Download failed (HTTP %d). Check clock, network, TLS certificates and wget.", code);
		break;
	}
	if (cancelled.load()) { status = B_CANCELED; error = "Cancelled; installed models unchanged."; }
	if (status == B_OK && fsync(fd) != 0) { status = B_IO_ERROR; error = "Could not sync download."; }
	close(fd);
	if (status == B_OK) status = VerifyResource(model, temporary.String(), cancelled, error, target);
	if (status == B_OK && cancelled.load()) { status = B_CANCELED; error = "Cancelled."; }
	if (status == B_OK) status = AssistantModelStorePrivate::PublishModel(temporary.String(), destination.String(),
		model.bytes, cancelled, target, error);
	unlink(temporary.String());
	if (status != B_OK && error.IsEmpty()) error = "Download failed; no model was installed.";
	return status;
}

status_t InstallAssistantModel(size_t index, const std::atomic<bool>& cancelled,
	const BMessenger& target, BString& error)
{
	if (index >= kAssistantModelCount || !kAssistantModels[index].compatible) {
		error = "This model is not supported by an installed backend."; return B_NOT_SUPPORTED;
	}
	bool tiny = index == kAssistantTinyModel;
	status_t status = InstallResource(kAssistantModels[index], tiny ? "stories260K/stories260K.bin"
		: kAssistantModels[index].filename, tiny, cancelled, target, error);
	if (status == B_OK && tiny)
		status = InstallResource(kAssistantTinyTokenizer, "stories260K/tok512.bin", true, cancelled, target, error);
	return status;
}
