/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleBootOwner.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
struct BootIO {
    std::array<uint32_t,64> shim{};
    uint32_t boot[2]{0x12345678U,0xabcdef12U};
    bool held=true,defaultRoute=true,corruptReaderOnWriter=false;
    int failWrite=-1;
    unsigned writes=0,bootWrites=0;
    BootIO() { for (unsigned i=0;i<shim.size();i++) shim[i]=i^0x99999999U; }
    bool AllRolesHeld(unsigned core) const { return held && core==7; }
    bool DefaultBootRouting(unsigned core) const { return core==7 && defaultRoute; }
    bool ReadBoot(unsigned core,unsigned address,uint32_t& value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        if (address==DataflowRoleBriscBootAddress) value=boot[0];
        else if (address==DataflowRoleNcriscBootAddress) value=boot[1];
        else return false;
        return true;
    }
    bool WriteBoot(unsigned core,unsigned address,uint32_t value) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core));
        const unsigned role=address==DataflowRoleBriscBootAddress?0:address==DataflowRoleNcriscBootAddress?1:2;
        assert(role<2); ++writes; ++bootWrites;
        boot[role]=value;
        return int(writes)!=failWrite;
    }
    bool ReadShim(unsigned core,unsigned address,uint32_t* output,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && output && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned offset=(address-DataflowRoleReaderShimAddress)/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=shim[offset+i];
        return true;
    }
    bool WriteShim(unsigned core,unsigned address,const uint32_t* input,unsigned bytes) {
        assert(AllRolesHeld(core) && DefaultBootRouting(core) && input && !(bytes&3));
        if (address<DataflowRoleReaderShimAddress || address>DataflowRoleWriterShimLimit
            || bytes>DataflowRoleWriterShimLimit-address) return false;
        const unsigned offset=(address-DataflowRoleReaderShimAddress)/4;
        ++writes;
        if (int(writes)==failWrite) { shim[offset]=input[0]; return false; }
        for (unsigned i=0;i<bytes/4;i++) shim[offset+i]=input[i];
        if (corruptReaderOnWriter && address==DataflowRoleWriterShimAddress) {
            shim[0]^=1; corruptReaderOnWriter=false;
        }
        return true;
    }
};
int main()
{
    const uint32_t reader[]={1,2,3,4},writer[]={5,6,7};
    {
        BootIO io; const auto original=io.shim; const uint32_t b0=io.boot[0],b1=io.boot[1];
        DataflowRoleBootOwner owner;
        io.defaultRoute=false; assert(!owner.Capture(7,io));
        io.defaultRoute=true; assert(owner.Capture(7,io));
        assert(owner.Stage(reader,4,writer,3,io) && owner.StagedAndVerified());
        assert(owner.VerifyStaged(io));
        io.defaultRoute=false; const unsigned writes=io.writes;
        assert(!owner.Restore(io) && io.writes==writes && owner.RestorationRequired());
        io.defaultRoute=true;
        assert(owner.Restore(io) && owner.RestoredOriginal());
        assert(io.boot[0]==b0 && io.boot[1]==b1 && io.shim==original);
    }
    {
        BootIO io; const auto original=io.shim; DataflowRoleBootOwner owner;
        assert(owner.Capture(7,io)); io.corruptReaderOnWriter=true;
        assert(!owner.Stage(reader,4,writer,3,io) && owner.RestorationRequired());
        assert(io.bootWrites==0); // detect corrupted shim before changing boot
        assert(owner.Restore(io) && io.shim==original);
    }
    {
        BootIO io; const auto original=io.shim; const uint32_t b0=io.boot[0],b1=io.boot[1];
        DataflowRoleBootOwner owner;
        assert(owner.Capture(7,io)); io.failWrite=4; // uncertain second boot write
        assert(!owner.Stage(reader,4,writer,3,io) && owner.RestorationRequired());
        io.failWrite=-1;
        assert(owner.Restore(io) && io.boot[0]==b0 && io.boot[1]==b1 && io.shim==original);
    }
    {
        BootIO io; const auto original=io.shim; DataflowRoleBootOwner owner;
        assert(owner.Capture(7,io)); assert(owner.Stage(reader,4,writer,3,io));
        io.shim[20]^=1; // untouched shim bytes must also be preserved
        assert(!owner.VerifyStaged(io) && owner.RestorationRequired());
        assert(owner.Restore(io) && io.shim==original);
    }
    std::puts("PASS boot/shim snapshots, partial-write retention and verified restoration; SIMULATED ONLY");
}
