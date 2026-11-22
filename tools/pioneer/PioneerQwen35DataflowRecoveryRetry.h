/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RECOVERY_RETRY_H
#define PIONEER_QWEN35_DATAFLOW_RECOVERY_RETRY_H
namespace PioneerQwen35 {
enum DataflowRecoveryAttemptResult {
	DataflowRecoveryAttemptComplete,DataflowRecoveryAttemptPending,
	DataflowRecoveryAttemptStop
};

// Only pending settlement is retryable. Each attempt must rerun the driver's
// full fence and fleet audit; this loop never clears the latch or grants a
// completion proof. An interrupted delay stops immediately.
template<class IO> bool DataflowRecoveryRetry(IO& io,unsigned maxAttempts) {
	if (!maxAttempts) return false;
	for (unsigned attempt=0;attempt<maxAttempts;attempt++) {
		const auto result=io.Attempt(attempt);
		if (result==DataflowRecoveryAttemptComplete) return true;
		if (result!=DataflowRecoveryAttemptPending || attempt+1==maxAttempts
			|| !io.Delay()) return false;
	}
	return false;
}
}
#endif
