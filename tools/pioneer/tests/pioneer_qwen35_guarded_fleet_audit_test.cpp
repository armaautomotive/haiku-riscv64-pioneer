/* SPDX-License-Identifier: MIT */
#define TEST_Q35_FABRIC_AUDIT 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define main pioneer_post_reset_mapping_test_unused_main
#include "pioneer_post_reset_mapping_test.cpp"
#undef main

struct Permit {
    unsigned calls=0,failAt=0;bool transient=false;
    bool operator()() {
        ++calls;if (failAt && calls==failAt) { permitRevoked=true;return false; }
        return transient || !permitRevoked;
    }
};
static void Prepare(PioneerPostResetMapping& mapping) {
    tt_arc_scratch_result arc{};
    permitRevoked=false;ResetFake();assert(mapping.Open() && mapping.ProbeFirmware(3,arc));
    deviceAccesses=accessesAfterRevoke=0;
}
int main()
{
    tt_pioneer_profile profile;unsigned total=0;
    {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit permit;
        PioneerPostResetAuditResult result;
        assert(mapping.AuditQ35Fabric(result,permit) && mapping.fleetAudited && !mapping.dmaAudited
            && result.audited==240 && !accessesAfterRevoke);
        total=permit.calls;assert(total>1000 && deviceAccesses>1000);
    }
    const unsigned failures[]={1,2,3,17,50,100,total/4,total/2,total*3/4,total-1,total};
    for (unsigned mode=0;mode<2;mode++) for (unsigned failure:failures) {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit permit;
        permit.failAt=failure;permit.transient=mode!=0;PioneerPostResetAuditResult result;
        assert(!mapping.AuditQ35Fabric(result,permit) && permitRevoked && !mapping.fleetAudited
            && !mapping.dmaAudited && !accessesAfterRevoke);
    }
    {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit permit;wrongNode=true;
        PioneerPostResetAuditResult result;
        assert(!mapping.AuditQ35Fabric(result,permit) && !mapping.fleetAudited
            && result.failure==PioneerPostResetAuditSelect);
    }
    {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit permit;
        nocPage[(0x10000+0x240)/4]=1;PioneerPostResetAuditResult result;
        assert(!mapping.AuditQ35Fabric(result,permit) && !mapping.fleetAudited
            && result.failure==PioneerPostResetAuditOutstanding && result.noc==1);
    }
    unsigned l2Total=0;
    {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit fleetPermit;PioneerPostResetAuditResult fleet;
        assert(mapping.AuditQ35Fabric(fleet,fleetPermit));
        uint32 saved[3];for (unsigned i=0;i<3;i++) saved[i]=mapping.TlbRead(i);
        Permit permit;PioneerQ35PostResetL2Result result;deviceAccesses=accessesAfterRevoke=0;
        assert(mapping.AuditQ35L2CpuHeldGuarded(result,permit) && result.sampled && !result.restoreFailed
            && !accessesAfterRevoke);
        for (unsigned i=0;i<3;i++) assert(mapping.TlbRead(i)==saved[i]);
        l2Total=permit.calls;assert(l2Total>100 && deviceAccesses>10);
    }
    const unsigned l2Failures[]={1,2,3,17,50,l2Total/4,l2Total/2,l2Total*3/4,l2Total-1,l2Total};
    for (unsigned mode=0;mode<2;mode++) for (unsigned failure:l2Failures) {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit fleetPermit;PioneerPostResetAuditResult fleet;
        assert(mapping.AuditQ35Fabric(fleet,fleetPermit));
        Permit permit;permit.failAt=failure;permit.transient=mode!=0;
        PioneerQ35PostResetL2Result result;deviceAccesses=accessesAfterRevoke=0;
        assert(!mapping.AuditQ35L2CpuHeldGuarded(result,permit) && permitRevoked && !accessesAfterRevoke);
    }
    unsigned dmaTotal=0;
    {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit fleetPermit;PioneerPostResetAuditResult fleet;
        assert(mapping.AuditQ35Fabric(fleet,fleetPermit));Permit l2Permit;PioneerQ35PostResetL2Result l2;
        assert(mapping.AuditQ35L2CpuHeldGuarded(l2,l2Permit));
        Permit permit;deviceAccesses=accessesAfterRevoke=0;
        assert(mapping.AuditQ35HostDmaGuarded(permit) && mapping.dmaAudited && mapping.dmaModified
            && !mapping.dmaDiagnostic && !accessesAfterRevoke);
        dmaTotal=permit.calls;assert(dmaTotal>100 && deviceAccesses>10);
    }
    const unsigned dmaFailures[]={1,2,3,17,50,dmaTotal/4,dmaTotal/2,dmaTotal*3/4,dmaTotal-1,dmaTotal};
    for (unsigned mode=0;mode<2;mode++) for (unsigned failure:dmaFailures) {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit fleetPermit;PioneerPostResetAuditResult fleet;
        assert(mapping.AuditQ35Fabric(fleet,fleetPermit));Permit l2Permit;PioneerQ35PostResetL2Result l2;
        assert(mapping.AuditQ35L2CpuHeldGuarded(l2,l2Permit));
        Permit permit;permit.failAt=failure;permit.transient=mode!=0;deviceAccesses=accessesAfterRevoke=0;
        assert(!mapping.AuditQ35HostDmaGuarded(permit) && permitRevoked && !mapping.dmaAudited && !accessesAfterRevoke);
    }
    {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit fleetPermit;PioneerPostResetAuditResult fleet;
        assert(mapping.AuditQ35Fabric(fleet,fleetPermit));hdmaPage[0x100/4]=1;Permit permit;
        assert(!mapping.AuditQ35HostDmaGuarded(permit) && !mapping.dmaAudited && mapping.dmaDiagnostic==0x54001);
    }
    {
        PioneerPostResetMapping mapping(profile);Prepare(mapping);Permit fleetPermit;PioneerPostResetAuditResult fleet;
        assert(mapping.AuditQ35Fabric(fleet,fleetPermit));fakePciPendingPeer=1;Permit permit;
        assert(!mapping.AuditQ35HostDmaGuarded(permit) && !mapping.dmaAudited && mapping.dmaDiagnostic==0x48020);
    }
    puts("PASS Q35 fleet, L2 and DMA audits: ticket checked around simulated PCI/MMIO accesses; SIMULATED ONLY");
}
