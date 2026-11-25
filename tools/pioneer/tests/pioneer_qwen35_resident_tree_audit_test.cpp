/* SPDX-License-Identifier: MIT */
// Root-only tree certificate audit; all device and physical proofs are mocked.
#define PIONEER_QWEN35_TREE_SETTLEMENT 1
#include "resident-fleet-fixture.inc"
#include "../PioneerQwen35DataflowResidentTreeAudit.h"
struct TreeIO : FleetIO {
    bool treeAllowed=true;int treeLossAfter=-1;unsigned treeChecks=0;
    explicit TreeIO(const DataflowResidentCommand& c):FleetIO(c) {}
    bool ResidentTreeAuditAllowed(const DataflowResidentCommand&) {
        ++treeChecks;return treeAllowed && (treeLossAfter<0 || int(treeChecks)<=treeLossAfter);
    }
};
static_assert(sizeof(DataflowResidentTreeAudit)*16<sizeof(DataflowResidentFleetAudit),
    "tree audit must not allocate one snapshot per worker");
int main() {
    for (unsigned scenario=0;scenario<13;scenario++) {
        DataflowResidentCommand c{};c.magic=DataflowResidentCommandMagic;c.version=DataflowResidentCommandVersion;
        c.trigger=2;c.epoch=7;c.firstSequence=1000;c.count=scenario==1?DataflowGraphPromptSteps:DataflowGraphDecodeSteps;
        c.pollLimit=100;c.inputSlot=0;c.token=42;c.position=1;c.checksum=DataflowResidentCommandChecksum(c);
        auto audit=std::make_unique<DataflowResidentTreeAudit>();auto io=std::make_unique<TreeIO>(c);
        if (scenario==2) io->treeAllowed=false;
        if (scenario==3) io->sample[0].worker.state=DataflowWorkerFatal;
        if (scenario==4) io->sample[0].sender.checksum^=1;
        if (scenario==5) io->sample[0].queues[0].produced=1;
        if (scenario==6) io->badRuntime=0;
        if (scenario==7) io->end=false;
        if (scenario==8) io->changeOnConfirm=0;
        if (scenario==9) io->treeLossAfter=4;
        if (scenario==10) io->badRead=0;
        if (scenario==11) io->badFence=2;
        if (scenario==12) io->sample[37].worker.state=DataflowWorkerFatal; // deliberately dishonest physical proof
        const auto result=audit->Audit(c,*io);
        if (scenario<2 || scenario==12) {
            assert(result==DataflowResidentFleetAuditComplete && audit->AuditedFor(c)
                && !audit->Retained() && io->reads==4 && io->selected==0);
            for (unsigned core=1;core<DataflowCoreCount;core++) assert(!io->coreReads[core]);
            auto changed=c;changed.token++;changed.checksum=DataflowResidentCommandChecksum(changed);
            assert(!audit->AuditedFor(changed) && audit->Audit(c,*io)==DataflowResidentFleetAuditInvalid);
        } else if (scenario==2) {
            assert(result==DataflowResidentFleetAuditPreflight && !audit->Retained() && !io->reads);
        } else {
            assert(result==DataflowResidentFleetAuditRetained && audit->Retained() && io->retained
                && !audit->AuditedFor(c) && audit->Audit(c,*io)==DataflowResidentFleetAuditInvalid);
        }
        const auto candidate=DataflowResidentCandidateBuild(c);
        assert(candidate.version==2 && DataflowResidentCandidateMatches(candidate,c));
        auto old=candidate;old.version=1;old.checksum=DataflowResidentCandidateChecksum(old);
        assert(!DataflowResidentCandidateMatches(old,c));
    }
    puts("PASS root-only tree audit requires independent proof and exact v2 receipt; SIMULATED ONLY");
}
