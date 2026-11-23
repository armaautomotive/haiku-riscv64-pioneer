/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleRestoreHandoffOwner.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
struct RestoreHandoffIO {
    DataflowRoleSnapshotHandoff handoff{0x1234,0x5678,0x9abc};
    bool held=true,completion=false,completedHeld=false;
    unsigned writes=0,reads=0,heldChecks=0,failHeldCheck=0,failWrite=0,driftRead=0;
    bool RestoreHandoffHeldSafe(unsigned core) {
        return core==3 && held && ++heldChecks!=failHeldCheck;
    }
    bool RestoreCompletionSafe(unsigned core) { return core==3 && !held && completion; }
    bool RestoreCompletedAndHeld(unsigned core) { return core==3 && held && completedHeld; }
    bool ReadRestoreHandoff(unsigned core,DataflowRoleSnapshotHandoff& out) {
        if (core!=3) return false;
        out=handoff;
        if (++reads==driftRead) out.readerReady^=1;
        return true;
    }
    bool WriteRestoreHandoffWord(unsigned core,unsigned word,uint32_t value) {
        if (core!=3 || !held || word>=3) return false;
        if (word==0) handoff.readerReady=value;
        else if (word==1) handoff.writerReady=value;
        else handoff.release=value;
        return ++writes!=failWrite; // failure may still have reached hardware
    }
};
int main()
{
    {
        RestoreHandoffIO io;DataflowRoleRestoreHandoffOwner owner;
        assert(owner.Capture(3,io) && owner.Untouched() && owner.ClearForRelease(io));
        assert(owner.ArmedForRelease() && owner.ClearAttempts()==7 && !owner.ClearForRelease(io));
        assert(owner.BeginReleaseAttempt(io) && owner.ReleaseWasAttempted()
            && !owner.BeginReleaseAttempt(io) && !owner.RestoreOriginal(io));
        io.held=false;io.completion=true;
        io.handoff={PIONEER_Q35_ROLE_RESTORE_READER_DONE,PIONEER_Q35_ROLE_RESTORE_WRITER_DONE,0};
        io.driftRead=io.reads+2;
        assert(!owner.ConfirmCompletion(io));
        io.driftRead=0;io.handoff.writerReady=PIONEER_Q35_ROLE_RESTORE_WRITER_FAILED;
        assert(!owner.ConfirmCompletion(io));
        io.handoff.writerReady=PIONEER_Q35_ROLE_RESTORE_WRITER_DONE;
        assert(owner.ConfirmCompletion(io) && !owner.ConfirmCompletion(io));
        io.held=true;
        assert(!owner.RestoreOriginal(io)); // completion continuity not proved
        io.completedHeld=true;
        assert(owner.RestoreOriginal(io) && owner.HandoffRestored() && owner.ReleaseWasAttempted()
            && owner.RestoreAttempts()==7 && !owner.RestoreOriginal(io));
        assert(io.handoff.readerReady==0x1234 && io.handoff.writerReady==0x5678 && io.handoff.release==0x9abc);
    }
    {
        RestoreHandoffIO io;DataflowRoleRestoreHandoffOwner owner;
        assert(owner.Capture(3,io) && owner.ClearForRelease(io));
        io.handoff.readerReady=PIONEER_Q35_ROLE_RESTORE_READER_DONE; // stale receipt
        assert(!owner.BeginReleaseAttempt(io) && !owner.ReleaseWasAttempted());
        assert(owner.RestoreOriginal(io) && owner.HandoffRestored());
    }
    {
        RestoreHandoffIO io;DataflowRoleRestoreHandoffOwner owner;
        assert(owner.Capture(3,io));io.failWrite=2;
        assert(!owner.ClearForRelease(io) && owner.ClearAttempts()==3 && owner.RestorationRequired());
        assert(!owner.ClearForRelease(io) && !owner.BeginReleaseAttempt(io));
        io.failWrite=0;assert(owner.RestoreOriginal(io) && owner.HandoffRestored());
        assert(io.handoff.readerReady==0x1234 && io.handoff.writerReady==0x5678 && io.handoff.release==0x9abc);
    }
    {
        RestoreHandoffIO io;DataflowRoleRestoreHandoffOwner owner;
        assert(owner.Capture(3,io) && owner.ClearForRelease(io));
        io.failWrite=io.writes+1;
        assert(!owner.RestoreOriginal(io) && owner.RestoreAttempts()==1 && owner.RestorationRequired());
        const unsigned writes=io.writes;io.failWrite=0;
        assert(!owner.RestoreOriginal(io) && io.writes==writes);
    }
    std::puts("PASS restore handoff freshness, one-shot writes and proof-gated cleanup; SIMULATED ONLY");
}
