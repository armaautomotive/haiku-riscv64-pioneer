/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRecoveryRetry.h"
#include <cassert>
using namespace PioneerQwen35;
struct RetryMock {
	DataflowRecoveryAttemptResult results[5]{};
	unsigned attempts=0,delays=0;
	bool delayOkay=true;
	DataflowRecoveryAttemptResult Attempt(unsigned index) {
		assert(index==attempts);return results[attempts++];
	}
	bool Delay() { delays++;return delayOkay; }
};
int main() {
	RetryMock success;success.results[0]=DataflowRecoveryAttemptPending;
	success.results[1]=DataflowRecoveryAttemptPending;success.results[2]=DataflowRecoveryAttemptComplete;
	assert(DataflowRecoveryRetry(success,4) && success.attempts==3 && success.delays==2);
	RetryMock exhausted;for (auto& result:exhausted.results) result=DataflowRecoveryAttemptPending;
	assert(!DataflowRecoveryRetry(exhausted,4) && exhausted.attempts==4 && exhausted.delays==3);
	RetryMock fatal;fatal.results[0]=DataflowRecoveryAttemptStop;
	assert(!DataflowRecoveryRetry(fatal,4) && fatal.attempts==1 && !fatal.delays);
	RetryMock interrupted;interrupted.results[0]=DataflowRecoveryAttemptPending;interrupted.delayOkay=false;
	assert(!DataflowRecoveryRetry(interrupted,4) && interrupted.attempts==1 && interrupted.delays==1);
	RetryMock none;assert(!DataflowRecoveryRetry(none,0) && !none.attempts && !none.delays);
}
