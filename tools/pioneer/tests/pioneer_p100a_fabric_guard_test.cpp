/* SPDX-License-Identifier: MIT */
// Exercise the actual experimental guard export with simulated kernel state.
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#include "../../../src/add-ons/kernel/drivers/misc/pioneer_p100a_reload_guard.cpp"
#include <cstdio>
#include <cstring>

static PioneerDriverFabricRecoveryProof FabricProof(uint64 ticket) {
    PioneerDriverFabricRecoveryProof p{};
    p.flags=PIONEER_FABRIC_PROOF_REQUIRED;p.core_count=120;
    p.fabric_generation=ticket;p.firmware_generation=ticket;return p;
}
int main() {
    assert(sFabricRecoveryWitness.info.std_ops(B_MODULE_INIT)==B_OK);
    assert(modules[0]==&sGuard.info && modules[1]==&sRecoveryGuard.info
        && modules[2]==&sRecoveryGuardV2.info && modules[3]==&sFabricRecoveryWitness.info
        && modules[4]==nullptr);
    uint64 generation=0,ticket=0,next=0;
    assert(sGuard.attach(&generation)==B_OK && generation);
    sGuard.fault(generation);
    assert(sFabricRecoveryWitness.begin(generation,&ticket)==B_OK && ticket);
    auto proof=FabricProof(ticket);
    auto partial=proof;
    partial.flags &= ~(PIONEER_FABRIC_PROOF_OWNERS_RELEASED|PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED
        |PIONEER_FABRIC_PROOF_MATRIX_VERIFIED);
    partial.retained_owners=1;
    assert(partial.ReadyToRetireOneOwner() && !partial.Complete());
    for (unsigned bit=0;bit<11;bit++) {
        auto invalid=proof;invalid.flags^=1U<<bit;
        assert(sFabricRecoveryWitness.complete(ticket,&invalid,&next)==B_DEV_NOT_READY && !next && Cold());
    }
    for (unsigned fault=0;fault<8;fault++) {
        auto invalid=proof;
        switch (fault) {
        case 0: invalid.version=1;break;
        case 1: invalid.core_count=119;break;
        case 2: invalid.reserved=1;break;
        case 3: invalid.noc_busy_mask=1;break;
        case 4: invalid.noc_outstanding_mask=1;break;
        case 5: invalid.host_dma_active=1;break;
        case 6: invalid.retained_owners=1;break;
        case 7: invalid.flags|=1U<<31;break;
        }
        assert(sFabricRecoveryWitness.complete(ticket,&invalid,&next)==B_DEV_NOT_READY && !next && Cold());
    }
    auto invalid=proof;invalid.fabric_generation++;
    assert(sFabricRecoveryWitness.complete(ticket,&invalid,&next)==B_DEV_NOT_READY && !next && Cold());
    invalid=proof;invalid.firmware_generation++;
    assert(sFabricRecoveryWitness.complete(ticket,&invalid,&next)==B_DEV_NOT_READY && !next && Cold());
    invalid=proof;invalid.version=0x51463301U; // previous contract had no matrix evidence
    assert(sFabricRecoveryWitness.complete(ticket,&invalid,&next)==B_DEV_NOT_READY && !next && Cold());
    assert(sFabricRecoveryWitness.complete(ticket+1,&proof,&next)==B_DEV_NOT_READY && !next && Cold());
    // Identical-sized evidence cannot cross the existing ASIC-reset API.
    PioneerDriverRecoveryProof wrongType{};memcpy(&wrongType,&proof,sizeof(wrongType));
    assert(sRecoveryGuardV2.complete(ticket,&wrongType,&next)==B_DEV_NOT_READY && !next && Cold());
    PioneerDriverRecoveryProof asic{};asic.flags=PIONEER_RECOVERY_PROOF_REQUIRED;
    asic.core_count=120;asic.reset_generation=ticket;asic.firmware_generation=ticket;
    PioneerDriverFabricRecoveryProof wrongFabric{};memcpy(&wrongFabric,&asic,sizeof(wrongFabric));
    assert(sFabricRecoveryWitness.complete(ticket,&wrongFabric,&next)==B_DEV_NOT_READY && !next && Cold());
    assert(sFabricRecoveryWitness.complete(ticket,nullptr,&next)==B_BAD_VALUE);
    assert(sFabricRecoveryWitness.complete(0,&proof,&next)==B_BAD_VALUE);
    assert(sFabricRecoveryWitness.complete(ticket,&proof,&next)==B_OK && next==generation+1 && !Cold());
    assert(sRecoveryGuardV2.validate(ticket)==B_DEV_NOT_READY);
    generation=next;sGuard.fault(generation);
    assert(sFabricRecoveryWitness.begin(generation,&ticket)==B_OK);
    proof=FabricProof(ticket);sGuard.fault(generation);
    assert(sFabricRecoveryWitness.complete(ticket,&proof,&next)==B_DEV_NOT_READY && !next && Cold());
    assert(sFabricRecoveryWitness.abort(ticket)==B_OK);
    assert(sFabricRecoveryWitness.begin(generation,&ticket)==B_OK);
    proof=FabricProof(ticket);injectAtomicFault=true;
    assert(sFabricRecoveryWitness.complete(ticket,&proof,&next)==B_DEV_NOT_READY
        && !next && Cold() && sState.generation==generation);
    assert(sFabricRecoveryWitness.begin(generation,&ticket)==B_OK);
    assert(sFabricRecoveryWitness.abort(ticket)==B_OK && Cold());
    puts("PASS distinct fabric guard proof and fault epochs; SIMULATED ONLY");
}
