/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_MAILBOX_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_MAILBOX_H

#include "PioneerQwen35DataflowKernels.h"
#include "PioneerQwen35DataflowGraphTrace.h"
#include "PioneerQwen35DataflowRoleAsmConstants.h"

namespace PioneerQwen35 {
// Proposed per-tile handoff, in already-saved L1. The coordinator owns
// command/ack; the reader and writer each exclusively own their own receipt.
// This protocol alone does not launch processors, establish cache visibility,
// audit queues, recover failed work, or authorize resource release.
enum : unsigned {
    DataflowRoleMailboxAddress=PIONEER_Q35_ROLE_MAILBOX_ADDRESS,
    DataflowRoleMailboxMagic=0x31524d35,
    DataflowRoleMailboxVersion=2,
    DataflowRoleReader=0,
    DataflowRoleWriter=1,
    DataflowRoleCount=2,
    DataflowRoleReady=0,
    DataflowRoleRunning=1,
    DataflowRoleComplete=2,
    DataflowRoleFailed=3,
    DataflowRoleStartedReader=0x52333551,
    DataflowRoleStartedWriter=0x57333551
};
struct alignas(32) DataflowRoleMailbox {
    uint32_t magic,version;
    volatile uint32_t command,acknowledged;
    volatile uint32_t completed[DataflowRoleCount];
    volatile uint32_t status[DataflowRoleCount];
    volatile uint32_t stopRequest,stopped[DataflowRoleCount];
    volatile uint32_t started[DataflowRoleCount];
    uint32_t reserved[3];
};
// Dedicated pre-stack handshake. Each role writes only its own receipt; the
// host writes release only after checking both copies against its retained
// snapshots. This does not establish physical ownership or write exclusion.
struct DataflowRoleSnapshotHandoff {
    uint32_t readerReady,writerReady,release;
};
enum : unsigned {
    DataflowRoleSnapshotHandoffAddress=PIONEER_Q35_ROLE_SNAPSHOT_HANDOFF,
    DataflowRoleSnapshotReaderReady=PIONEER_Q35_ROLE_SNAPSHOT_READER_READY,
    DataflowRoleSnapshotWriterReady=PIONEER_Q35_ROLE_SNAPSHOT_WRITER_READY,
    DataflowRoleSnapshotRelease=PIONEER_Q35_ROLE_SNAPSHOT_RELEASE,
    DataflowRoleSnapshotCancel=PIONEER_Q35_ROLE_SNAPSHOT_CANCEL,
    DataflowRoleSnapshotReaderCancelled=PIONEER_Q35_ROLE_SNAPSHOT_READER_CANCELLED,
    DataflowRoleSnapshotWriterCancelled=PIONEER_Q35_ROLE_SNAPSHOT_WRITER_CANCELLED
};
static_assert(sizeof(DataflowRoleSnapshotHandoff)==12
    && DataflowRoleSnapshotHandoffAddress>=DataflowRoleMailboxAddress+sizeof(DataflowRoleMailbox)
    && DataflowRoleSnapshotHandoffAddress+sizeof(DataflowRoleSnapshotHandoff)<=DataflowBarrierArrivalBase,
    "pre-stack handoff must be in saved L1 outside the role mailbox and barriers");
static_assert(sizeof(DataflowRoleMailbox)==64,"two aligned role handoff records");
static_assert(DataflowRoleMailboxAddress>=DataflowGraphTraceAddress
        +DataflowGraphTraceSlots*sizeof(DataflowGraphTraceRecord)
    && DataflowRoleMailboxAddress+sizeof(DataflowRoleMailbox)<=DataflowBarrierArrivalBase,
    "role handoff is in saved L1 outside traces and barrier arrivals");

inline bool DataflowRoleMailboxHeaderValid(const DataflowRoleMailbox& box)
{
    if (box.magic!=DataflowRoleMailboxMagic || box.version!=DataflowRoleMailboxVersion)
        return false;
    for (unsigned i=0;i<sizeof(box.reserved)/sizeof(box.reserved[0]);i++)
        if (box.reserved[i]) return false;
    return true;
}
// Each role publishes its own receipt only after its startup shim has disabled
// the non-coherent L0 cache and initialized its private stack. A zeroed box
// is not evidence that either processor actually started.
inline uint32_t DataflowRoleStartedValue(unsigned role)
{
    return role==DataflowRoleReader?DataflowRoleStartedReader:DataflowRoleStartedWriter;
}
inline bool DataflowRoleMailboxPublishStarted(DataflowRoleMailbox& box,unsigned role)
{
    if (role>=DataflowRoleCount || !DataflowRoleMailboxHeaderValid(box)
        || DataflowLoadAcquire(box.started[role]) || DataflowLoadAcquire(box.command)
        || DataflowLoadAcquire(box.acknowledged) || DataflowLoadAcquire(box.stopRequest)
        || DataflowLoadAcquire(box.stopped[role]) || DataflowLoadAcquire(box.completed[role])
        || DataflowLoadAcquire(box.status[role])!=DataflowRoleReady) return false;
    DataflowStoreRelease(box.started[role],DataflowRoleStartedValue(role));
    return true;
}
inline bool DataflowRoleMailboxAllStarted(const DataflowRoleMailbox& box)
{
    if (!DataflowRoleMailboxHeaderValid(box)) return false;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (DataflowLoadAcquire(box.started[role])!=DataflowRoleStartedValue(role)) return false;
    return true;
}
// The driver can use this on a stable, freshly read startup snapshot before
// accepting the service. Receipts do not prove PC location or NoC idleness.
inline bool DataflowRoleMailboxStartupReady(const DataflowRoleMailbox& box)
{
    if (!DataflowRoleMailboxAllStarted(box) || DataflowLoadAcquire(box.command)
        || DataflowLoadAcquire(box.acknowledged) || DataflowLoadAcquire(box.stopRequest)) return false;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (DataflowLoadAcquire(box.completed[role]) || DataflowLoadAcquire(box.stopped[role])
            || DataflowLoadAcquire(box.status[role])!=DataflowRoleReady) return false;
    return true;
}
inline bool DataflowRoleMailboxReceipt(const DataflowRoleMailbox& box,
    unsigned role,uint32_t sequence,unsigned expectedStatus)
{
    if (role>=DataflowRoleCount) return false;
    const uint32_t first=DataflowLoadAcquire(box.completed[role]);
    const uint32_t status=DataflowLoadAcquire(box.status[role]);
    const uint32_t second=DataflowLoadAcquire(box.completed[role]);
    return first==second && first==sequence && status==expectedStatus;
}
// An idle service may have completed many projection commands. Its role
// sequence is independent of the graph command sequence.
inline bool DataflowRoleMailboxIdle(const DataflowRoleMailbox& box)
{
    if (!DataflowRoleMailboxAllStarted(box) || DataflowLoadAcquire(box.stopRequest)) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    if (ack>DataflowWorkerSequenceLimit || DataflowLoadAcquire(box.command)!=ack) return false;
    const unsigned status=ack?DataflowRoleComplete:DataflowRoleReady;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (DataflowLoadAcquire(box.stopped[role]) || !DataflowRoleMailboxReceipt(box,role,ack,status)) return false;
    return true;
}
// On graph failure the coordinator may leave a command unacknowledged. Both
// roles must have returned a terminal receipt before the driver can audit
// the failed graph; this does not itself prove queues or NoCs are quiet.
inline bool DataflowRoleMailboxFaultSettled(const DataflowRoleMailbox& box)
{
    if (DataflowRoleMailboxIdle(box)) return true;
    if (!DataflowRoleMailboxAllStarted(box) || DataflowLoadAcquire(box.stopRequest)) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    const uint32_t command=DataflowLoadAcquire(box.command);
    if (ack>=DataflowWorkerSequenceLimit || command!=ack+1) return false;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (DataflowLoadAcquire(box.stopped[role])
            || (!DataflowRoleMailboxReceipt(box,role,command,DataflowRoleComplete)
                && !DataflowRoleMailboxReceipt(box,role,command,DataflowRoleFailed))) return false;
    return true;
}
// Only the coordinator may issue. A new sequence cannot reuse queues before
// both roles and the compute owner have had their previous work acknowledged.
inline bool DataflowRoleMailboxIssue(DataflowRoleMailbox& box,uint32_t sequence)
{
    if (!DataflowRoleMailboxAllStarted(box)) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    if (ack>=DataflowWorkerSequenceLimit || sequence!=ack+1
        || DataflowLoadAcquire(box.command)!=ack
        || DataflowLoadAcquire(box.stopRequest)) return false;
    const unsigned expected=ack?DataflowRoleComplete:DataflowRoleReady;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (DataflowLoadAcquire(box.stopped[role])
            || !DataflowRoleMailboxReceipt(box,role,ack,expected)) return false;
    DataflowStoreRelease(box.command,sequence);
    return true;
}
// Called only by the specified role. Clear its old receipt before changing
// status so the coordinator cannot mistake a partially updated record for a
// receipt for the new sequence. The command is never a completion proof.
inline bool DataflowRoleMailboxBegin(DataflowRoleMailbox& box,unsigned role,uint32_t sequence)
{
    if (!DataflowRoleMailboxHeaderValid(box) || role>=DataflowRoleCount || !sequence)
        return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    const unsigned expected=ack?DataflowRoleComplete:DataflowRoleReady;
    if (ack>=DataflowWorkerSequenceLimit || sequence!=ack+1
        || DataflowLoadAcquire(box.command)!=sequence || DataflowLoadAcquire(box.stopRequest)
        || !DataflowRoleMailboxReceipt(box,role,ack,expected)) return false;
    DataflowStoreRelease(box.completed[role],0);
    DataflowStoreRelease(box.status[role],DataflowRoleRunning);
    return true;
}
inline bool DataflowRoleMailboxComplete(DataflowRoleMailbox& box,unsigned role,
    uint32_t sequence,bool success)
{
    if (!DataflowRoleMailboxHeaderValid(box) || role>=DataflowRoleCount || !sequence)
        return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    if (ack>=DataflowWorkerSequenceLimit || sequence!=ack+1
        || DataflowLoadAcquire(box.command)!=sequence || DataflowLoadAcquire(box.stopRequest)
        || DataflowLoadAcquire(box.completed[role])!=0
        || DataflowLoadAcquire(box.status[role])!=DataflowRoleRunning) return false;
    DataflowStoreRelease(box.status[role],success?DataflowRoleComplete:DataflowRoleFailed);
    DataflowStoreRelease(box.completed[role],sequence);
    return true;
}
// The trusted caller supplies independent compute-commit and queue-drain
// results. False, absent or failed role receipts retain the outstanding
// command; this helper never acknowledges failure or proves hardware idle.
inline bool DataflowRoleMailboxAcknowledge(DataflowRoleMailbox& box,uint32_t sequence,
    bool computeCommitted,bool queuesDrained)
{
    if (!DataflowRoleMailboxHeaderValid(box) || !sequence
        || !computeCommitted || !queuesDrained) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    if (ack>=DataflowWorkerSequenceLimit || sequence!=ack+1
        || DataflowLoadAcquire(box.command)!=sequence || DataflowLoadAcquire(box.stopRequest)) return false;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (!DataflowRoleMailboxReceipt(box,role,sequence,DataflowRoleComplete)) return false;
    DataflowStoreRelease(box.acknowledged,sequence);
    return true;
}
// A stop request can be published only when the previous operation has been
// cleanly acknowledged. Each role owns its own stop receipt. A receipt only
// records that the role is leaving its loop; it is NOT proof that its PC has
// parked, that NoCs are quiet, or that any code or storage can be restored.
inline bool DataflowRoleMailboxRequestStop(DataflowRoleMailbox& box)
{
    if (!DataflowRoleMailboxAllStarted(box) || DataflowLoadAcquire(box.stopRequest)) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    if (ack>DataflowWorkerSequenceLimit || DataflowLoadAcquire(box.command)!=ack) return false;
    const unsigned expected=ack?DataflowRoleComplete:DataflowRoleReady;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (DataflowLoadAcquire(box.stopped[role])
            || !DataflowRoleMailboxReceipt(box,role,ack,expected)) return false;
    DataflowStoreRelease(box.stopRequest,ack+1);
    return true;
}
// Fault settlement is separate from acknowledgement. The trusted caller
// must first establish that compute has settled and no queue writer can make
// further progress; both roles must already have terminal receipts. A timeout
// or missing receipt never qualifies. Physical PC/NoC audits are still needed.
inline bool DataflowRoleMailboxRequestFaultStop(DataflowRoleMailbox& box,
    bool computeSettled,bool queuesQuiescent,bool operationFailed)
{
    if (!DataflowRoleMailboxHeaderValid(box) || !computeSettled || !queuesQuiescent
        || DataflowLoadAcquire(box.stopRequest)) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    const uint32_t command=DataflowLoadAcquire(box.command);
    if (ack>=DataflowWorkerSequenceLimit || command!=ack+1) return false;
    bool failure=operationFailed;
    for (unsigned role=0;role<DataflowRoleCount;role++) {
        if (DataflowLoadAcquire(box.stopped[role])) return false;
        if (DataflowRoleMailboxReceipt(box,role,command,DataflowRoleFailed)) failure=true;
        else if (!DataflowRoleMailboxReceipt(box,role,command,DataflowRoleComplete)) return false;
    }
    if (!failure) return false;
    DataflowStoreRelease(box.stopRequest,command);
    return true;
}
inline bool DataflowRoleMailboxStopStateValid(const DataflowRoleMailbox& box,
    uint32_t ack,uint32_t command,uint32_t stop)
{
    if (!stop || ack>DataflowWorkerSequenceLimit) return false;
    if (command==ack && stop==ack+1) {
        const unsigned expected=ack?DataflowRoleComplete:DataflowRoleReady;
        for (unsigned role=0;role<DataflowRoleCount;role++)
            if (!DataflowRoleMailboxReceipt(box,role,ack,expected)) return false;
        return true;
    }
    if (ack>=DataflowWorkerSequenceLimit || command!=ack+1 || stop!=command) return false;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (!DataflowRoleMailboxReceipt(box,role,command,DataflowRoleComplete)
            && !DataflowRoleMailboxReceipt(box,role,command,DataflowRoleFailed)) return false;
    return true;
}
inline bool DataflowRoleMailboxObserveStop(DataflowRoleMailbox& box,unsigned role)
{
    if (!DataflowRoleMailboxHeaderValid(box) || role>=DataflowRoleCount) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    const uint32_t stop=DataflowLoadAcquire(box.stopRequest);
    const uint32_t command=DataflowLoadAcquire(box.command);
    if (DataflowLoadAcquire(box.stopped[role])
        || !DataflowRoleMailboxStopStateValid(box,ack,command,stop)) return false;
    DataflowStoreRelease(box.stopped[role],stop);
    return true;
}
inline bool DataflowRoleMailboxStopReceipts(const DataflowRoleMailbox& box)
{
    if (!DataflowRoleMailboxHeaderValid(box)) return false;
    const uint32_t ack=DataflowLoadAcquire(box.acknowledged);
    const uint32_t stop=DataflowLoadAcquire(box.stopRequest);
    const uint32_t command=DataflowLoadAcquire(box.command);
    if (!DataflowRoleMailboxStopStateValid(box,ack,command,stop)) return false;
    for (unsigned role=0;role<DataflowRoleCount;role++)
        if (DataflowLoadAcquire(box.stopped[role])!=stop) return false;
    return true;
}
}
#endif
