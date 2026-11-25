/* SPDX-License-Identifier: MIT */
// Actual qualification adapter and service owner over simulated fleet IO.
#define main q35_service_owner_unused_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main
#include "../PioneerQwen35DataflowProjectionQualificationService.h"
#include "../PioneerQwen35DataflowActivationReuse.h"
#include <memory>

struct SharingIO : MockDataflowIO {
    uint8_t packets[DataflowCoreCount][2*DataflowActivationSharePacketBytes]{};
    bool WriteCoreMemory(unsigned core,unsigned address,const void* data,unsigned bytes) {
        if (core<DataflowCoreCount && held[core] && bytes==DataflowActivationSharePacketBytes
            && (address==DataflowActivationShareReadyAddress || address==DataflowActivationShareScratchAddress)) {
            std::memcpy(packets[core]+(address-DataflowActivationShareReadyAddress),data,bytes);return true;
        }
        return MockDataflowIO::WriteCoreMemory(core,address,data,bytes);
    }
    bool ReadCoreMemory(unsigned core,unsigned address,void* data,unsigned bytes) {
        if (core<DataflowCoreCount && held[core] && bytes==DataflowActivationSharePacketBytes
            && (address==DataflowActivationShareReadyAddress || address==DataflowActivationShareScratchAddress)) {
            std::memcpy(data,packets[core]+(address-DataflowActivationShareReadyAddress),bytes);return true;
        }
        return MockDataflowIO::ReadCoreMemory(core,address,data,bytes);
    }
};
struct Scope;
struct Scratch {
    bool staged=true,running=false;
    bool IsStaged() const { return staged; }
    bool MatchesStagedLease(unsigned bank,unsigned address,unsigned bytes) const {
        return staged && bank==0 && address==BankBase && bytes==DataflowProjectionQualification::ScratchBytes;
    }
    bool MarkRunning(Scope&);
};
struct Scope {
    Scratch* scratch;SharingIO* io;bool retained=false;
    bool QualificationLeaseMatches(const Scratch& s,unsigned bank,unsigned address,unsigned bytes,
        const unsigned (&routes)[2][Banks]) const {
        return &s==scratch && bank==0 && address==BankBase && bytes==DataflowProjectionQualification::ScratchBytes
            && DataflowBankRoutesValid(routes);
    }
    bool QualificationExecutionSafe(const Scratch& s,const DataflowProjectionProgram& p) const {
        return &s==scratch && (s.staged || s.running) && p.Valid();
    }
    bool QualificationProjectionInFlightSafe(const Scratch& s,const DataflowProjectionProgram& p) const {
        return &s==scratch && s.running && p.Valid();
    }
    bool QualificationRestoreRuntimeRoute(const Scratch& s,const DataflowProjectionProgram& p,SharingIO& value) const {
        return &s==scratch && &value==io && p.Valid();
    }
    bool AuditQualification(SharingIO& value,const DataflowProjectionProgram& p,unsigned sequence,
        const DataflowWorkerSnapshot (&receipts)[DataflowCoreCount]) const {
        if (&value!=io || !value.AuditProjection(p,sequence,receipts)) return false;
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            DataflowCoreInvocation original{},expected{};
            if (!DataflowProgramBuilder::BindCore(p,core,original)
                || !DataflowQualificationBindSharing(p,core,original,expected)
                || std::memcmp(&expected,&value.invocation[core],sizeof(expected))) return false;
        }
        return true;
    }
    void RetainQualification(const void*,const void*) { retained=true; }
};
bool Scratch::MarkRunning(Scope& scope) {
    if (!staged || running || scope.scratch!=this) return false;
    staged=false;running=true;return true;
}
int main()
{
    auto io=std::make_unique<SharingIO>();auto service=std::make_unique<DataflowServiceOwner>();
    auto qualification=std::make_unique<DataflowProjectionQualificationService>();
    Scratch scratch;Scope scope{&scratch,io.get()};
    uint32_t code[96]{},dispatch[80]{};
    for (unsigned i=0;i<96;i++) code[i]=0x10000000U+i;
    for (unsigned i=0;i<80;i++) dispatch[i]=0x20000000U+i;
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(service->Install(image,*io)==DataflowServiceComplete);
    assert(qualification->Bind(scratch,scope,0,BankBase,TestRoutes));
    const auto& program=*qualification->Program();
    DataflowCoreInvocation original{},shared{};
    assert(DataflowProgramBuilder::BindCore(program,3,original) && !DataflowCoreHeaderValid(original));
    assert(DataflowQualificationBindSharing(program,3,original,shared) && DataflowCoreHeaderValid(shared));
    for (unsigned i=0;i<DataflowCoreCount;i++) assert(shared.sharing.members[i/32]&(uint32_t(1)<<(i%32)));
    auto bad=original;bad.inputBase^=16;
    assert(!DataflowQualificationBindSharing(program,3,bad,shared));
    assert(qualification->Run(*service,*io,scratch,scope)==DataflowServiceComplete
        && qualification->IsAudited() && !scope.retained);
    assert(service->Close(*io)==DataflowServiceClosed);
    DataflowActivationReuse reuse{};
    assert(!DataflowActivationReuseSetStandalone(reuse,0));
    assert(DataflowActivationReuseSetStandalone(reuse,42));
    assert(reuse.epoch==42 && reuse.step==DataflowActivationReuseStandaloneStep && !reuse.magic);
    assert(DataflowQualificationBindSharing(program,3,original,shared));
    DataflowActivationSharePacket packet{};
    assert(DataflowActivationSharePacketBuild(shared,42,DataflowActivationReuseStandaloneStep,0,packet));
    assert(DataflowActivationSharePacketMatches(packet,shared,42,DataflowActivationReuseStandaloneStep,0));
    assert(!DataflowActivationSharePacketMatches(packet,shared,42,1,0));
    assert(!DataflowActivationSharePacketMatches(packet,shared,43,DataflowActivationReuseStandaloneStep,0));
    assert(!DataflowActivationReuseSetStandalone(reuse,DataflowWorkerSequenceLimit+1U));
    std::puts("PASS fixed projection stages sharing on 120 workers with standalone epoch; SIMULATED ONLY");
}
