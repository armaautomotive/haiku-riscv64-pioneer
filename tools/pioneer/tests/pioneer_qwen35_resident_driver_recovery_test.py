#!/usr/bin/env python3
"""Test retained resident-owner retirement against simulated recovery evidence."""
import os
from pathlib import Path
import subprocess
import tempfile

test = Path(__file__).resolve().parent
root = test.parents[2]
misc = root / 'src/add-ons/kernel/drivers/misc'
marker = '\nint main() {'
host = (test / 'pioneer_qwen35_resident_host_pci_test.cpp').read_text()
service = (test / 'pioneer_qwen35_resident_service_ownership_test.cpp').read_text()
base = (test / 'pioneer_qwen35_resident_driver_owner_test.cpp').read_text().split(marker, 1)[0]
owner = (misc / 'PioneerQwen35ResidentDriverOwner.inc').read_text()
needle = 'bool PhysicalRuntimeProof(PioneerSessionIO&,const Command&) const { return false; }'
assert owner.count(needle) == 1
owner = owner.replace(needle, 'bool PhysicalRuntimeProof(PioneerSessionIO&,const Command&) const { return true; }')
base = base.replace('struct PioneerStoreContext;', '''#include "PioneerQ35GraphRecoveryProof.h"
constexpr int B_OK=0;
static bool ticketValid=true;
static unsigned guardCalls=0,guardFailAt=0;
static int ValidateRecovery(uint64_t ticket) {
    if (++guardCalls==guardFailAt) ticketValid=false;
    return ticketValid && ticket==9?B_OK:-1;
}
struct pioneer_driver_recovery_guard_v2_info { unsigned version=2;int (*validate)(uint64_t)=ValidateRecovery; };
struct PioneerStoreContext;''', 1)
base = base.replace('struct PioneerSessionIO : ResidentRaw, MockDataflowIO {',
                    'struct PioneerSessionIO : ResidentRaw, MockDataflowIO {\n    bool postResetDisposeComplete=false,fabricOldMmioDisposeComplete=false,fabricHostDisposeComplete=false;', 1)
base = base.replace('struct PioneerStoreContext {', '''struct PioneerStoreContext {
    bool recoveryAttempted=false,dispatchOwnerRetained=false,recoveryFinishPending=false;
    bool recoveryFabricEvidenceRecorded=false;
    uint64_t recoveryFinishTicket=0,recoveryFabricStartTicket=0;
    PioneerDriverRecoveryProof recoveryFinishProof{};
    PioneerDriverFabricRecoveryProof recoveryFabricEvidence{};
    void* recoveryMapping=nullptr;''', 1)
base = base.replace('#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentDriverOwner.inc"',
                    '#include "resident-driver-recovery-test.inc"')
