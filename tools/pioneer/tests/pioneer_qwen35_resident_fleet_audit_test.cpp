/* SPDX-License-Identifier: MIT */
// Fleet contents and physical proof are simulated. No device is accessed.
#include "../PioneerQwen35DataflowResidentFleetAudit.h"
#include <cassert>
#include <cstdio>
#include <memory>
using namespace PioneerQwen35;
struct FleetIO {
    DataflowResidentFleetSample sample[DataflowCoreCount]{};
    bool allowed=true,begin=true,end=true,retained=false,mutate=false,lateFault=false,deliverPending=false;
    int badRuntime=-1,badRead=-1,badFence=-1,scopeLossAfter=-1;
    int changeOnConfirm=-1;
    unsigned reads=0,runtimeChecks=0,fences=0,checks=0,selected=0,pauses=0;
    unsigned coreReads[DataflowCoreCount]{};
    DataflowResidentCommand request{};
    explicit FleetIO(const DataflowResidentCommand& c) {
        request=c;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            auto& s=sample[core];s.worker.magic=DataflowWorkerMailboxMagic;
            s.worker.version=DataflowWorkerMailboxVersion;s.worker.logicalCore=core;
            s.worker.commandSequence=s.worker.acknowledgedSequence=s.worker.completedSequence=c.trigger;
            s.worker.state=DataflowWorkerReady;s.roles.magic=DataflowRoleMailboxMagic;
            s.roles.version=DataflowRoleMailboxVersion;
            s.roles.started[0]=DataflowRoleStartedReader;s.roles.started[1]=DataflowRoleStartedWriter;
            s.sender={c.trigger,c.epoch,core,DataflowResidentSenderReceiptChecksum(c.trigger,c.epoch,core)};
        }
        if (c.count==DataflowGraphDecodeSteps) {
            auto& b=sample[0].worker;const unsigned token=42,score=0x3f800000,sequence=c.firstSequence+c.count-1;
            b.reserved[0]=token;b.reserved[1]=score;b.reserved[2]=sequence;b.reserved[3]=DataflowArgmaxResultMagic;
            b.reserved[4]=~token;b.reserved[5]=DataflowArgmaxResultChecksum(token,score,sequence);
        }
    }
    bool ResidentAuditAllowed(const DataflowResidentCommand&) {
        ++checks;return allowed && (scopeLossAfter<0 || int(checks)<=scopeLossAfter);
    }
    bool BeginResidentAudit(const DataflowResidentCommand&) { return begin; }
    bool FenceResidentAudit(const DataflowResidentCommand&) {
        ++fences;if (mutate && fences==2) sample[37].roles.reserved[0]=1;
        return int(fences)!=badFence;
    }
    bool AuditResidentCoreRuntime(unsigned core,const DataflowResidentCommand&) {
        ++runtimeChecks;selected=core;return int(core)!=badRuntime;
    }
    bool ReadStableResidentCore(unsigned core,DataflowResidentFleetSample& out) {
        ++reads;if (int(core)==badRead) return false;
        if (++coreReads[core]==2 && int(core)==changeOnConfirm) sample[core].sender.checksum^=1;
        memcpy(&out,&sample[core],sizeof(out));return true;
    }
    bool EndResidentAudit(const DataflowResidentCommand&) {
        if (!end) return false;
        if (lateFault) sample[37].worker.state=DataflowWorkerFatal;
        selected=0;return true;
    }
    void PauseResidentAudit() {
        ++pauses;
        if (deliverPending) {
            sample[37].sender={request.trigger,request.epoch,37,
                DataflowResidentSenderReceiptChecksum(request.trigger,request.epoch,37)};
            deliverPending=false;
        }
    }
    void RetainResidentAudit() { retained=true; }
};
int main() {
    {
        DataflowResidentCommand invalid{};invalid.magic=DataflowResidentCommandMagic;
        invalid.version=DataflowResidentCommandVersion;invalid.trigger=DataflowResidentRunLimit+1;
        invalid.epoch=1;invalid.firstSequence=1;invalid.count=DataflowGraphDecodeSteps;
        invalid.pollLimit=1;invalid.checksum=DataflowResidentCommandChecksum(invalid);
        assert(DataflowResidentCommandValid(invalid));
        auto audit=std::make_unique<DataflowResidentFleetAudit>();auto io=std::make_unique<FleetIO>(invalid);
        assert(audit->Audit(invalid,*io)==DataflowResidentFleetAuditInvalid && !io->reads);
    }
    for (unsigned scenario=0;scenario<20;scenario++) {
        DataflowResidentCommand c{};c.magic=DataflowResidentCommandMagic;c.version=DataflowResidentCommandVersion;
        c.trigger=2;c.epoch=7;c.firstSequence=1000;c.count=scenario==1?DataflowGraphPromptSteps:DataflowGraphDecodeSteps;
        c.pollLimit=100;c.inputSlot=0;c.token=42;c.position=1;c.checksum=DataflowResidentCommandChecksum(c);
        auto audit=std::make_unique<DataflowResidentFleetAudit>();auto io=std::make_unique<FleetIO>(c);
        if (scenario==2) io->sample[37].worker.state=DataflowWorkerFatal;
        if (scenario==3) io->mutate=true;
        if (scenario==4) io->badRuntime=37;
        if (scenario==5) io->badRead=37;
        if (scenario==6) io->end=false;
        if (scenario==7) io->sample[0].worker.reserved[5]^=1;
        if (scenario==8) io->begin=false;
        if (scenario==9) io->badFence=2;
        if (scenario==10) io->scopeLossAfter=20;
        if (scenario==11) io->sample[37].worker.reserved[0]=7;
        if (scenario==12) io->sample[37].queues[0].produced=1;
        if (scenario==13) io->sample[37].roles.status[0]=DataflowRoleFailed;
        if (scenario==14) io->allowed=false;
        if (scenario==15) io->lateFault=true; // fake IO falsely claims physical exclusion
        if (scenario==16) { io->sample[37].sender.trigger=0;io->deliverPending=true; }
        if (scenario==17) io->sample[37].sender.trigger=0;
        if (scenario==18) io->sample[37].sender.checksum^=1;
        if (scenario==19) io->changeOnConfirm=37;
        const auto result=audit->Audit(c,*io);
        if (scenario<2) {
            assert(result==DataflowResidentFleetAuditComplete && audit->AuditedFor(c)
                && !audit->Retained() && io->reads==4*DataflowCoreCount && io->selected==0);
            auto changed=c;changed.token++;changed.checksum=DataflowResidentCommandChecksum(changed);
            assert(!audit->AuditedFor(changed));
            assert(audit->Audit(c,*io)==DataflowResidentFleetAuditInvalid);
        } else if (scenario==14) {
            assert(result==DataflowResidentFleetAuditPreflight && !audit->Retained() && !io->reads);
        } else if (scenario==15) {
            // Two read passes alone cannot catch a sender that fails after
            // its sample. A real IO must prove that this cannot occur.
            assert(result==DataflowResidentFleetAuditComplete && audit->AuditedFor(c)
                && io->sample[37].worker.state==DataflowWorkerFatal);
        } else if (scenario==16) {
            assert(result==DataflowResidentFleetAuditComplete && audit->AuditedFor(c) && io->pauses==1);
        } else {
            assert(result==DataflowResidentFleetAuditRetained && audit->Retained() && io->retained
                && !audit->AuditedFor(c));
            assert(audit->Audit(c,*io)==DataflowResidentFleetAuditInvalid);
            if (scenario==2 || scenario==3 || scenario==4 || scenario==5 || scenario==11
                || scenario==12 || scenario==13 || scenario==17 || scenario==18 || scenario==19)
                assert(audit->FailedCore()==37);
            if (scenario==8) assert(!io->reads);
        }
    }
    puts("PASS two-pass 120-core resident candidate audit, output and fault retention; SIMULATED ONLY");
}
