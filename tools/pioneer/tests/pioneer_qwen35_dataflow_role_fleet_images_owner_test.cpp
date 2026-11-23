/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleFleetImagesOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct ImagesIO {
    static constexpr unsigned PerCore=DataflowRoleCodeOwner::RequiredWords();
    std::vector<uint32_t> code,boots,shims;
    bool held=true,defaultRoute=true,corruptCodeOnBoot=false,corruptBootOnCode=false;
    unsigned writes=0;
    ImagesIO():code(DataflowRoleFleetImagesOwner::RequiredWords()),
        boots(DataflowBarrierWorkers*2),shims(DataflowBarrierWorkers*DataflowRoleShimWords) {
        for (unsigned i=0;i<code.size();i++) code[i]=i^0x1234abcdU;
        for (unsigned i=0;i<boots.size();i++) boots[i]=i^0x5678abcdU;
        for (unsigned i=0;i<shims.size();i++) shims[i]=i^0x9abc1234U;
    }
    bool AllRolesHeld(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    bool DefaultBootRouting(unsigned core) const { return defaultRoute && core<DataflowBarrierWorkers; }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && out && DataflowRoleExtraL1Range(address,bytes));
        const unsigned at=core*PerCore+(address-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=code[at+i];
        return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && in && DataflowRoleExtraL1Range(address,bytes));
        const unsigned at=core*PerCore+(address-DataflowRoleExtraL1Address)/4;
        ++writes; for (unsigned i=0;i<bytes/4;i++) code[at+i]=in[i];
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
        return true;
    }
};
int main()
{
    std::vector<uint32_t> storage(DataflowRoleFleetImagesOwner::RequiredWords());
    const uint32_t codeA[]={1,2,3},codeB[]={4,5},shimA[]={6,7},shimB[]={8,9};
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
        assert(owner.RestoreAll(io) && owner.ImagesRestored());
        assert(io.code==code && io.boots==boots && io.shims==shims);
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
