/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>

#include "../PioneerQwen35DataflowBlackholeWorker.h"

using namespace PioneerQwen35;

int main()
{
	DataflowWorkerMailbox mailbox{};
	mailbox.magic=DataflowWorkerMailboxMagic;
	mailbox.version=DataflowWorkerMailboxVersion;
	mailbox.state=DataflowWorkerReady;
	assert(DataflowWorkerMailboxStartupValid(mailbox));

	for (unsigned core=0;core<DataflowCoreCount;core++) {
		const unsigned route=DataflowBlackholeRouteForCore(core);
		unsigned decoded=UINT32_MAX;
		assert(route && DataflowBlackholeLocalCore(route,decoded));
		assert(decoded==core);
		for (unsigned prior=0;prior<core;prior++)
			assert(DataflowBlackholeRouteForCore(prior)!=route);
	}
	unsigned unused=0;
	assert(!DataflowBlackholeLocalCore(0,unused));
	assert(!DataflowBlackholeLocalCore(0xfff,unused));
	assert(DataflowBlackholeRouteForCore(DataflowCoreCount)==0);

	mailbox.commandSequence=1;
	assert(!DataflowWorkerMailboxStartupValid(mailbox));
	mailbox.commandSequence=0;
	mailbox.version++;
	assert(!DataflowWorkerMailboxStartupValid(mailbox));
	mailbox.version=DataflowWorkerMailboxVersion;
	mailbox.reserved[3]=1;
	assert(!DataflowWorkerMailboxStartupValid(mailbox));
	mailbox.reserved[3]=0;
	assert(DataflowWorkerMailboxStartupValid(mailbox));

	DataflowWorkerPublish(mailbox,7,DataflowWorkerComplete,0,2);
	assert(DataflowLoadAcquire(mailbox.completedSequence)==7);
	assert(DataflowLoadAcquire(mailbox.state)==DataflowWorkerComplete);
	assert(DataflowLoadAcquire(mailbox.failure)==0);
	assert(DataflowLoadAcquire(mailbox.phase)==2);

	DataflowWorkerPublish(mailbox,8,DataflowWorkerFailed,DataflowFailureCompute,5);
	assert(DataflowLoadAcquire(mailbox.completedSequence)==8);
	assert(DataflowLoadAcquire(mailbox.state)==DataflowWorkerFailed);
	assert(DataflowLoadAcquire(mailbox.failure)==DataflowFailureCompute);
	assert(DataflowLoadAcquire(mailbox.phase)==5);

	std::puts("PASS Qwen35 resident mailbox ABI, per-core route bijection, and completion publication");
	return 0;
}
