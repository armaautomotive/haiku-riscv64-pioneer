/* SPDX-License-Identifier: MIT */
// Fixed flash-code BAR view over fake platform and fake guard only.
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowFlashCodePciView.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct Guard {
    bool pinned=true,route=true,read=true,write=true,mapping=true,area=true,buffer=true,drained=false,retire=false;
    bool absent=false,exact=false;
    unsigned selected=7;
    bool ProvePinnedBar(uint64_t a,uint64_t b) const {return pinned && a==0x100000000ULL && b==0x11fffffffULL;}
    bool ProveSelectedRoute(unsigned core) const {return route && core==selected;}
    bool ProveFlashMappingSafe(uint64_t address,unsigned bytes) const {
        return mapping && address==0x1190d0000ULL && bytes==DataflowFlashCodeBytes;
    }
    bool ProveFlashMappingArea(int32_t id) const {return area && id==3;}
    bool ProveFlashHostBuffer(const uint32_t* p,unsigned bytes,bool) const {return buffer && p && bytes;}
    bool ProveFlashReadAllowed(unsigned core,unsigned address,unsigned bytes) const {
        return read && core==selected && DataflowFlashCodeRange(address,bytes);
    }
    bool ProveFlashWriteAllowed(unsigned core,unsigned address,unsigned bytes) const {
        return write && core==selected && DataflowFlashCodeRange(address,bytes);
    }
    bool ProveFlashWriteDrained() const {return drained;}
    bool ProveFlashMappingRetirementSafe(int32_t id) const {return retire && id==3;}
    bool ProveFlashOriginalMappingAbsent(int32_t id) const {return absent && id==3;}
    bool ProveFlashOriginalMappingExact(int32_t id,const volatile uint32_t* p) const {return exact && id==3 && p;}
    bool ProveFlashFleetHeldAndQuiet() const {return pinned && route;}
    bool ProveFlashSnapshotSafe(const uint32_t* p,unsigned words,bool) const {
        return buffer && p && words==DataflowFlashFleetSnapshotWords;
    }
    bool ProveFlashImageSafe(const uint32_t* p,unsigned words,const uint32_t* saved,unsigned total) const {
        return buffer && p && words && saved && total==DataflowFlashFleetSnapshotWords;
    }
    bool ProveFlashRestoreSafe(bool retry) const {return pinned && route && (!retry || drained);}
    bool SelectFlashRoute(unsigned core) {if (!route || core>=DataflowBarrierWorkers) return false;selected=core;return true;}
};
struct Platform {
    Guard& guard;std::vector<uint32_t> words=std::vector<uint32_t>(DataflowFlashFleetSnapshotWords);
    bool mapOkay=true,unmapOkay=true,revokeAfterWrite=false;unsigned maps=0,unmaps=0,reads=0,writes=0;
    bool MapReadWrite(uint64_t,unsigned,int32_t& area,volatile uint32_t*& view) {
        ++maps;area=3;view=words.data();return mapOkay;
    }
    bool Unmap(int32_t area) {assert(area==3);++unmaps;return unmapOkay;}
    uint32_t ReadWord(const volatile uint32_t* p) {
        ++reads;const auto offset=unsigned(p-words.data());assert(offset<DataflowFlashCodeWords);
        return words[guard.selected*DataflowFlashCodeWords+offset];
    }
    void WriteWord(volatile uint32_t* p,uint32_t value) {
        ++writes;const auto offset=unsigned(p-words.data());assert(offset<DataflowFlashCodeWords);
        words[guard.selected*DataflowFlashCodeWords+offset]=value;if (revokeAfterWrite) guard.write=false;
    }
};
int main() {
    Guard guard;Platform platform{guard};DataflowFlashCodePciView<Platform> view(platform);
    uint32_t data[4]={11,22,33,44},out[4]{};
    assert(!view.Read(7,DataflowFlashCodeAddress,out,sizeof(out),guard) && !platform.reads);
    guard.mapping=false;assert(!view.Open(0x100000000ULL,0x11fffffffULL,guard) && !platform.maps);guard.mapping=true;
    assert(view.Open(0x100000000ULL,0x11fffffffULL,guard) && view.Opened() && view.HasMapping());
    assert(!view.Open(0x100000000ULL,0x11fffffffULL,guard));
    assert(!view.Write(8,DataflowFlashCodeAddress,data,sizeof(data),guard) && !platform.writes);
    assert(!view.Write(7,DataflowFlashCodeLimit-4,data,sizeof(data),guard) && !platform.writes);
    guard.buffer=false;assert(!view.Write(7,DataflowFlashCodeAddress,data,sizeof(data),guard) && !platform.writes);guard.buffer=true;
    guard.area=false;assert(!view.Read(7,DataflowFlashCodeAddress,out,sizeof(out),guard) && !platform.reads);guard.area=true;
    assert(view.Write(7,DataflowFlashCodeAddress,data,sizeof(data),guard) && view.WritesConfirmed());
    assert(view.Read(7,DataflowFlashCodeAddress,out,sizeof(out),guard));
    for (unsigned i=0;i<4;i++) assert(out[i]==data[i]);
    platform.revokeAfterWrite=true;
    assert(!view.Write(7,DataflowFlashCodeAddress,data,sizeof(data),guard) && !view.WritesConfirmed());
    const unsigned reads=platform.reads,writes=platform.writes;
    guard.write=true;guard.retire=true;
    assert(!view.Read(7,DataflowFlashCodeAddress,out,sizeof(out),guard) && platform.reads==reads);
    assert(!view.Write(7,DataflowFlashCodeAddress,data,sizeof(data),guard) && platform.writes==writes);
    assert(!view.Close(guard) && !view.ConfirmWriteDrain(guard));
    guard.drained=true;assert(view.ConfirmWriteDrain(guard) && view.WritesConfirmed());
    platform.revokeAfterWrite=false;assert(view.Write(7,DataflowFlashCodeAddress,data,sizeof(data),guard));
    assert(view.Close(guard) && !view.HasMapping() && platform.unmaps==1);
    assert(!view.Close(guard) && platform.unmaps==1);

    Guard partialGuard;Platform partial{partialGuard};partial.mapOkay=false;
    DataflowFlashCodePciView<Platform> incomplete(partial);
    assert(!incomplete.Open(0x100000000ULL,0x11fffffffULL,partialGuard) && incomplete.HasMapping());
    assert(!incomplete.Close(partialGuard));partialGuard.retire=true;
    assert(incomplete.Close(partialGuard) && !incomplete.HasMapping());

    Guard closeGuard;Platform failedClose{closeGuard};DataflowFlashCodePciView<Platform> closeView(failedClose);
    assert(closeView.Open(0x100000000ULL,0x11fffffffULL,closeGuard));
    closeGuard.retire=true;failedClose.unmapOkay=false;
    assert(!closeView.Close(closeGuard) && closeView.HasMapping() && failedClose.unmaps==1);
    failedClose.unmapOkay=true;assert(!closeView.Close(closeGuard) && failedClose.unmaps==1);
    assert(!closeView.ReconcileFailedClose(closeGuard));
    closeGuard.exact=true;assert(closeView.ReconcileFailedClose(closeGuard) && closeView.Opened());
    assert(closeView.Close(closeGuard) && failedClose.unmaps==2);

    Guard absentGuard;Platform absentPlatform{absentGuard};DataflowFlashCodePciView<Platform> absentView(absentPlatform);
    assert(absentView.Open(0x100000000ULL,0x11fffffffULL,absentGuard));
    absentGuard.retire=true;absentPlatform.unmapOkay=false;
    assert(!absentView.Close(absentGuard) && absentView.HasMapping());
    absentGuard.absent=true;assert(absentView.ReconcileFailedClose(absentGuard) && !absentView.HasMapping());

    Guard fleetGuard;Platform fleet{fleetGuard};
    for (unsigned i=0;i<fleet.words.size();i++) fleet.words[i]=i^0x63a1d275U;
    const auto original=fleet.words;
    DataflowFlashCodePciView<Platform> fleetView(fleet);
    assert(fleetView.Open(0x100000000ULL,0x11fffffffULL,fleetGuard));
    DataflowFlashCodePciOwnerScope<decltype(fleetView),Guard> scope(fleetView,fleetGuard);
    DataflowFlashCodeFleetOwner owner;
    std::vector<uint32_t> saved(owner.RequiredWords()),image(65,0x12345678);
    assert(owner.CaptureAll(saved.data(),saved.size(),scope) && saved==original);
    assert(owner.StageAll(image.data(),image.size(),scope) && owner.VerifyStagedAll(scope));
    assert(owner.RestoreAll(scope) && owner.VerifyOriginalAll(scope) && fleet.words==original);
    fleetGuard.retire=true;assert(fleetView.Close(fleetGuard));

    Guard uncertainGuard;Platform uncertainPlatform{uncertainGuard};
    DataflowFlashCodePciView<Platform> uncertainView(uncertainPlatform);
    assert(uncertainView.Open(0x100000000ULL,0x11fffffffULL,uncertainGuard));
    DataflowFlashCodePciOwnerScope<decltype(uncertainView),Guard> uncertainScope(uncertainView,uncertainGuard);
    DataflowFlashCodeFleetOwner uncertainOwner;std::vector<uint32_t> uncertainSaved(uncertainOwner.RequiredWords());
    assert(uncertainOwner.CaptureAll(uncertainSaved.data(),uncertainSaved.size(),uncertainScope));
    uncertainPlatform.revokeAfterWrite=true;
    assert(!uncertainOwner.StageAll(image.data(),image.size(),uncertainScope)
        && uncertainOwner.RestorationRequired() && !uncertainView.WritesConfirmed());
    uncertainPlatform.revokeAfterWrite=false;uncertainGuard.write=true;
    assert(!uncertainOwner.RestoreAll(uncertainScope));
    uncertainGuard.drained=true;assert(uncertainView.ConfirmWriteDrain(uncertainGuard));
    assert(uncertainOwner.RestoreAll(uncertainScope) && uncertainOwner.VerifyOriginalAll(uncertainScope));
    uncertainGuard.retire=true;assert(uncertainView.Close(uncertainGuard));
    std::puts("PASS fixed flash BAR view, retained uncertain writes and independent drainage; NO HARDWARE");
}
