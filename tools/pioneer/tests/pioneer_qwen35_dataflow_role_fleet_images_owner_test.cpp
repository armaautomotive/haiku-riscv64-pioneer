/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetImagesOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct ImagesIO {
    static constexpr unsigned PerCore=DataflowRoleCodeOwner::RequiredWords();
    std::vector<uint32_t> code,boots,shims;
    std::vector<uint32_t> retainedCopies;
    bool copyCaptured[DataflowBarrierWorkers]{};
    bool copyProof=true,corruptCapturedOnShimWrite=false;
    bool retirementProof=false,retirementRetry=false;
    int failCodeCore=-1;
    bool held=true,defaultRoute=true,corruptCodeOnBoot=false,corruptBootOnCode=false;
    bool corruptShimOnWrite=false,corruptCodeOnShimWrite=false;
    unsigned writes=0;
    ImagesIO():code(DataflowRoleFleetImagesOwner::RequiredWords()),
        boots(DataflowBarrierWorkers*2),shims(DataflowBarrierWorkers*DataflowRoleShimWords),
        retainedCopies(DataflowBarrierWorkers*2*(DataflowRoleRamCopyBytes/4)) {
        for (unsigned i=0;i<code.size();i++) code[i]=i^0x1234abcdU;
        for (unsigned i=0;i<boots.size();i++) boots[i]=i^0x5678abcdU;
        for (unsigned i=0;i<shims.size();i++) shims[i]=i^0x9abc1234U;
    }
    bool AllRolesHeld(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    bool DefaultBootRouting(unsigned core) const { return defaultRoute && core<DataflowBarrierWorkers; }
    void InstallCapturedCopies(unsigned core) {
        assert(core<DataflowBarrierWorkers);
        const unsigned words=DataflowRoleRamCopyBytes/4,base=core*2*words;
        const unsigned readerAt=core*PerCore+(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4;
        const unsigned writerAt=core*PerCore+(DataflowRoleWriterRamCopy-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<words;i++) {
            retainedCopies[base+i]=0x11110000U+i;
            retainedCopies[base+words+i]=0x22220000U+i;
            code[readerAt+i]=retainedCopies[base+i];code[writerAt+i]=retainedCopies[base+words+i];
        }
        copyCaptured[core]=true;
    }
    bool StacklessHeldCopySources(unsigned core,const uint32_t*& reader,const uint32_t*& writer,bool& captured) {
        if (!copyProof || !AllRolesHeld(core)) return false;
        captured=copyCaptured[core];reader=writer=nullptr;
        if (captured) {
            const unsigned words=DataflowRoleRamCopyBytes/4;
            reader=retainedCopies.data()+core*2*words; writer=reader+words;
        }
        return true;
    }
    bool StacklessHeldCopySourceSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        const uint32_t* expectedReader=nullptr;const uint32_t* expectedWriter=nullptr;bool captured=false;
        return StacklessHeldCopySources(core,expectedReader,expectedWriter,captured) && captured
            && reader==expectedReader && writer==expectedWriter;
    }
    bool RetirementRamSnapshotSources(unsigned core,const uint32_t*& reader,const uint32_t*& writer,bool& captured) {
        return StacklessHeldCopySources(core,reader,writer,captured);
    }
    bool FleetRamImageRetirementSafe() { return held && defaultRoute && retirementProof; }
    bool FleetRamImageRetirementRetrySafe() { return FleetRamImageRetirementSafe() && retirementRetry; }
    bool CoreRamImageRetirementSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        const uint32_t* r=nullptr;const uint32_t* w=nullptr;bool captured=false;
        return FleetRamImageRetirementSafe() && RetirementRamSnapshotSources(core,r,w,captured)
            && captured && reader==r && writer==w;
    }
    bool CoreRamImageRetirementRetrySafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        return retirementRetry && CoreRamImageRetirementSafe(core,reader,writer);
    }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && out && DataflowRoleExtraL1Range(address,bytes));
        const unsigned at=core*PerCore+(address-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=code[at+i];
        return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && in && DataflowRoleExtraL1Range(address,bytes));
        const unsigned at=core*PerCore+(address-DataflowRoleExtraL1Address)/4;
        ++writes;
        if (int(core)==failCodeCore) { code[at]=in[0];failCodeCore=-1;return false; }
        for (unsigned i=0;i<bytes/4;i++) code[at+i]=in[i];
        if (corruptBootOnCode && core==3) { boots[0]^=1; corruptBootOnCode=false; }
        return true;
    }
    bool ReadBoot(unsigned core,unsigned address,uint32_t& value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        const unsigned role=address==DataflowRoleBriscBootAddress?0:address==DataflowRoleNcriscBootAddress?1:2;
        assert(role<2); value=boots[core*2+role]; return true;
    }
    bool WriteBoot(unsigned core,unsigned address,uint32_t value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        const unsigned role=address==DataflowRoleBriscBootAddress?0:address==DataflowRoleNcriscBootAddress?1:2;
        assert(role<2); ++writes; boots[core*2+role]=value;
        if (corruptCodeOnBoot && core==3) { code[119*PerCore]^=1; corruptCodeOnBoot=false; }
        return true;
    }
    bool ReadShim(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && out && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned at=core*DataflowRoleShimWords+(address-DataflowRoleReaderShimAddress)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=shims[at+i];
        return true;
    }
    bool WriteShim(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && in && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned at=core*DataflowRoleShimWords+(address-DataflowRoleReaderShimAddress)/4;
        ++writes;
        for (unsigned i=0;i<bytes/4;i++) shims[at+i]=in[i];
        if (core==3 && corruptShimOnWrite) { shims[119*DataflowRoleShimWords]^=1; corruptShimOnWrite=false; }
        if (core==3 && corruptCodeOnShimWrite) { code[119*PerCore]^=1; corruptCodeOnShimWrite=false; }
        if (core==3 && corruptCapturedOnShimWrite) {
            code[core*PerCore+(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4]^=1;
            corruptCapturedOnShimWrite=false;
        }
        return true;
    }
};
#ifndef PIONEER_Q35_IMAGES_FIXTURE_ONLY
int main()
{
    std::vector<uint32_t> storage(DataflowRoleFleetImagesOwner::RequiredWords());
    const uint32_t codeA[]={1,2,3},codeB[]={4,5},shimA[]={6,7},shimB[]={8,9};
    {
        ImagesIO io;const auto code=io.code,boots=io.boots,shims=io.shims;
        DataflowRoleFleetImagesOwner owner;
        assert(owner.CaptureAll(storage.data(),storage.size(),io)
            && owner.StageAll(codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.BeginCopyAwareOwnership());
        io.InstallCapturedCopies(3);io.InstallCapturedCopies(119);
        assert(owner.VerifyStagedAllWithCopies(io));
        const unsigned before=io.writes;
        assert(!owner.RestoreAfterFleetRamProof(io) && io.writes==before);
        io.retirementProof=true;io.failCodeCore=3;
        assert(!owner.RestoreAfterFleetRamProof(io) && owner.RestorationRequired());
        const unsigned partial=io.writes;
        assert(!owner.RestoreAll(io) && !owner.RestoreAfterFleetRamProof(io) && io.writes==partial);
        io.retirementRetry=true;
        assert(owner.RestoreAfterFleetRamProof(io) && owner.ImagesRestored()
            && io.code==code && io.boots==boots && io.shims==shims);
    }
    {
        ImagesIO io; const auto code=io.code,boots=io.boots,shims=io.shims;
        DataflowRoleFleetImagesOwner owner;
        assert(owner.CaptureAll(storage.data(),storage.size(),io));
        io.defaultRoute=false;
        assert(!owner.StageAll(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.RestorationRequired() && !owner.RestoreAll(io) && io.writes==0);
        io.defaultRoute=true;
        io.shims[119*DataflowRoleShimWords]^=1; // mismatch must be detected and then repaired
        assert(!owner.RestoreAll(io) && owner.RestorationRequired());
        assert(owner.RestoreAll(io) && owner.ImagesRestored());
        assert(io.code==code && io.boots==boots && io.shims==shims);
    }
    {
        ImagesIO io; const auto code=io.code,boots=io.boots,shims=io.shims;
        DataflowRoleFleetImagesOwner owner;
        assert(owner.CaptureAll(storage.data(),storage.size(),io));
        assert(!owner.StageOne(DataflowBarrierWorkers,codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(!io.writes);
        assert(owner.StageOne(3,codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.StagedAndVerified() && owner.VerifyStagedAll(io) && io.writes<20);
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) if (core!=3) {
            assert(io.boots[core*2]==boots[core*2] && io.boots[core*2+1]==boots[core*2+1]);
            for (unsigned i=0;i<DataflowRoleCodeOwner::RequiredWords();i++)
                assert(io.code[core*ImagesIO::PerCore+i]==code[core*ImagesIO::PerCore+i]);
        }
        const uint32_t replacement[]={0xabc};
        const unsigned writes=io.writes;
        assert(!owner.RestageOne(4,replacement,1,replacement,1,io)
            && owner.StagedAndVerified() && io.writes==writes);
        assert(owner.RestageOne(3,replacement,1,replacement,1,io) && owner.VerifyStagedAll(io));
        assert(owner.RestoreAll(io) && owner.ImagesRestored());
        assert(io.code==code && io.boots==boots && io.shims==shims);
    }
    for (unsigned fault=0;fault<2;fault++) {
        ImagesIO io; const auto code=io.code,boots=io.boots,shims=io.shims;
        DataflowRoleFleetImagesOwner owner; const uint32_t replacement[]={0xdef};
        assert(owner.CaptureAll(storage.data(),storage.size(),io)
            && owner.StageOne(3,codeA,3,codeB,2,shimA,2,shimB,2,io));
        io.corruptShimOnWrite=fault==0; io.corruptCodeOnShimWrite=fault==1;
        assert(!owner.RestageOne(3,replacement,1,replacement,1,io) && owner.RestorationRequired());
        assert(owner.RestoreAll(io) && owner.ImagesRestored());
        assert(io.code==code && io.boots==boots && io.shims==shims);
    }
    {
        // Ordinary restaging still rejects role-produced RAM copies; the
        // separate copy-aware path requires independent source authority.
        ImagesIO io;
        DataflowRoleFleetImagesOwner owner; const uint32_t replacement[]={0xdef};
        assert(owner.CaptureAll(storage.data(),storage.size(),io)
            && owner.StageOne(3,codeA,3,codeB,2,shimA,2,shimB,2,io));
        const unsigned copy=3*ImagesIO::PerCore
            +(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4;
        io.code[copy]^=1; const unsigned writes=io.writes;
        assert(!owner.RestageOne(3,replacement,1,replacement,1,io)
            && owner.RestorationRequired() && io.writes==writes);
    }
    for (unsigned scenario=0;scenario<3;scenario++) {
        ImagesIO io; DataflowRoleFleetImagesOwner owner; const uint32_t replacement[]={0xabc};
        assert(owner.CaptureAll(storage.data(),storage.size(),io)
            && owner.StageOne(3,codeA,3,codeB,2,shimA,2,shimB,2,io));
        const unsigned initially=io.writes;
        assert(owner.BeginCopyAwareOwnership() && !owner.BeginCopyAwareOwnership()
            && !owner.StagedAndVerified() && !owner.CopyAwareStagedAndVerified());
        assert(!owner.RestoreAll(io) && io.writes==initially);
        io.InstallCapturedCopies(3);
        const auto retained=io.retainedCopies;const unsigned before=io.writes;
        if (scenario==1) io.copyProof=false;
        if (scenario==2) io.corruptCapturedOnShimWrite=true;
        const bool okay=owner.RestageOneWithCopies(3,replacement,1,replacement,1,io);
        assert(okay==(scenario==0) && io.retainedCopies==retained);
        if (scenario==0) {
            assert(owner.CopyAwareStagedAndVerified() && !owner.StagedAndVerified());
            const unsigned after=io.writes;
            assert(!owner.VerifyStagedAll(io)
                && !owner.RestageOne(3,replacement,1,replacement,1,io) && io.writes==after);
            assert(owner.VerifyStagedAllWithCopies(io));
        }
        else {
            assert(owner.RestorationRequired());
            if (scenario==1) assert(io.writes==before);
        }
        const unsigned afterAttempt=io.writes;
        assert(!owner.RestoreAll(io) && owner.RestorationRequired()
            && io.writes==afterAttempt && io.retainedCopies==retained);
        // No simulated recovery is claimed: captured private snapshots and
        // their L1 copies remain owned for a future physical restore path.
    }
    {
        // A held tile without retained private-RAM snapshots must never be
        // switched to a restore shim, even if the rest of the fleet is safe.
        ImagesIO io; DataflowRoleFleetImagesOwner owner; const uint32_t replacement[]={0xaaa};
        assert(owner.CaptureAll(storage.data(),storage.size(),io)
            && owner.StageOne(3,codeA,3,codeB,2,shimA,2,shimB,2,io)
            && owner.BeginCopyAwareOwnership());
        const unsigned writes=io.writes;
        assert(!owner.RestageOneWithCopies(3,replacement,1,replacement,1,io)
            && owner.RestorationRequired() && io.writes==writes);
        assert(!owner.RestoreAll(io) && io.writes==writes);
    }
    {
        ImagesIO io; DataflowRoleFleetImagesOwner owner; const uint32_t replacement[]={0xabe};
        assert(owner.CaptureAll(storage.data(),storage.size(),io)
            && owner.StageAll(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.BeginCopyAwareOwnership());
        io.InstallCapturedCopies(3);io.InstallCapturedCopies(119);
        const auto retained=io.retainedCopies;
        assert(owner.RestageOneWithCopies(3,replacement,1,replacement,1,io)
            && owner.VerifyStagedAllWithCopies(io) && io.retainedCopies==retained);
    }
    {
        ImagesIO io; const auto code=io.code,boots=io.boots,shims=io.shims;
        DataflowRoleFleetImagesOwner owner;
        assert(owner.CaptureAll(storage.data(),storage.size(),io));
        io.corruptCodeOnBoot=true; // a core-3 write silently damages core 119
        assert(!owner.StageOne(3,codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.RestorationRequired() && owner.RestoreAll(io) && owner.ImagesRestored());
        assert(io.code==code && io.boots==boots && io.shims==shims);
    }
    {
        ImagesIO io; const auto code=io.code,boots=io.boots,shims=io.shims;
        DataflowRoleFleetImagesOwner owner;
        assert(owner.CaptureAll(storage.data(),storage.size(),io));
        assert(owner.StageAll(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.StagedAndVerified());
        const uint32_t replacement[]={0x456};
        assert(owner.RestageOne(3,replacement,1,replacement,1,io) && owner.VerifyStagedAll(io));
        io.held=false; const unsigned writes=io.writes;
        assert(!owner.RestoreAll(io) && owner.RestorationRequired() && io.writes==writes);
        io.held=true; io.corruptBootOnCode=true;
        assert(!owner.RestoreAll(io) && owner.RestorationRequired());
        assert(owner.RestoreAll(io) && owner.ImagesRestored());
        assert(io.code==code && io.boots==boots && io.shims==shims);
    }
    {
        ImagesIO io; const auto code=io.code,boots=io.boots,shims=io.shims;
        DataflowRoleFleetImagesOwner owner;
        assert(owner.CaptureAll(storage.data(),storage.size(),io));
        io.corruptCodeOnBoot=true;
        assert(!owner.StageAll(codeA,3,codeB,2,shimA,2,shimB,2,io));
        assert(owner.RestorationRequired());
        assert(owner.RestoreAll(io) && owner.ImagesRestored());
        assert(io.code==code && io.boots==boots && io.shims==shims);
    }
    std::puts("PASS 120-core composed code/boot/shim ownership and cross-region faults; SIMULATED ONLY");
}
#endif
