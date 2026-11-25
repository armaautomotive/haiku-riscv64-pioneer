/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_RECEIPT_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_RECEIPT_H
#include "PioneerQwen35DataflowResidentDispatch.h"
#include "PioneerQwen35DataflowGraphTrace.h"
namespace PioneerQwen35 {
// A v1 candidate is only a gather; v2 additionally means the device tree
// checked each sender receipt. Neither version proves installed-image or
// external-writer exclusion. A separate qualified audit is still mandatory.
// The installation must clear this record before use.
enum : unsigned { DataflowResidentCandidateAddress=0xab310,DataflowResidentCandidateAckAddress=0xab330,
    DataflowResidentCandidateMagic=0x31435235,
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    DataflowResidentCandidateVersion=2
#else
    DataflowResidentCandidateVersion=1
#endif
};
struct alignas(16) DataflowResidentCandidate {
    uint32_t magic,version,trigger,epoch,firstSequence,count,checksum,commit;
};
static_assert(sizeof(DataflowResidentCandidate)==32
    && DataflowResidentCandidateAddress>=DataflowGraphOperationProgressAddress+4
    && DataflowResidentCandidateAddress+sizeof(DataflowResidentCandidate)==DataflowResidentCandidateAckAddress
    && DataflowResidentCandidateAckAddress+4*sizeof(uint32_t)<=DataflowGraphTraceAddress,
    "candidate record occupies unused saved L1 between graph progress and trace");
inline uint32_t DataflowResidentCandidateChecksum(const DataflowResidentCandidate& r) {
    const uint32_t words[]={r.magic,r.version,r.trigger,r.epoch,r.firstSequence,r.count};
    uint32_t hash=2166136261U;
    for (uint32_t word:words) hash=(hash^word)*16777619U;
    return hash;
}
inline DataflowResidentCandidate DataflowResidentCandidateBuild(const DataflowResidentCommand& c) {
    if (!DataflowResidentCommandValid(c)) return {};
    DataflowResidentCandidate r{DataflowResidentCandidateMagic,DataflowResidentCandidateVersion,
        c.trigger,c.epoch,c.firstSequence,c.count,0,c.trigger};
    r.checksum=DataflowResidentCandidateChecksum(r);return r;
}
inline bool DataflowResidentCandidateMatches(const DataflowResidentCandidate& r,
    const DataflowResidentCommand& c) {
    const auto expected=DataflowResidentCandidateBuild(c);
    return expected.commit && r.magic==expected.magic && r.version==expected.version
        && r.trigger==expected.trigger && r.epoch==expected.epoch && r.firstSequence==expected.firstSequence
        && r.count==expected.count && r.checksum==expected.checksum && r.commit==expected.commit;
}
inline bool DataflowResidentCandidatePublish(volatile uint32_t* dest,const DataflowResidentCommand& c) {
    const auto r=DataflowResidentCandidateBuild(c);
    if (!dest || !r.commit) return false;
    DataflowStoreRelease(dest[7],0);
    const uint32_t words[]={r.magic,r.version,r.trigger,r.epoch,r.firstSequence,r.count,r.checksum};
    for (unsigned i=0;i<7;i++) DataflowStoreRelease(dest[i],words[i]);
    DataflowFence();DataflowStoreRelease(dest[7],r.commit);return true;
}
// Only the locked kernel owner may write an acknowledgement, and only after
// resolving the candidate's fleet faults. Exact sequence/epoch matching keeps
// an old posted acknowledgement from releasing the next token.
inline bool DataflowResidentCandidateAckMatches(const volatile uint32_t* source,unsigned trigger,unsigned epoch) {
    if (!source || !trigger || !epoch) return false;
    for (unsigned pass=0;pass<2;pass++)
        if (DataflowLoadAcquire(source[0])!=trigger || DataflowLoadAcquire(source[1])!=~trigger
            || DataflowLoadAcquire(source[2])!=epoch || DataflowLoadAcquire(source[3])!=~epoch) return false;
    return true;
}
}
#endif
