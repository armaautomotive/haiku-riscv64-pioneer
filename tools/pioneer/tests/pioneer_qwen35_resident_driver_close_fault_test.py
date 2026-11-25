"""Inject a simulated close proof into a private copy of the real owner."""
import os, subprocess, tempfile
from pathlib import Path
test=Path(__file__).resolve().parent
root=test.parents[2]
misc=root/'src/add-ons/kernel/drivers/misc'
marker='\nint main() {'
host=(test/'pioneer_qwen35_resident_host_pci_test.cpp').read_text()
service=(test/'pioneer_qwen35_resident_service_ownership_test.cpp').read_text()
source=(test/'pioneer_qwen35_resident_driver_owner_test.cpp').read_text()
owner=(misc/'PioneerQwen35ResidentDriverOwner.inc').read_text()
needle='bool PhysicalCloseProof(PioneerSessionIO&) const { return false; }'
assert owner.count(needle)==1
owner=owner.replace(needle,'bool PhysicalCloseProof(PioneerSessionIO& io) const { return io.simulatedCloseProof; }')
source=source.replace('struct PioneerSessionIO : ResidentRaw, MockDataflowIO {','struct PioneerSessionIO : ResidentRaw, MockDataflowIO {\n    bool simulatedCloseProof=false;',1)
source=source.replace('#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ResidentDriverOwner.inc"', '#include "resident-driver-permitted-test.inc"',1)
needle='''    assert(!PioneerQ35ResidentDriverDestroy(owner)); // bound service requires a qualified stop
    PioneerQ35DeleteAligned(owner);sRuntimeOwner=nullptr; // test teardown; simulated resources only'''
replacement='''    assert(!PioneerQ35ResidentDriverDestroy(owner)); // bound service requires a qualified stop
    // The generated test-only owner substitutes a simulated proof. Device IO
    // is still the mock; force a failed stop after the service entered close.
    static_cast<MockDataflowIO&>(context->io)=*serviceIO;
    context->io.simulatedCloseProof=true;context->io.failStopAudit=true;
    assert(PioneerQ35ResidentDriverClose(owner)==DataflowServiceStopAuditFailed);
    assert(PioneerQ35ResidentDriverRetained(owner) && service->Faulted()
        && !PioneerQ35ResidentDriverDestroy(owner) && !context->io.resumeCount);
    assert(service->Close(context->io)==DataflowServiceInvalidState
        && PioneerQ35ResidentDriverClose(owner)==DataflowServiceInvalidState);
    PioneerQ35DeleteAligned(owner);sRuntimeOwner=nullptr; // test teardown; simulated resources only'''
assert source.count(needle)==1
source=source.replace(needle,replacement)
source=source.replace('PASS actual resident driver owner binds sealed plan and denies unqualified publication; SIMULATED ONLY', 'PASS actual resident owner retains a simulated failed close; NO PHYSICAL ADMISSION')
with tempfile.TemporaryDirectory() as tmp:
    d=Path(tmp)
    (d/'resident-host-fixture.inc').write_text(host.split(marker,1)[0]+'\n')
    (d/'resident-service-fixture.inc').write_text(service.split(marker,1)[0]+'\n')
    (d/'resident-driver-permitted-test.inc').write_text(owner)
    (d/'test.cpp').write_text(source)
    defs=['-DPIONEER_QWEN35_DEVICE_DISPATCH','-DPIONEER_QWEN35_RECOVERY_COMPILE_ONLY','-DPIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS','-DPIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS','-DPIONEER_QWEN35_RESIDENT_SERVICE_WITNESS']
    for suffix,extra in [('ordinary',[]),('paired',['-DPIONEER_QWEN35_PAIRED_RECURRENCE']),
                         ('tree-paired',['-DPIONEER_QWEN35_TREE_SETTLEMENT','-DPIONEER_QWEN35_PAIRED_RECURRENCE'])]:
        binary=d/suffix
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(d),'-I'+str(test),'-I'+str(misc),*defs,*extra,str(d/'test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
