/* SPDX-License-Identifier: MIT */
// Fixed flash-code fleet ownership over fake memory. No PCI or hardware.
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowFlashCodeOwner.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
using namespace PioneerQwen35;
struct FakeIO {
    std::vector<uint32_t>& data;
    bool held=true,snapshotSafe=true,imageSafe=true,restoreSafe=true,retrySafe=true;
    int failRead=-1,failWrite=-1,misrouteWrite=-1;unsigned reads=0,writes=0;
    bool FlashFleetHeldAndQuiet() const {return held;}
    bool FlashSnapshotSafe(const uint32_t* p,unsigned n,bool) const {
        return snapshotSafe && p && n==DataflowFlashFleetSnapshotWords;
    }
    bool FlashImageSafe(const uint32_t* p,unsigned n,const uint32_t* snapshot,unsigned words) const {
        return imageSafe && p && n && snapshot && words==DataflowFlashFleetSnapshotWords;
    }
    bool FlashRestoreSafe(bool retry) const {return restoreSafe && (!retry || retrySafe);}
    bool Range(unsigned core,unsigned address,unsigned bytes) const {
        return core<DataflowBarrierWorkers && address>=DataflowFlashCodeAddress
            && address<DataflowFlashCodeLimit && bytes && !(address&3) && !(bytes&3)
            && bytes<=DataflowFlashCodeLimit-address;
    }
    bool FlashReadAllowed(unsigned core,unsigned address,unsigned bytes) const {return held && Range(core,address,bytes);}
    bool FlashWriteAllowed(unsigned core,unsigned address,unsigned bytes) const {return held && Range(core,address,bytes);}
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        if (!Range(core,address,bytes) || !out || int(reads++)==failRead) return false;
        std::memcpy(out,data.data()+core*DataflowFlashCodeWords+(address-DataflowFlashCodeAddress)/4,bytes);return true;
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        if (!Range(core,address,bytes) || !in) return false;
        const unsigned write=writes++;
        std::memcpy(data.data()+core*DataflowFlashCodeWords+(address-DataflowFlashCodeAddress)/4,in,bytes);
        if (int(write)==misrouteWrite) data[0]^=0x80000000U; // later extra write to earlier worker
        return int(write)!=failWrite; // a reported failure can have taken effect
    }
};
int main() {
    uint64_t physical=0;
    assert(DataflowFlashCodePhysicalAddress(0x100000000ULL,0x20000000,physical)
        && physical==0x1190d0000ULL);
    assert(!DataflowFlashCodePhysicalAddress(0x100000001ULL,0x20000000,physical)
        && !DataflowFlashCodePhysicalAddress(0x100000000ULL,0x190d7fff,physical)
        && !DataflowFlashCodePhysicalAddress(UINT64_MAX-0xfff,0x20000000,physical));
    assert(DataflowFlashCodeRange(DataflowFlashCodeAddress,DataflowFlashCodeBytes)
        && DataflowFlashCodeRange(DataflowFlashCodeLimit-4,4)
        && !DataflowFlashCodeRange(DataflowFlashCodeAddress-4,4)
        && !DataflowFlashCodeRange(DataflowFlashCodeLimit,4)
        && !DataflowFlashCodeRange(DataflowFlashCodeLimit-4,8));
    std::vector<uint32_t> original(DataflowFlashFleetSnapshotWords);
    for (unsigned i=0;i<original.size();i++) original[i]=(i*2654435761U)^0x5a91f073U;
    std::vector<uint32_t> device=original,snapshot(original.size()),image(2057);
    for (unsigned i=0;i<image.size();i++) image[i]=0x13000000U+i;
    FakeIO io{device};auto owner=std::make_unique<DataflowFlashCodeFleetOwner>();
    assert(owner->RequiredWords()==original.size());
    assert(!owner->CaptureAll(snapshot.data(),snapshot.size()-1,io) && !io.reads && !io.writes);
    assert(owner->CaptureAll(snapshot.data(),snapshot.size(),io) && snapshot==original);
    assert(owner->VerifyOriginalAll(io));
    assert(!owner->StageAll(snapshot.data(),32,io) && !io.writes); // image/snapshot alias
    io.imageSafe=false;assert(!owner->StageAll(image.data(),image.size(),io) && !io.writes);io.imageSafe=true;
    assert(owner->StageAll(image.data(),image.size(),io) && owner->StagedAndVerified());
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        const auto start=device.begin()+core*DataflowFlashCodeWords;
        for (unsigned i=0;i<image.size();i++) assert(start[i]==image[i]);
        for (unsigned i=image.size();i<DataflowFlashCodeWords;i++) assert(start[i]==original[core*DataflowFlashCodeWords+i]);
    }
    assert(owner->VerifyStagedAll(io));
    device[17]^=1;assert(!owner->VerifyStagedAll(io) && owner->RestorationRequired());device[17]^=1;
    io.restoreSafe=false;const unsigned before=io.writes;
    assert(!owner->RestoreAll(io) && io.writes==before);io.restoreSafe=true;
    io.failWrite=int(io.writes+7);assert(!owner->RestoreAll(io) && owner->RestorationRequired());
    io.failWrite=-1;io.retrySafe=false;const unsigned retryBefore=io.writes;
    assert(!owner->RestoreAll(io) && io.writes==retryBefore);io.retrySafe=true;
    assert(owner->RestoreAll(io) && owner->RegionRestored() && device==original && owner->VerifyOriginalAll(io));
    assert(!owner->RestoreAll(io) && !owner->StageAll(image.data(),image.size(),io));

    device=original;std::fill(snapshot.begin(),snapshot.end(),0);FakeIO lost{device};
    auto failing=std::make_unique<DataflowFlashCodeFleetOwner>();
    lost.held=false;assert(!failing->CaptureAll(snapshot.data(),snapshot.size(),lost) && !lost.writes);
    lost.held=true;assert(!failing->CaptureAll(snapshot.data(),snapshot.size(),lost)); // fail closed

    auto misrouted=std::make_unique<DataflowFlashCodeFleetOwner>();FakeIO misroute{device};
    assert(misrouted->CaptureAll(snapshot.data(),snapshot.size(),misroute));
    const unsigned writesPerCore=(image.size()+63)/64;
    misroute.misrouteWrite=int(writesPerCore); // a later core corrupts a previously checked core
    assert(!misrouted->StageAll(image.data(),image.size(),misroute) && misrouted->RestorationRequired());
    misroute.misrouteWrite=-1;
    assert(misrouted->RestoreAll(misroute) && device==original);
    device[DataflowFlashCodeWords+7]^=1;
    assert(!misrouted->VerifyOriginalAll(misroute) && misrouted->RestorationRequired());
    assert(misrouted->RestoreAll(misroute) && device==original);
    std::puts("PASS flash code fleet snapshot, stage, fail-closed restore and misroute audit; NO HARDWARE");
}
