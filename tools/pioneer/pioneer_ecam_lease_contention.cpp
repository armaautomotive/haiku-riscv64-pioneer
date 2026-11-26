/* SPDX-License-Identifier: MIT */
// Fixed, bounded qualification: one lease holder and one read-only profile.
#include "PioneerEcamLeaseProbe.h"
#include <Errors.h>
#include <fcntl.h>
#include <atomic>
#include <cstdio>
#include <sys/ioctl.h>
#include <unistd.h>

struct Holder {
	int fd;
	pioneer_ecam_lease_hold request;
	int ioctlStatus = -1;
};
struct Reader {
	int fd;
	pioneer_ecam_lease_probe_result result{};
	int ioctlStatus = -1;
	std::atomic<bool> done{false};
};
static int32 hold_thread(void* opaque) {
	Holder& h = *static_cast<Holder*>(opaque);
	h.ioctlStatus = ioctl(h.fd, PIONEER_ECAM_LEASE_HOLD, &h.request, sizeof(h.request));
	return B_OK;
}
static int32 read_thread(void* opaque) {
	Reader& r = *static_cast<Reader*>(opaque);
	r.ioctlStatus = ioctl(r.fd, PIONEER_ECAM_LEASE_PROBE, &r.result, sizeof(r.result));
	r.done.store(true);
	return B_OK;
}
static bool successful_probe(const pioneer_ecam_lease_probe_result& r) {
	return r.version == 1 && r.managerStatus == B_OK && r.pciStatus == B_OK && r.cards == 1
		&& r.profileStatus == B_OK && r.profileStage == 6 && r.profileReady == 1
		&& r.leaseModuleStatus == B_OK && r.leaseStatus == B_OK && r.leaseCallbackStatus == B_CANCELED
		&& r.leaseCallbacks == 1 && r.recoveryModuleStatus == B_OK
		&& r.recoveryStatus == B_NOT_ALLOWED && r.recoveryCallbacks == 0;
}
int main() {
	int a = open("/dev/misc/pioneer_ecam_lease_probe", O_RDONLY);
	int b = open("/dev/misc/pioneer_ecam_lease_probe", O_RDONLY);
	int c = open("/dev/misc/pioneer_ecam_lease_probe", O_RDONLY);
	if (a < 0 || b < 0 || c < 0) { std::perror("open ECAM contention probe"); return 1; }
	sem_id ready = create_sem(0, "q35 ecam ready"), release = create_sem(0, "q35 ecam release");
	if (ready < B_OK || release < B_OK) { std::fprintf(stderr,"semaphore creation failed\n"); return 1; }
	Holder holder{a, {1, ready, release, B_NO_INIT, 0, 0, B_NO_INIT, B_NO_INIT}};
	Reader reader{b};
	thread_id holding = spawn_thread(hold_thread, "q35 ecam lease holder", B_NORMAL_PRIORITY, &holder);
	thread_id reading = -1;
	bool observed = false, readySeen = false, startFailed = false;
	if (holding < B_OK || resume_thread(holding) != B_OK) startFailed = true;
	if (!startFailed && acquire_sem_etc(ready, 1, B_RELATIVE_TIMEOUT, 4000000) == B_OK) {
		readySeen = true;
		reading = spawn_thread(read_thread, "q35 ecam competing read", B_NORMAL_PRIORITY, &reader);
		if (reading < B_OK || resume_thread(reading) != B_OK) startFailed = true;
		else {
			const bigtime_t deadline = system_time() + 1500000;
			while (system_time() < deadline && !reader.done.load()) {
				pioneer_ecam_lease_state state{};
				if (ioctl(c, PIONEER_ECAM_LEASE_STATE, &state, sizeof(state)) == 0
					&& state.version == 1 && state.stateStatus == B_OK && state.active == 1
					&& state.waiter == reading && state.waiters > 0 && !reader.done.load()) {
					observed = true; break;
				}
				snooze(10000);
			}
		}
	}
	// Always release the bounded hold, including on a failed observation.
	(void)release_sem(release);
	status_t holdingResult = B_ERROR, readingResult = B_ERROR;
	status_t holdingWait = holding >= B_OK ? wait_for_thread_etc(holding, B_RELATIVE_TIMEOUT, 5000000, &holdingResult) : B_BAD_VALUE;
	status_t readingWait = reading >= B_OK ? wait_for_thread_etc(reading, B_RELATIVE_TIMEOUT, 5000000, &readingResult) : B_BAD_VALUE;
	if (holdingWait != B_OK || (reading >= B_OK && readingWait != B_OK)) {
		std::fprintf(stderr,"CONTENTION_PROBE retained=1 holder_wait=%ld reader_wait=%ld\n",(long)holdingWait,(long)readingWait);
		return 2; // caller must retain the driver/lock if a thread did not finish
	}
	const bool holderOk = !startFailed && readySeen && holdingResult == B_OK && holder.ioctlStatus == 0
		&& holder.request.profileStatus == B_OK && holder.request.profileStage == 6
		&& holder.request.profileReady == 1 && holder.request.holdStatus == B_OK && holder.request.callbackStatus == B_OK;
	const bool readerOk = reading >= B_OK && readingResult == B_OK && reader.ioctlStatus == 0 && successful_probe(reader.result);
	pioneer_ecam_lease_state finalState{};
	const bool stateOk = ioctl(c, PIONEER_ECAM_LEASE_STATE, &finalState, sizeof(finalState)) == 0
		&& finalState.stateStatus == B_OK && finalState.active == 0 && finalState.waiters == 0;
	const int closeA=close(a),closeB=close(b),closeC=close(c);
	const status_t deleteReady=delete_sem(ready),deleteRelease=delete_sem(release);
	const bool clean = closeA == 0 && closeB == 0 && closeC == 0
		&& deleteReady == B_OK && deleteRelease == B_OK;
	std::printf("ECAM_CONTENTION observed=%u holder_ok=%u reader_ok=%u state_idle=%u clean=%u\n",
		unsigned(observed),unsigned(holderOk),unsigned(readerOk),unsigned(stateOk),unsigned(clean));
	if (!clean) return 2;
	return observed && holderOk && readerOk && stateOk ? 0 : 1;
}
