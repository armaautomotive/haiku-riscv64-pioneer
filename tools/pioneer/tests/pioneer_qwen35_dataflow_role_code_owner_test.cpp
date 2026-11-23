/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleCodeOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;

struct SimulatedIO {
    std::vector<uint32_t> memory;
    bool held=true,unstable=false;
    bool copyProof=false,dropCopyProofAfterFirst=false;
    const uint32_t* trustedReader=nullptr; const uint32_t* trustedWriter=nullptr;
    unsigned copyProofCalls=0;
    int failWrite=-1,releaseOnWrite=-1,corruptFirstOnWrite=-1;
    unsigned reads=0,writes=0;
    SimulatedIO():memory(DataflowRoleCodeOwner::RequiredWords()) {
        for (unsigned i=0;i<memory.size();i++) memory[i]=i^0xa5a55a5aU;
    }
    bool AllRolesHeld(unsigned core) const { return held && core==4; }
    bool StacklessHeldCopySourceSafe(unsigned core,const uint32_t* reader,const uint32_t* writer) {
        ++copyProofCalls;
        return AllRolesHeld(core) && copyProof && reader==trustedReader && writer==trustedWriter
            && !(dropCopyProofAfterFirst && copyProofCalls>1);
    }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        assert(AllRolesHeld(core) && out && DataflowRoleExtraL1Range(address,bytes));
        const unsigned offset=(address-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<bytes/4;i++) out[i]=memory[offset+i];
        if (unstable && (reads++%2)) out[0]^=1;
        return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        assert(AllRolesHeld(core) && in && DataflowRoleExtraL1Range(address,bytes));
        const unsigned offset=(address-DataflowRoleExtraL1Address)/4;
        ++writes;
        if (failWrite>=0 && int(writes)==failWrite) {
            memory[offset]=in[0]; // uncertain partial hardware write
            return false;
        }
        for (unsigned i=0;i<bytes/4;i++) memory[offset+i]=in[i];
        if (corruptFirstOnWrite>=0 && int(writes)==corruptFirstOnWrite) memory[0]^=1;
        if (releaseOnWrite>=0 && int(writes)==releaseOnWrite) held=false;
        return true;
    }
};

