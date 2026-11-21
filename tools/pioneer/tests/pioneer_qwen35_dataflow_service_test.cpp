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
	assert(DataflowWorkerCommandSequenceValid(0,0,1));
	assert(DataflowWorkerCommandSequenceValid(1,1,2));
	assert(DataflowWorkerCommandSequenceValid(1,1,7));
	assert(!DataflowWorkerCommandSequenceValid(1,0,2));
	assert(!DataflowWorkerCommandSequenceValid(1,1,1));
	assert(!DataflowWorkerCommandSequenceValid(1,1,0));
	assert(!DataflowWorkerCommandSequenceValid(1,1,
		DataflowWorkerSequenceLimit+1));

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

	DataflowWorkerMailbox stopMailbox{};
	stopMailbox.magic=DataflowWorkerMailboxMagic;
	stopMailbox.version=DataflowWorkerMailboxVersion;
	stopMailbox.state=DataflowWorkerReady;
	assert(DataflowWorkerMailboxStartupValid(stopMailbox));
	stopMailbox.shutdownSequence=DataflowWorkerShutdownRequest;
	assert(!DataflowWorkerMailboxStartupValid(stopMailbox));
	assert(DataflowWorkerStopRequestValid(stopMailbox));
	stopMailbox.state=DataflowWorkerComplete;
	assert(!DataflowWorkerStopRequestValid(stopMailbox));
	stopMailbox.state=DataflowWorkerReady;
	stopMailbox.commandSequence=2;
	assert(!DataflowWorkerStopRequestValid(stopMailbox));

	DataflowQueueControl queues[DataflowCircularBufferCount]{};
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		queues[i].produced=17+i;
		queues[i].consumed=17+i;
		queues[i].readerFailure=DataflowFailureReader;
		queues[i].reserved[0]=0xfeed;
	}
	assert(DataflowWorkerResetQueueRecords(queues));
	for (unsigned i=0;i<DataflowCircularBufferCount;i++) {
		assert(!queues[i].produced && !queues[i].consumed);
		assert(!queues[i].readerFailure && !queues[i].reserved[0]);
	}
	queues[1].produced=3;
	queues[1].consumed=2;
	queues[0].produced=4;
	queues[0].consumed=4;
	assert(!DataflowWorkerResetQueueRecords(queues));
	assert(queues[0].produced==4 && queues[0].consumed==4);
	assert(queues[1].produced==3 && queues[1].consumed==2);

	std::puts("PASS Qwen35 resident mailbox, queue reuse, and stop protocol");
	return 0;
}
