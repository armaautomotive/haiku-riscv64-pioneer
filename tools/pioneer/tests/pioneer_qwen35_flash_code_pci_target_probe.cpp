/* SPDX-License-Identifier: MIT */
// Compile-only composition; the platform and all guard proofs are undefined.
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowFlashCodePciView.h"
using namespace PioneerQwen35;
struct ProbePlatform {
    bool MapReadWrite(uint64_t,unsigned,int32_t&,volatile uint32_t*&);
    bool Unmap(int32_t);
    uint32_t ReadWord(const volatile uint32_t*);
    void WriteWord(volatile uint32_t*,uint32_t);
};
struct ProbeGuard {
    bool ProvePinnedBar(uint64_t,uint64_t);
    bool ProveSelectedRoute(unsigned);
    bool ProveFlashMappingSafe(uint64_t,unsigned);
    bool ProveFlashMappingArea(int32_t);
    bool ProveFlashMappingRetirementSafe(int32_t);
    bool ProveFlashHostBuffer(const uint32_t*,unsigned,bool);
    bool ProveFlashReadAllowed(unsigned,unsigned,unsigned);
    bool ProveFlashWriteAllowed(unsigned,unsigned,unsigned);
    bool ProveFlashWriteDrained();
    bool ProveFlashFleetHeldAndQuiet();
    bool ProveFlashSnapshotSafe(const uint32_t*,unsigned,bool);
    bool ProveFlashImageSafe(const uint32_t*,unsigned,const uint32_t*,unsigned);
    bool ProveFlashRestoreSafe(bool);
    bool SelectFlashRoute(unsigned);
};
using View=DataflowFlashCodePciView<ProbePlatform>;
using Scope=DataflowFlashCodePciOwnerScope<View,ProbeGuard>;
extern "C" bool q35_flash_pci_open(View& v,ProbeGuard& g,uint64_t first,uint64_t last) {
    return v.Open(first,last,g);
}
extern "C" bool q35_flash_pci_capture(DataflowFlashCodeFleetOwner& owner,View& view,ProbeGuard& guard,
    uint32_t* snapshot,unsigned words) {
    Scope scope(view,guard);return owner.CaptureAll(snapshot,words,scope);
}
extern "C" bool q35_flash_pci_stage(DataflowFlashCodeFleetOwner& owner,View& view,ProbeGuard& guard,
    const uint32_t* image,unsigned words) {
    Scope scope(view,guard);return owner.StageAll(image,words,scope);
}
extern "C" bool q35_flash_pci_restore(DataflowFlashCodeFleetOwner& owner,View& view,ProbeGuard& guard) {
    Scope scope(view,guard);return owner.RestoreAll(scope);
}
extern "C" bool q35_flash_pci_drain(View& view,ProbeGuard& guard) {return view.ConfirmWriteDrain(guard);}
extern "C" bool q35_flash_pci_close(View& view,ProbeGuard& guard) {return view.Close(guard);}