int main() {
    SimulatedIO io;
    const auto original=io.memory;
    std::vector<uint32_t> saved(DataflowRoleCodeOwner::RequiredWords());
    std::vector<uint32_t> reader(100,0x12345678U),writer(70,0xabcdef01U);
    DataflowRoleCodeOwner owner;
    assert(!owner.Capture(3,saved.data(),saved.size(),io));
    assert(!owner.Capture(4,saved.data(),saved.size()-1,io));
    io.unstable=true;
    assert(!owner.Capture(4,saved.data(),saved.size(),io));
    io.unstable=false;
    assert(owner.Capture(4,saved.data(),saved.size(),io));
    assert(owner.CapturedSnapshot() && !owner.NeedsRestore());
    assert(!owner.Stage(true,reader.data(),(DataflowRoleReaderCodeLimit-DataflowRoleReaderCodeAddress)/4+1,io));
    assert(!owner.NeedsRestore());
    assert(owner.Stage(true,reader.data(),reader.size(),io));
    assert(!owner.VerifyStaged(io) && !owner.CodeVerified());
    assert(owner.Stage(false,writer.data(),writer.size(),io));
    assert(owner.VerifyStaged(io) && owner.CodeVerified());
    assert(owner.NeedsRestore());
    io.held=false;
    const unsigned writes=io.writes;
    assert(!owner.Restore(io) && io.writes==writes && owner.NeedsRestore());
    io.held=true;
    assert(!owner.Stage(false,writer.data(),writer.size(),io));
    io.failWrite=int(io.writes)+2;
    assert(!owner.Restore(io) && owner.NeedsRestore() && !owner.RegionRestored());
    assert(!owner.Stage(false,writer.data(),writer.size(),io));
    io.failWrite=-1;
    // A later transfer corrupts an earlier verified chunk; the final full
    // verification must retain the snapshot so recovery can be retried.
    io.corruptFirstOnWrite=int(io.writes)+2;
    assert(!owner.Restore(io) && owner.NeedsRestore());
    io.corruptFirstOnWrite=-1;
    assert(owner.Restore(io) && owner.RegionRestored());
    assert(io.memory==original);
    assert(!owner.Stage(true,reader.data(),reader.size(),io));
    assert(!owner.Restore(io));
    // If the hold proof fails after capture, preserve the snapshot for
    // recovery and forbid a new stage until the original is restored.
    SimulatedIO staleIO;
    DataflowRoleCodeOwner stale;
    assert(stale.Capture(4,saved.data(),saved.size(),staleIO));
    staleIO.held=false;
    assert(!stale.Stage(true,reader.data(),reader.size(),staleIO));
    assert(stale.NeedsRestore() && !stale.Restore(staleIO));
    staleIO.held=true;
    assert(!stale.Stage(true,reader.data(),reader.size(),staleIO));
    assert(stale.Restore(staleIO) && staleIO.memory==original);
    SimulatedIO damagedIO;
    DataflowRoleCodeOwner damaged;
    assert(damaged.Capture(4,saved.data(),saved.size(),damagedIO));
    assert(damaged.Stage(true,reader.data(),reader.size(),damagedIO));
    assert(damaged.Stage(false,writer.data(),writer.size(),damagedIO));
    damagedIO.memory[0]^=1;
    assert(!damaged.VerifyStaged(damagedIO) && damaged.NeedsRestore());
    assert(!damaged.Stage(true,reader.data(),reader.size(),damagedIO));
    assert(damaged.Restore(damagedIO) && damagedIO.memory==original);
    SimulatedIO holeIO;
    DataflowRoleCodeOwner hole;
    assert(hole.Capture(4,saved.data(),saved.size(),holeIO));
    assert(hole.Stage(true,reader.data(),reader.size(),holeIO));
    assert(hole.Stage(false,writer.data(),writer.size(),holeIO));
    holeIO.memory[reader.size()+5]^=1; // outside both staged code images
    assert(!hole.VerifyStaged(holeIO) && hole.NeedsRestore());
    assert(hole.Restore(holeIO) && holeIO.memory==original);
    SimulatedIO copyIO;
    DataflowRoleCodeOwner copy;
    assert(copy.Capture(4,saved.data(),saved.size(),copyIO));
    assert(copy.Stage(true,reader.data(),reader.size(),copyIO));
    assert(copy.Stage(false,writer.data(),writer.size(),copyIO));
    copyIO.memory[(DataflowRoleWriterRamCopy-DataflowRoleExtraL1Address)/4]^=1;
    assert(!copy.VerifyStaged(copyIO) && copy.NeedsRestore());
    assert(copy.Restore(copyIO) && copyIO.memory==original);
    // A separate audit can accept exact retained core-produced copies only
    // under an independent, lifetime-spanning source/route proof. It must
    // reject drift and must not certify the ordinary original-L1 audit.
    for (unsigned scenario=0;scenario<4;scenario++) {
        SimulatedIO ownedIO; DataflowRoleCodeOwner owned;
        std::vector<uint32_t> originalL1(DataflowRoleCodeOwner::RequiredWords());
        std::vector<uint32_t> readerCopy(DataflowRoleRamCopyBytes/4,0x10203040U);
        std::vector<uint32_t> writerCopy(DataflowRoleRamCopyBytes/4,0x50607080U);
        assert(owned.Capture(4,originalL1.data(),originalL1.size(),ownedIO));
        assert(owned.Stage(true,reader.data(),reader.size(),ownedIO)
            && owned.Stage(false,writer.data(),writer.size(),ownedIO) && owned.VerifyStaged(ownedIO));
        assert(owned.BeginCopyAwareOwnership() && owned.CopyAwareOwnership()
            && !owned.BeginCopyAwareOwnership());
        const unsigned readerAt=(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4;
        const unsigned writerAt=(DataflowRoleWriterRamCopy-DataflowRoleExtraL1Address)/4;
        for (unsigned i=0;i<readerCopy.size();i++) ownedIO.memory[readerAt+i]=readerCopy[i];
        for (unsigned i=0;i<writerCopy.size();i++) ownedIO.memory[writerAt+i]=writerCopy[i];
        ownedIO.trustedReader=readerCopy.data(); ownedIO.trustedWriter=writerCopy.data();
        ownedIO.copyProof=scenario!=1;
        if (scenario==2) ownedIO.memory[writerAt+5]^=1;
        if (scenario==3) ownedIO.dropCopyProofAfterFirst=true;
        const unsigned before=ownedIO.writes;
        assert(owned.VerifyStagedWithCopies(readerCopy.data(),writerCopy.data(),ownedIO)==(scenario==0));
        assert(ownedIO.writes==before && !owned.CodeVerified() && owned.NeedsRestore());
        if (scenario==0) assert(!owned.VerifyStaged(ownedIO)); // ordinary audit still detects the copy
        assert(!owned.Restore(ownedIO) && ownedIO.writes==before);
    }
    SimulatedIO failedIO;
    DataflowRoleCodeOwner failed;
    assert(failed.Capture(4,saved.data(),saved.size(),failedIO));
    failedIO.failWrite=1;
    assert(!failed.Stage(true,reader.data(),reader.size(),failedIO));
    assert(failed.NeedsRestore());
    assert(!failed.Stage(false,writer.data(),writer.size(),failedIO));
    failedIO.failWrite=-1;
    assert(failed.Restore(failedIO));
    assert(failedIO.memory==original);
    SimulatedIO releasedIO;
    DataflowRoleCodeOwner released;
    assert(released.Capture(4,saved.data(),saved.size(),releasedIO));
    releasedIO.releaseOnWrite=1;
    assert(!released.Stage(true,reader.data(),reader.size(),releasedIO));
    assert(released.NeedsRestore() && !released.RegionRestored());
    assert(!released.Restore(releasedIO));
    releasedIO.held=true;
    releasedIO.releaseOnWrite=-1;
    assert(released.Restore(releasedIO) && releasedIO.memory==original);
    std::puts("PASS role code snapshot, partial-write retention and verified restoration; SIMULATED ONLY");
}
