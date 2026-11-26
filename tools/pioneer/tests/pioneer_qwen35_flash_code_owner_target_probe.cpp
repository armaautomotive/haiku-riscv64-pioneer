/* SPDX-License-Identifier: MIT */
// Compile-only owner witness; none of these undefined methods grants a
// physical mapping, held fleet, exclusion, snapshot lifetime or retry proof.
#define PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#include "../PioneerQwen35DataflowFlashCodeOwner.h"
using namespace PioneerQwen35;
struct ProbeScope {
    bool FlashFleetHeldAndQuiet();
    bool FlashSnapshotSafe(const uint32_t*,unsigned,bool);
    bool FlashImageSafe(const uint32_t*,unsigned,const uint32_t*,unsigned);
    bool FlashRestoreSafe(bool);
    bool FlashReadAllowed(unsigned,unsigned,unsigned);
    bool FlashWriteAllowed(unsigned,unsigned,unsigned);
    bool Read(unsigned,unsigned,uint32_t*,unsigned);
    bool Write(unsigned,unsigned,const uint32_t*,unsigned);
};
extern "C" bool q35_flash_code_capture(DataflowFlashCodeFleetOwner& owner,uint32_t* storage,
    unsigned words,ProbeScope& scope) {return owner.CaptureAll(storage,words,scope);}
extern "C" bool q35_flash_code_stage(DataflowFlashCodeFleetOwner& owner,const uint32_t* image,
    unsigned words,ProbeScope& scope) {return owner.StageAll(image,words,scope);}
extern "C" bool q35_flash_code_restore(DataflowFlashCodeFleetOwner& owner,ProbeScope& scope) {
    return owner.RestoreAll(scope);
}
