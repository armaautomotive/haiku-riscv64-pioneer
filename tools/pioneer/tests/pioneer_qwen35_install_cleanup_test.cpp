/* SPDX-License-Identifier: MIT */
// Exercise the real post-install cleanup branch with retained ownership.
#include <cassert>
#include <cstdio>
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
namespace PioneerQwen35 { enum DataflowServiceResult {
    DataflowServicePreflightFailed, DataflowServiceRestoreFailed
}; }
static bool sColdRecoveryRequired=false, destroyOkay=false;
static unsigned bindingDeleted=0,serviceDeleted=0,memoryDeleted=0;
struct Binding { ~Binding() { ++bindingDeleted; } };
struct Service { bool started; bool Started() const { return started; } ~Service() { ++serviceDeleted; } };
struct Memory { ~Memory() { ++memoryDeleted; } };
static bool PioneerQ35DestroySharedRoleBinding(Binding* binding) {
    if (!destroyOkay) return false;
    delete binding;return true;
}
struct Context {
    Binding* qwen35SharedRoleBinding=new Binding;
    Service* qwen35DataflowService;
    Memory* qwen35DataflowMemory=new Memory;
    PioneerQwen35::DataflowServiceResult qwen35DataflowLastResult=PioneerQwen35::DataflowServicePreflightFailed;
    explicit Context(bool started):qwen35DataflowService(new Service{started}) {}
    PioneerQwen35::DataflowServiceResult Finish() {
        const auto result=PioneerQwen35::DataflowServicePreflightFailed;
#include "pioneer_q35_install_cleanup.inc"
        qwen35DataflowLastResult=result;return result;
    }
    ~Context() { delete qwen35SharedRoleBinding;delete qwen35DataflowService;delete qwen35DataflowMemory; }
};
int main() {
    for (unsigned scenario=0;scenario<3;scenario++) {
        sColdRecoveryRequired=false;bindingDeleted=serviceDeleted=memoryDeleted=0;
        destroyOkay=scenario==1;
        {
            Context c(scenario==0);
            const auto result=c.Finish();
            if (scenario==1) {
                assert(result==PioneerQwen35::DataflowServicePreflightFailed && !sColdRecoveryRequired);
                assert(!c.qwen35SharedRoleBinding && !c.qwen35DataflowService && !c.qwen35DataflowMemory);
                assert(bindingDeleted==1 && serviceDeleted==1 && memoryDeleted==1);
            } else {
                assert(c.qwen35SharedRoleBinding && c.qwen35DataflowService && c.qwen35DataflowMemory);
                assert(!bindingDeleted && !serviceDeleted && !memoryDeleted);
                assert(sColdRecoveryRequired==(scenario==2));
                assert(result==(scenario==2?PioneerQwen35::DataflowServiceRestoreFailed:
                    PioneerQwen35::DataflowServicePreflightFailed));
                assert(c.qwen35DataflowLastResult==result);
            }
        }
        assert(bindingDeleted==1 && serviceDeleted==1 && memoryDeleted==1);
    }
    std::puts("PASS install cleanup preserves the complete owner on failed binding retirement; SIMULATED ONLY");
}
