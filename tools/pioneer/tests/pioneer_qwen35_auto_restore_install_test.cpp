/* SPDX-License-Identifier: MIT */
// Exercise the actual snapshot-mode install selection and ownership cleanup.
#include <cassert>
#include <cstdio>
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
namespace PioneerQwen35 { enum DataflowServiceResult { DataflowServiceInvalidState, DataflowServiceComplete }; }
static bool sColdRecoveryRequired=false,selectOkay=false,destroyOkay=false;
static unsigned selected=0,destroyed=0,serviceDeleted=0,memoryDeleted=0;
struct Binding {};
struct Service { ~Service() { ++serviceDeleted; } };
struct Memory { ~Memory() { ++memoryDeleted; } };
static bool PioneerQ35SelectSharedRoleAutoRestore(Binding* b) { ++selected;return b && selectOkay; }
static bool PioneerQ35DestroySharedRoleBinding(Binding* b) {
    ++destroyed;if (!destroyOkay) return false;delete b;return true;
}
struct Context {
    Binding* qwen35SharedRoleBinding=new Binding;
    Service* qwen35DataflowService=new Service;
    Memory* qwen35DataflowMemory=new Memory;
    PioneerQwen35::DataflowServiceResult Install() {
#include "pioneer_q35_auto_restore_select.inc"
        return PioneerQwen35::DataflowServiceComplete;
    }
    ~Context() { delete qwen35SharedRoleBinding;delete qwen35DataflowService;delete qwen35DataflowMemory; }
};
int main() {
    for (unsigned scenario=0;scenario<3;scenario++) {
        selected=destroyed=serviceDeleted=memoryDeleted=0;sColdRecoveryRequired=false;
        selectOkay=scenario==0;destroyOkay=scenario==1;
        {
            Context c;
            assert(c.Install()==(scenario==0?PioneerQwen35::DataflowServiceComplete:PioneerQwen35::DataflowServiceInvalidState));
            assert(selected==1 && destroyed==(scenario?1U:0U));
            if (scenario==0) assert(c.qwen35SharedRoleBinding && c.qwen35DataflowService && c.qwen35DataflowMemory && !sColdRecoveryRequired);
            if (scenario==1) assert(!c.qwen35SharedRoleBinding && !c.qwen35DataflowService && !c.qwen35DataflowMemory
                && serviceDeleted==1 && memoryDeleted==1 && !sColdRecoveryRequired);
            if (scenario==2) assert(c.qwen35SharedRoleBinding && c.qwen35DataflowService && c.qwen35DataflowMemory
                && !serviceDeleted && !memoryDeleted && sColdRecoveryRequired);
        }
    }
    std::puts("PASS snapshot service selects restore images before install and retains failed cleanup; SIMULATED ONLY");
}
