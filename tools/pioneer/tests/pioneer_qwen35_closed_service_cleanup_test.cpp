/* SPDX-License-Identifier: MIT */
// Exercise the actual extracted context close with simulated binding state.
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#include <cassert>
#include <cstdio>

namespace PioneerQwen35 {
enum DataflowServiceResult { DataflowServiceInvalidState,DataflowServiceClosed,DataflowServiceRestoreFailed };
}
static bool sColdRecoveryRequired=false;
static unsigned destroyed=0,freed=0;
struct Binding { bool safe=true; };
static bool PioneerQ35DestroySharedRoleBinding(Binding* binding) {
    if (!binding) return true;
    if (!binding->safe) return false;
    ++destroyed;delete binding;return true;
}
struct Memory { ~Memory() { ++freed; } };
struct IO {};
struct Service {
    bool closed=false,closeSucceeds=true;unsigned calls=0;
    bool Closed() const { return closed; }
    PioneerQwen35::DataflowServiceResult Close(IO&) {
        ++calls;if (!closeSucceeds) return PioneerQwen35::DataflowServiceRestoreFailed;
        closed=true;return PioneerQwen35::DataflowServiceClosed;
    }
};
struct Context {
    Service* qwen35DataflowService=nullptr;
    Binding* qwen35SharedRoleBinding=nullptr;
    Memory* qwen35DataflowMemory=nullptr;
    PioneerQwen35::DataflowServiceResult qwen35DataflowLastResult=PioneerQwen35::DataflowServiceInvalidState;
    IO io;
#include "pioneer_q35_dataflow_close.inc"
};
int main() {
    {
        Context c;assert(c.CloseDataflowRewrite()==PioneerQwen35::DataflowServiceInvalidState);
    }
    {
        Service service;service.closed=true;
        Context c;c.qwen35DataflowService=&service;c.qwen35SharedRoleBinding=new Binding;
        c.qwen35DataflowMemory=new Memory;
        assert(c.CloseDataflowRewrite()==PioneerQwen35::DataflowServiceClosed && !service.calls
            && !c.qwen35SharedRoleBinding && !c.qwen35DataflowMemory && destroyed==1 && freed==1);
        assert(c.CloseDataflowRewrite()==PioneerQwen35::DataflowServiceClosed && destroyed==1 && freed==1);
    }
    {
        Service service;service.closed=true;
        Context c;c.qwen35DataflowService=&service;c.qwen35SharedRoleBinding=new Binding{false};
        c.qwen35DataflowMemory=new Memory;
        assert(c.CloseDataflowRewrite()==PioneerQwen35::DataflowServiceRestoreFailed
            && sColdRecoveryRequired && c.qwen35SharedRoleBinding && c.qwen35DataflowMemory);
        assert(c.CloseDataflowRewrite()==PioneerQwen35::DataflowServiceRestoreFailed && !service.calls);
        // Fake external teardown only; real retained state must not be freed.
        delete c.qwen35SharedRoleBinding;delete c.qwen35DataflowMemory;sColdRecoveryRequired=false;
    }
    {
        Service service;service.closeSucceeds=false;
        Context c;c.qwen35DataflowService=&service;c.qwen35SharedRoleBinding=new Binding;
        c.qwen35DataflowMemory=new Memory;
        assert(c.CloseDataflowRewrite()==PioneerQwen35::DataflowServiceRestoreFailed
            && service.calls==1 && c.qwen35SharedRoleBinding && c.qwen35DataflowMemory);
        delete c.qwen35SharedRoleBinding;delete c.qwen35DataflowMemory;
    }
    std::puts("PASS closed service still retires binding and retains failed cleanup; SIMULATED ONLY");
}
