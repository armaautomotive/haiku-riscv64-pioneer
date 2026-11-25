/* SPDX-License-Identifier: MIT */
// Protocol mismatch must never promote a baseline gather as a tree proof.
#include "../PioneerQwen35DataflowResidentReceipt.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
int main() {
    DataflowResidentCommand c{};c.magic=DataflowResidentCommandMagic;c.version=DataflowResidentCommandVersion;
    c.trigger=2;c.epoch=7;c.firstSequence=1000;c.count=DataflowGraphDecodeSteps;
    c.pollLimit=100;c.token=42;c.position=1;c.checksum=DataflowResidentCommandChecksum(c);
    assert(DataflowResidentCommandValid(c));
    const auto candidate=DataflowResidentCandidateBuild(c);
#ifdef PIONEER_QWEN35_TREE_SETTLEMENT
    assert(candidate.version==2);
#else
    assert(candidate.version==1);
#endif
    assert(DataflowResidentCandidateMatches(candidate,c));
    auto other=candidate;other.version=candidate.version==1?2:1;
    other.checksum=DataflowResidentCandidateChecksum(other);
    assert(!DataflowResidentCandidateMatches(other,c));
    puts("PASS baseline and tree candidate versions cannot be confused; OFFLINE ONLY");
}
