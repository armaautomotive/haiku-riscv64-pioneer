/* SPDX-License-Identifier: MIT */
// Test the exact driver qualification guard and routing with the real service
// owner and simulated IO. This does not authorize hardware operations.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#ifdef PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#endif
#include <memory>
#define main dataflow_service_existing_test_main
#include "pioneer_qwen35_dataflow_service_owner_test.cpp"
#undef main

struct Owner {
    bool active=false,retained=false,scratch=true;
    bool Active() const { return active; }
    bool Retained() const { return retained; }
    const void* Scratch() const { return scratch?this:nullptr; }
};
struct CountingIO : MockDataflowIO {
    mutable unsigned idleChecks=0;
    bool SessionIdle() const { ++idleChecks;return MockDataflowIO::SessionIdle(); }
};
struct Context;
#ifdef PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS
static DataflowServiceResult PioneerQ35GraphOwnerInstallResident(Context&,const DataflowServiceImageView&);
#endif
struct Context {
    bool qwen35QualificationInternalActive=false;
    Owner qwen35QualificationOwner;
    void* qwen35GraphOwner=nullptr;
    DataflowServiceOwner service;
    DataflowServiceOwner* qwen35DataflowService=&service;
    CountingIO io;
    unsigned graphInstalls=0;
    DataflowServiceResult Install(bool fixedQualification) {
#include "pioneer_q35_install_qualification_guard.inc"
        uint32_t code[96]{},dispatchCode[80]{};
        DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatchCode,80};
#include "pioneer_q35_install_route.inc"
        return result;
    }
};
#ifdef PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS
static DataflowServiceResult PioneerQ35GraphOwnerInstallResident(Context& context,const DataflowServiceImageView&) {
    ++context.graphInstalls;return DataflowServiceComplete;
}
#endif
int main() {
    auto context=std::make_unique<Context>();
    assert(context->Install(true)==DataflowServiceInvalidState && !context->io.idleChecks);
    context->qwen35QualificationOwner.active=true;
    assert(context->Install(true)==DataflowServiceInvalidState && !context->io.idleChecks);
    context->qwen35QualificationInternalActive=true;
    context->qwen35QualificationOwner.retained=true;
    assert(context->Install(true)==DataflowServiceInvalidState && !context->io.idleChecks);
    context->qwen35QualificationOwner.retained=false;
    context->qwen35QualificationOwner.scratch=false;
    assert(context->Install(true)==DataflowServiceInvalidState && !context->io.idleChecks);
    context->qwen35QualificationOwner.scratch=true;
    context->qwen35GraphOwner=context.get();
    assert(context->Install(true)==DataflowServiceInvalidState && !context->io.idleChecks);
    context->qwen35GraphOwner=nullptr;
    context->io.failIdleCore=0;
    assert(context->Install(true)==DataflowServicePreflightFailed && context->io.idleChecks==1
        && !context->graphInstalls);
#ifdef PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS
    assert(context->Install(false)==DataflowServiceComplete && context->graphInstalls==1
        && context->io.idleChecks==1);
#else
    assert(context->Install(false)==DataflowServicePreflightFailed && !context->graphInstalls
        && context->io.idleChecks==2);
#endif
    std::puts("PASS fixed qualification selects its actual service; ordinary resident selects graph; SIMULATED ONLY");
}
