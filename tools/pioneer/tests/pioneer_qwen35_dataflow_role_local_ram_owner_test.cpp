/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleLocalRamOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct RamIO {
    std::vector<uint32_t> ram;
    bool held=true,stable=true,corruptEnabled=false;
    int failWrite=-1,corruptAt=-1;
    unsigned writes=0,reads=0;
    RamIO():ram(DataflowRoleLocalRamOwner::RequiredWords()) {
        for (unsigned i=0;i<ram.size();i++) ram[i]=i^0xabc13579U;
    }
    bool AllRolesHeld(unsigned core) const { return held && core==9; }
    bool LocalRamStable(unsigned core) const { return core==9 && stable; }
    bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* output,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && output && DataflowRoleLocalRamRange(role,address,bytes));
        ++reads;
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned offset=role*DataflowRoleLocalRamWords+(address-base)/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=ram[offset+i];
        return true;
    }
    bool WriteRam(unsigned core,unsigned role,uint32_t address,const uint32_t* input,unsigned bytes) {
        assert(AllRolesHeld(core) && LocalRamStable(core) && input && DataflowRoleLocalRamRange(role,address,bytes));
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        const unsigned offset=role*DataflowRoleLocalRamWords+(address-base)/4;
        ++writes;
        if (int(writes)==failWrite) { ram[offset]=input[0]; return false; }
        for (unsigned i=0;i<bytes/4;i++) ram[offset+i]=input[i];
        if (corruptEnabled && int(writes)==corruptAt) { ram[0]^=1; corruptEnabled=false; }
        return true;
    }
};
struct StacklessRamIO : RamIO {
    std::vector<uint32_t> copies=ram;
    bool safe=true;
    unsigned copyReads=0,failCopyAt=UINT32_MAX,corruptAtRead=UINT32_MAX;
    mutable unsigned safeChecks=0,failSafeAt=UINT32_MAX;
    bool restoredProof=false;
    mutable unsigned restoreProofChecks=0,failRestoreProofAt=UINT32_MAX;
    const uint32_t* trustedReader=nullptr;const uint32_t* trustedWriter=nullptr;
    bool StacklessCopiesSafe(unsigned core) const {
        return core==9 && safe && safeChecks++!=failSafeAt;
    }
    bool ReadStacklessRamCopy(unsigned core,unsigned role,unsigned offset,uint32_t* output,unsigned bytes) {
        if (core!=9 || role>=DataflowRoleCount || !output || !bytes || (offset&3) || (bytes&3)
            || offset>DataflowRoleLocalRamBytes || bytes>DataflowRoleLocalRamBytes-offset) return false;
        const unsigned call=copyReads++;
        if (call==failCopyAt) return false;
        if (call==corruptAtRead) copies[0]^=1;
        const unsigned first=role*DataflowRoleLocalRamWords+offset/4;
        for (unsigned i=0;i<bytes/4;i++) output[i]=copies[first+i];
        return true;
    }
    bool StacklessCoreRestoreProven(unsigned core,const uint32_t* reader,const uint32_t* writer) const {
        if (!restoredProof || restoreProofChecks++==failRestoreProofAt || !AllRolesHeld(core)
            || reader!=trustedReader || writer!=trustedWriter) return false;
        for (unsigned i=0;i<DataflowRoleLocalRamWords;i++)
            if (ram[i]!=reader[i] || ram[DataflowRoleLocalRamWords+i]!=writer[i]) return false;
        return true;
    }
};
int main()
{
    assert(DataflowRoleLocalRamRange(DataflowRoleReader,DataflowRoleBriscLocalRam,DataflowRoleLocalRamBytes));
    assert(DataflowRoleLocalRamRange(DataflowRoleWriter,DataflowRoleNcriscLocalRam,DataflowRoleLocalRamBytes));
    assert(!DataflowRoleLocalRamRange(DataflowRoleReader,DataflowRoleNcriscLocalRam,4));
    assert(!DataflowRoleLocalRamRange(DataflowRoleWriter,DataflowRoleNcriscLocalRam+DataflowRoleLocalRamBytes-4,8));
    assert(!DataflowRoleLocalRamRange(DataflowRoleWriter,DataflowRoleNcriscLocalRam,0xffffffffU));
    std::vector<uint32_t> snapshot(DataflowRoleLocalRamOwner::RequiredWords());
    {
        RamIO io; const auto original=io.ram; DataflowRoleLocalRamOwner owner;
        io.stable=false; assert(!owner.Capture(9,snapshot.data(),snapshot.size(),io));
        io.stable=true; assert(owner.Capture(9,snapshot.data(),snapshot.size(),io));
        assert(owner.MarkMayRun(io));
        io.ram.assign(io.ram.size(),0); // simulated reset clearing both RAMs
        io.stable=false; assert(!owner.Restore(io) && !io.writes && owner.RestorationRequired());
        io.stable=true; io.failWrite=2;
        assert(!owner.Restore(io) && owner.RestorationRequired());
        io.failWrite=-1; io.corruptEnabled=true; io.corruptAt=int(io.writes)+2;
        assert(!owner.Restore(io) && owner.RestorationRequired());
        assert(owner.Restore(io) && owner.RestoredOriginal() && io.ram==original);
        io.ram[0]^=1;
        assert(!owner.VerifyOriginal(io) && owner.RestorationRequired());
        assert(owner.Restore(io) && io.ram==original);
    }
    {
        StacklessRamIO io; const auto original=io.ram; DataflowRoleLocalRamOwner owner;
        assert(owner.CaptureStackless(9,snapshot.data(),snapshot.size(),io)
            && owner.MarkMayRunStackless(io));
        io.trustedReader=snapshot.data();io.trustedWriter=snapshot.data()+DataflowRoleLocalRamWords;
        io.ram.assign(io.ram.size(),0);const unsigned reads=io.reads,writes=io.writes;
        assert(!owner.MarkRestoredByCore(io) && owner.RestorationRequired());
        io.restoredProof=true;
        assert(!owner.MarkRestoredByCore(io) && owner.RestorationRequired()); // data not restored
        io.ram=original;io.held=false;
        assert(!owner.MarkRestoredByCore(io) && owner.RestorationRequired());
        io.held=true;io.restoreProofChecks=0;io.failRestoreProofAt=1;
        assert(!owner.MarkRestoredByCore(io) && owner.RestorationRequired());
        io.restoreProofChecks=0;io.failRestoreProofAt=UINT32_MAX;
        assert(owner.MarkRestoredByCore(io) && owner.RestoredOriginal()
            && owner.VerifyRestoredByCore(io));
        assert(owner.AuditOriginal(io)==DataflowRoleLocalRamOwner::OriginalUnreadable
            && io.reads==reads && io.writes==writes);
        io.restoredProof=false;
        assert(!owner.VerifyRestoredByCore(io) && owner.RestorationRequired());
        assert(io.reads==reads && io.writes==writes);
    }
    {
        StacklessRamIO io; const auto original=io.ram; DataflowRoleLocalRamOwner owner;
        io.held=false; io.stable=false;
        assert(!owner.CaptureStackless(9,snapshot.data(),snapshot.size()-1,io));
        io.failCopyAt=1;
        assert(!owner.CaptureStackless(9,snapshot.data(),snapshot.size(),io));
        assert(!owner.OriginalWords(DataflowRoleReader) && !io.reads);
        io.copyReads=0; io.failCopyAt=UINT32_MAX; io.corruptAtRead=2;
        assert(!owner.CaptureStackless(9,snapshot.data(),snapshot.size(),io));
        assert(!owner.OriginalWords(DataflowRoleWriter) && !io.reads);
        io.copies=original; io.copyReads=0; io.corruptAtRead=UINT32_MAX;
        io.safeChecks=0; io.failSafeAt=4;
        assert(!owner.CaptureStackless(9,snapshot.data(),snapshot.size(),io) && !io.reads);
        io.safeChecks=0; io.failSafeAt=UINT32_MAX;
        assert(owner.CaptureStackless(9,snapshot.data(),snapshot.size(),io) && !io.reads);
        assert(snapshot==original && owner.OriginalWords(DataflowRoleReader)==snapshot.data());
        assert(owner.MarkMayRunStackless(io) && owner.RestorationRequired());
        io.ram.assign(io.ram.size(),0);
        io.held=true; io.stable=true;
        assert(owner.Restore(io) && io.ram==original);
    }
    {
        StacklessRamIO io; DataflowRoleLocalRamOwner owner;
        assert(owner.CaptureStackless(9,snapshot.data(),snapshot.size(),io));
        assert(!owner.MarkMayRun(io) && !owner.RestorationRequired());
        io.safe=false;
        assert(!owner.MarkMayRunStackless(io) && owner.RestorationRequired());
        assert(!owner.MarkMayRunStackless(io));
        assert(owner.Restore(io));
    }
    {
        RamIO io; const auto original=io.ram; DataflowRoleLocalRamOwner owner;
        assert(owner.Capture(9,snapshot.data(),snapshot.size(),io));
        io.held=false; assert(!owner.MarkMayRun(io) && owner.RestorationRequired());
        io.held=true; assert(!owner.MarkMayRun(io)); // retain, do not permit release
        assert(owner.Restore(io) && io.ram==original);
    }
    std::puts("PASS private role RAM snapshots, reset-clear retention and verified retry; SIMULATED ONLY");
}
