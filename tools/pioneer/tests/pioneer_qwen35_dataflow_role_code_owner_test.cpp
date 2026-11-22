/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleCodeOwner.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;

struct SimulatedIO {
    std::vector<uint32_t> memory;
    bool held=true,unstable=false;
    int failWrite=-1,releaseOnWrite=-1,corruptFirstOnWrite=-1;
    unsigned reads=0,writes=0;
    SimulatedIO():memory(DataflowRoleCodeOwner::RequiredWords()) {
        for (unsigned i=0;i<memory.size();i++) memory[i]=i^0xa5a55a5aU;
    }
    bool AllRolesHeld(unsigned core) const { return held && core==4; }
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
    assert(owner.Stage(false,writer.data(),writer.size(),io));
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
    // If a role starts after an untouched capture, the snapshot is no
    // longer trusted; a fresh capture is required before any code write.
    SimulatedIO staleIO;
    DataflowRoleCodeOwner stale;
    assert(stale.Capture(4,saved.data(),saved.size(),staleIO));
    staleIO.held=false;
    assert(!stale.Stage(true,reader.data(),reader.size(),staleIO));
    staleIO.held=true;
    assert(!stale.Stage(true,reader.data(),reader.size(),staleIO));
    assert(stale.Capture(4,saved.data(),saved.size(),staleIO));
    assert(stale.Stage(true,reader.data(),reader.size(),staleIO));
    assert(stale.Restore(staleIO) && staleIO.memory==original);
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
