#!/usr/bin/env python3
"""Test real owner reuse with simulated completion; no physical admission."""
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
needle = '''    PioneerQwen35ResidentFleetAuditPci<PioneerSessionIO,Scope> pci(io,*this);
    return audit.Audit(c,pci)==PioneerQwen35::DataflowResidentFleetAuditComplete && audit.AuditedFor(c);'''
assert owner.count(needle) == 1
owner = owner.replace(needle, '    (void)io;(void)c;return true; // simulated fleet receipt')
needle = 'public:\n    PioneerQ35ResidentDriverOwner(PioneerStoreContext& c,'
assert owner.count(needle) == 1
owner = owner.replace(needle, '''public:
    const void* TestIdleHandle() const { return idleHandle; }
    const void* TestIdleCandidate() const { return idleCandidate; }
    const void* TestActiveHandle() const { return active; }
    const void* TestRetainedCandidate() const { return runner.RetainedCandidate(); }
    void TestDiscardRetainedSimulation() {
        assert(runner.Retained() && active && runner.RetainedCandidate()!=context.qwen35DataflowAuthority);
        delete const_cast<PioneerQwen35::DataflowProjectionAuthority*>(runner.RetainedCandidate());
        PioneerQ35DeleteAligned(active);active=nullptr;
    }
    PioneerQ35ResidentDriverOwner(PioneerStoreContext& c,''')
needle = '    void Pause() { ResidentRaw::Pause(); }'
assert base.count(needle) == 1
base = base.replace(needle, '''    unsigned delivered=0;
    void Pause() {
        DataflowResidentCommand command{};
        memcpy(&command,panel+(DataflowBarrierReleasePayload-DataflowPciL1ViewBase)/4,sizeof(command));
        const auto* marker=panel+(DataflowBarrierReleaseCommit-DataflowPciL1ViewBase)/4;
        if (DataflowResidentCommandValid(command) && marker[0]==command.epoch && command.trigger>delivered) {
            PutCandidate(*this,command);delivered=command.trigger;
        }
        ResidentRaw::Pause();
    }''')
base = base.replace('#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentDriverOwner.inc"',
                    '#include "resident-driver-permitted-test.inc"')
main = r'''
int main() {
    (void)&q35_service_owner_fixture_main;(void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    (void)&PioneerQ35ResidentDriverReady;(void)&PioneerQ35ResidentDriverClose;(void)&PioneerQ35ResidentDriverDestroy;
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
    DataflowArgmaxResult output{};unsigned stage=0;
    assert(PioneerQ35ResidentDriverRun(owner,1,42,true,3,output,stage)==DataflowGraphSessionPreflight);
    const void* handle=owner->TestIdleHandle();const void* candidate=owner->TestIdleCandidate();
    assert(handle && candidate && context->qwen35DataflowAuthority==authority && !context->io.writes);
    for (unsigned position=0;position<3;position++) {
        context->io.writes=0;
        assert(PioneerQ35ResidentDriverRun(owner,position,42+position,true,3,output,stage)==DataflowGraphSessionComplete);
        assert(output.token==42 && owner->TestIdleHandle()==handle && owner->TestIdleCandidate()
            && owner->TestIdleCandidate()!=context->qwen35DataflowAuthority
            && !PioneerQ35ResidentDriverRetained(owner) && !sColdRecoveryRequired);
        if (!position) assert(context->qwen35DataflowAuthority==candidate && owner->TestIdleCandidate()==authority);
        if (position==1) assert(context->qwen35DataflowAuthority==authority && owner->TestIdleCandidate()==candidate);
        if (position==2) assert(context->qwen35DataflowAuthority==candidate && owner->TestIdleCandidate()==authority);
    }
    context->io.writes=0;context->io.drop=0;
    assert(PioneerQ35ResidentDriverRun(owner,3,45,true,3,output,stage)==DataflowGraphSessionRetained
        && PioneerQ35ResidentDriverRetained(owner) && owner->TestActiveHandle()==handle
        && owner->TestRetainedCandidate()==authority && !owner->TestIdleHandle() && !owner->TestIdleCandidate());
    context->io.writes=0;context->io.drop=-1;
    assert(PioneerQ35ResidentDriverRun(owner,3,45,true,3,output,stage)==DataflowGraphSessionInvalid
        && !context->io.writes && owner->TestActiveHandle()==handle);
    auto* live=context->qwen35DataflowAuthority;
    owner->TestDiscardRetainedSimulation();PioneerQ35DeleteAligned(owner);delete live;sRuntimeOwner=nullptr;
    puts("PASS resident owner reuses one transport and two authorities across simulated tokens; NO PHYSICAL ADMISSION");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    d = Path(tmp)
    (d / 'resident-host-fixture.inc').write_text(host.split(marker, 1)[0] + '\n')
    (d / 'resident-service-fixture.inc').write_text(service.split(marker, 1)[0] + '\n')
    (d / 'resident-driver-permitted-test.inc').write_text(owner)
    (d / 'test.cpp').write_text(base + main)
    defs = ['-DPIONEER_QWEN35_DEVICE_DISPATCH', '-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
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