main = r'''
static void Scenario(unsigned mode) {
    const bool fabric=mode==1;
    sColdRecoveryRequired=false;sReloadQuiesced=false;sBankStorageOnly=true;sReloadGeneration=1;
    ticketValid=true;guardCalls=guardFailAt=0;
    pioneer_driver_recovery_guard_v2_info guard{};
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    auto service=std::make_unique<DataflowServiceOwner>();auto serviceIO=std::make_unique<MockDataflowIO>();
    uint32_t code[96]{},dispatch[80]{};
    const DataflowServiceImageView image={DataflowServiceBootWord,code,96,dispatch,80};
    assert(service->Install(image,*serviceIO)==DataflowServiceComplete);
    auto* authority=new DataflowProjectionAuthority;
    assert(authority->BindValidatedModel(f->manifest,f->workspace,f->routes,f->rope));
    auto scratch=std::make_unique<DataflowProjectionProgram>();auto context=std::make_unique<PioneerStoreContext>();
    PioneerQ35GraphOwner graph;TestRoleScope role;TestRuntimeOwner runtime{};runtime.context=context.get();sRuntimeOwner=&runtime;
    context->io.qwen35DataflowContext=context.get();context->io.qwen35BorrowedRoleScope=&role;
    context->qwen35GraphOwner=&graph;context->qwen35DataflowService=service.get();
    context->qwen35DataflowAuthority=authority;context->qwen35DataflowProgramScratch=scratch.get();
    context->qwen35DataflowMemory=context.get();memcpy(context->qwen35DataflowRoutes,f->routes,sizeof(f->routes));
    PioneerQ35StreamRecordingWitness stream{plan.get()};
    auto* owner=PioneerQ35ResidentDriverCreate(*context,&graph,stream);
    assert(owner && PioneerQ35ResidentDriverBind(owner));
    context->io.drop=0;DataflowArgmaxResult output{};unsigned stage=0;
    assert(PioneerQ35ResidentDriverRun(owner,0,42,true,3,output,stage)==DataflowGraphSessionRetained
        && PioneerQ35ResidentDriverRetained(owner) && service->Faulted());
    sColdRecoveryRequired=true;context->recoveryAttempted=context->dispatchOwnerRetained=true;
    context->recoveryFinishPending=context->recoveryFabricEvidenceRecorded=true;
    context->recoveryFinishTicket=context->recoveryFabricStartTicket=9;
    context->io.postResetDisposeComplete=!fabric;context->io.fabricOldMmioDisposeComplete=fabric;
    context->io.fabricHostDisposeComplete=fabric;
    auto& asic=context->recoveryFinishProof;
    asic.flags=PIONEER_RECOVERY_PROOF_REQUIRED & ~PIONEER_RECOVERY_PROOF_RETAINED_OWNERS_RELEASED;
    asic.core_count=120;asic.retained_owners=1;asic.reset_generation=asic.firmware_generation=9;
    auto& fp=context->recoveryFabricEvidence;
    assert(!PioneerQ35ResidentDriverFabricRetired(owner,9));
    fp.flags=PIONEER_FABRIC_PROOF_REQUIRED & ~(PIONEER_FABRIC_PROOF_OWNERS_RELEASED | PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED);
    fp.core_count=120;fp.retained_owners=1;fp.fabric_generation=fp.firmware_generation=9;
    auto retire=[&](uint64_t ticket) {
        return fabric?PioneerQ35ResidentDriverReleaseAfterFabric(owner,fp,ticket,&guard)
            :PioneerQ35ResidentDriverReleaseAfterReset(owner,asic,ticket,&guard);
    };
    assert(!retire(8) && PioneerQ35ResidentDriverRetained(owner));
    ticketValid=false;assert(!retire(9) && PioneerQ35ResidentDriverRetained(owner));ticketValid=true;
    context->io.postResetDisposeComplete=fabric;context->io.fabricOldMmioDisposeComplete=!fabric;
    assert(!retire(9) && PioneerQ35ResidentDriverRetained(owner));
    context->io.postResetDisposeComplete=!fabric;context->io.fabricOldMmioDisposeComplete=fabric;
    if (fabric) {
        context->io.fabricHostDisposeComplete=false;
        assert(!retire(9) && PioneerQ35ResidentDriverRetained(owner));
        context->io.fabricHostDisposeComplete=true;
        context->io.postResetDisposeComplete=true;
        assert(!retire(9) && PioneerQ35ResidentDriverRetained(owner));
        context->io.postResetDisposeComplete=false;
    }
    context->recoveryMapping=context.get();
    assert(!retire(9) && PioneerQ35ResidentDriverRetained(owner));
    context->recoveryMapping=nullptr;sColdRecoveryRequired=false;
    assert(!retire(9) && PioneerQ35ResidentDriverRetained(owner));
    sColdRecoveryRequired=true;
    if (fabric) fp.flags &= ~PIONEER_FABRIC_PROOF_PCIE_HEALTHY;
    else asic.flags &= ~PIONEER_RECOVERY_PROOF_HOST_DMA_IDLE;
    assert(!retire(9) && PioneerQ35ResidentDriverRetained(owner));
    if (fabric) fp.flags |= PIONEER_FABRIC_PROOF_PCIE_HEALTHY;
    else asic.flags |= PIONEER_RECOVERY_PROOF_HOST_DMA_IDLE;
    if (mode==2) {
        guardCalls=0;guardFailAt=5;
        assert(!retire(9) && !ticketValid && PioneerQ35ResidentDriverRetained(owner)
            && service->ResidentRecoveredForRetirement(stream.reserved));
        ticketValid=true;guardFailAt=0;
        assert(!retire(9) && !PioneerQ35ResidentDriverDestroy(owner));
        PioneerQ35DeleteAligned(owner);delete authority;sRuntimeOwner=nullptr;return;
    }
    assert(retire(9) && !PioneerQ35ResidentDriverRetained(owner)
        && service->ResidentRecoveredForRetirement(stream.reserved) && retire(9));
    assert(PioneerQ35ResidentDriverFabricRetired(owner,9)==fabric
        && !PioneerQ35ResidentDriverFabricRetired(owner,8));
    assert(!(fabric?PioneerQ35ResidentDriverReleaseAfterReset(owner,asic,9,&guard):PioneerQ35ResidentDriverReleaseAfterFabric(owner,fp,9,&guard)));
    assert(!retire(8) && PioneerQ35ResidentDriverDestroy(owner) && !owner);
    delete authority;sRuntimeOwner=nullptr;
}
int main() {
    (void)&q35_service_owner_fixture_main;(void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    (void)&PioneerQ35ResidentDriverReady;(void)&PioneerQ35ResidentDriverClose;
    Scenario(0);Scenario(1);Scenario(2);
    puts("PASS retained resident owner retires only with matching ASIC or fabric evidence; SIMULATED ONLY");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    d = Path(tmp)
    (d / 'resident-host-fixture.inc').write_text(host.split(marker, 1)[0] + '\n')
    (d / 'resident-service-fixture.inc').write_text(service.split(marker, 1)[0] + '\n')
    (d / 'resident-driver-recovery-test.inc').write_text(owner)
    (d / 'test.cpp').write_text(base + main)
    defs = ['-DPIONEER_QWEN35_DEVICE_DISPATCH', '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
            '-DPIONEER_QWEN35_RECOVERY_RUN_ENABLE', '-DPIONEER_QWEN35_FABRIC_GUARD_WITNESS',
            '-DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS', '-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS',
            '-DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS']
    for name, flags in [('ordinary', []), ('paired', ['-DPIONEER_QWEN35_PAIRED_RECURRENCE']),
                        ('tree-paired', ['-DPIONEER_QWEN35_TREE_SETTLEMENT', '-DPIONEER_QWEN35_PAIRED_RECURRENCE'])]:
        binary = d / name
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-I' + str(d), '-I' + str(test), '-I' + str(misc),
                        *defs, *flags, str(d / 'test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True,
                       env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
