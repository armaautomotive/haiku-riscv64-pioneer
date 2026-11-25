/* SPDX-License-Identifier: MIT */
// Compose the actual 120-worker service state machines and host fleet audit
// with simulated NoC/panel IO. No graph tensors or hardware are executed.
#include "resident-service-run-fixture.inc"
#include "resident-fleet-fixture.inc"
int main() {
    (void)&q35_publisher_fixture_main;
    for (unsigned failure=0;failure<2;failure++) {
        auto workers=std::make_unique<ServiceFixture>(100);
        if (failure) workers->fabric->failGatherAfterDelivery=119;
        workers->Submit();
        for (unsigned round=0;round<300;round++) workers->Tick();
        assert(workers->state[0]==DataflowResidentServiceCandidate);
        auto audit=std::make_unique<DataflowResidentFleetAudit>();
        auto fleet=std::make_unique<FleetIO>(workers->command);
        for (unsigned core=0;core<DataflowCoreCount;core++) fleet->sample[core].sender=workers->fabric->sender[core];
        const auto result=audit->Audit(workers->command,*fleet);
        if (!failure) {
            assert(result==DataflowResidentFleetAuditComplete && audit->AuditedFor(workers->command));
        } else {
            // The root received the final gather despite the sender's
            // reported failure. Its missing local settlement marker prevents
            // the host from accepting the otherwise ready-looking fleet.
            assert(workers->state[119]==DataflowResidentServiceRetained
                && result==DataflowResidentFleetAuditRetained && audit->FailedCore()==119 && fleet->retained);
        }
    }
    puts("PASS delivered-but-failed gather cannot pass sender-receipt fleet audit; SIMULATED ONLY");
}
