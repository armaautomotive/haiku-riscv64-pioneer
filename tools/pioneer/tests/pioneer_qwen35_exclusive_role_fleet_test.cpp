/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../PioneerQwen35ExclusiveRoleFleet.h"
#include "PioneerQwen35SharedRoleImages.h"
#include "PioneerQwen35ExclusiveRoleImages.h"
using namespace PioneerQwen35;

struct FakeFleet {
    std::vector<uint32_t> code=std::vector<uint32_t>(DataflowBarrierWorkers*0x8000/4,0x12345678);
    std::vector<uint32_t> shim=std::vector<uint32_t>(DataflowBarrierWorkers*DataflowRoleShimWords,0x56789abc);
    std::array<std::array<uint32_t,2>,DataflowBarrierWorkers> boot{};
    unsigned writes=0,bootWrites=0,failAt=UINT32_MAX,dropAt=UINT32_MAX,badCore=UINT32_MAX,corruptAt=UINT32_MAX;
    bool AllRolesHeld(unsigned core) const { return core<DataflowBarrierWorkers && core!=badCore; }
    bool DefaultBootRouting(unsigned core) const { return AllRolesHeld(core); }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(core<DataflowBarrierWorkers && address>=0xb0000 && address+bytes<=0xb8000 && !(bytes&3));
        const unsigned attempt=writes++;
        if (attempt==failAt) return false;
        if (attempt==dropAt) return true;
        std::memcpy(code.data()+core*0x8000/4+(address-0xb0000)/4,in,bytes);return true;
    }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(core<DataflowBarrierWorkers && address>=0xb0000 && address+bytes<=0xb8000 && !(bytes&3));
        std::memcpy(out,code.data()+core*0x8000/4+(address-0xb0000)/4,bytes);return true;
    }
    bool WriteShim(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(core<DataflowBarrierWorkers && address>=0x70000 && address+bytes<=0x70200 && !(bytes&3));
        const unsigned attempt=writes++;
        if (attempt==failAt) return false;
        if (attempt==dropAt) return true;
        std::memcpy(shim.data()+core*DataflowRoleShimWords+(address-0x70000)/4,in,bytes);return true;
    }
    bool ReadShim(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(core<DataflowBarrierWorkers && address>=0x70000 && address+bytes<=0x70200 && !(bytes&3));
        std::memcpy(out,shim.data()+core*DataflowRoleShimWords+(address-0x70000)/4,bytes);return true;
    }
    bool WriteBoot(unsigned core,unsigned address,uint32_t value) {
        assert(core<DataflowBarrierWorkers && (address==DataflowRoleBriscBootAddress || address==DataflowRoleNcriscBootAddress));
        ++bootWrites;
        if (writes++==failAt) return false;
        boot[core][address==DataflowRoleNcriscBootAddress]=value;
        if (writes-1==corruptAt) code[0]^=1; // a late misroute damages an earlier tile
        return true;
    }
    bool ReadBoot(unsigned core,unsigned address,uint32_t& value) {
        assert(core<DataflowBarrierWorkers && (address==DataflowRoleBriscBootAddress || address==DataflowRoleNcriscBootAddress));
        value=boot[core][address==DataflowRoleNcriscBootAddress];return true;
    }
};

static DataflowRoleProgramImage SmallImage() {
    static const uint32_t rshim[]={0x6f,1},wshim[]={0x6f,2},rcode[]={3,4},wcode[]={5,6};
    return {{{2,2},{2,2},{0x70000,0x70100}},{rshim,wshim},{rcode,wcode}};
}
static DataflowRoleProgramImage BuiltImage() {
    return {{{sizeof(DataflowExclusiveReaderShimWords)/4,sizeof(DataflowExclusiveWriterShimWords)/4},
        {sizeof(DataflowSharedReaderCodeWords)/4,sizeof(DataflowSharedWriterCodeWords)/4},
        {DataflowExclusiveReaderParkAddress,DataflowExclusiveWriterParkAddress}},
        {DataflowExclusiveReaderShimWords,DataflowExclusiveWriterShimWords},
        {DataflowSharedReaderCodeWords,DataflowSharedWriterCodeWords}};
}
int main() {
    {
        FakeFleet io;DataflowExclusiveRoleFleet owner;const auto image=BuiltImage();
        assert(image.Valid() && owner.Stage(image,io) && owner.StagedAndVerified() && owner.ResetRequired());
        assert(owner.VerifyStaged(io) && !owner.Stage(image,io));
        io.code[0]^=1;
        assert(!owner.VerifyStaged(io) && !owner.StagedAndVerified() && owner.ResetRequired());
        owner.RetireAfterVerifiedReset();assert(!owner.ResetRequired());
    }
    for (unsigned failure:{0U,1U,239U,240U,479U,480U,719U}) {
        FakeFleet io;io.failAt=failure;DataflowExclusiveRoleFleet owner;const auto image=SmallImage();
        assert(!owner.Stage(image,io) && owner.ResetRequired() && !owner.Stage(image,io));
    }
    {
        FakeFleet io;io.badCore=42;DataflowExclusiveRoleFleet owner;
        assert(!owner.Stage(SmallImage(),io) && owner.Untouched() && !owner.ResetRequired() && !io.writes);
    }
    for (unsigned dropped:{0U,239U,240U,479U}) {
        FakeFleet io;io.dropAt=dropped;DataflowExclusiveRoleFleet owner;
        assert(!owner.Stage(SmallImage(),io) && owner.ResetRequired() && io.writes==480 && !io.bootWrites);
    }
    {
        FakeFleet io;io.corruptAt=719;DataflowExclusiveRoleFleet owner;
        assert(!owner.Stage(SmallImage(),io) && owner.ResetRequired() && io.writes==720);
    }
    std::puts("PASS dedicated 120-core role staging, real images, partial writes and late misroute; SIMULATED ONLY");
}
