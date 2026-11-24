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
static void PrepareClose(PioneerPostResetMapping& mapping) {
    Prepare(mapping);Permit fleetPermit;PioneerPostResetAuditResult fleet;
    assert(mapping.AuditQ35Fabric(fleet,fleetPermit));
    Permit l2Permit;PioneerQ35PostResetL2Result l2;
    assert(mapping.AuditQ35L2CpuHeldGuarded(l2,l2Permit));
    Permit dmaPermit;assert(mapping.AuditQ35HostDmaGuarded(dmaPermit));
    deviceAccesses=accessesAfterRevoke=0;unmapCalls=0;
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
    unsigned closeTotal=0;
    {
        PioneerPostResetMapping mapping(profile);PrepareClose(mapping);Permit permit;
        mapping.queueUnsafe=true;assert(!mapping.CloseQ35Guarded(permit) && !mapping.q35CloseAttempted && !deviceAccesses);
        mapping.queueUnsafe=false;
        assert(mapping.CloseQ35Guarded(permit) && mapping.q35CloseComplete && !mapping.ready
            && !mapping.modified && !mapping.dmaModified && unmapCalls==7 && !accessesAfterRevoke);
        assert(tlbPage[0x96c/4]==7 && tlbPage[0x970/4]==8 && tlbPage[0x974/4]==9
            && tlbPage[0x960/4]==10 && tlbPage[0x964/4]==11 && tlbPage[0x968/4]==12);
        closeTotal=permit.calls;assert(closeTotal>100);
    }
    const unsigned closeFailures[]={1,2,3,17,50,closeTotal/4,closeTotal/2,closeTotal*3/4,closeTotal-1,closeTotal};
    for (unsigned mode=0;mode<2;mode++) for (unsigned failure:closeFailures) {
        PioneerPostResetMapping mapping(profile);PrepareClose(mapping);Permit permit;
        permit.failAt=failure;permit.transient=mode!=0;
        assert(!mapping.CloseQ35Guarded(permit) && permitRevoked && !mapping.q35CloseComplete && !accessesAfterRevoke);
        if (mapping.q35CloseAttempted) {
            const unsigned accesses=deviceAccesses,unmaps=unmapCalls;
            permitRevoked=false;assert(!mapping.CloseQ35Guarded(permit) && !mapping.Close()
                && deviceAccesses==accesses && unmapCalls==unmaps);
        }
    }
    for (int failure=0;failure<7;failure++) {
        PioneerPostResetMapping mapping(profile);PrepareClose(mapping);Permit permit;failUnmap=failure;
        assert(!mapping.CloseQ35Guarded(permit) && mapping.q35CloseAttempted && !mapping.q35CloseComplete
            && unmapCalls==unsigned(failure+1));
        const unsigned accesses=deviceAccesses,unmaps=unmapCalls;failUnmap=-1;
        assert(!mapping.CloseQ35Guarded(permit) && !mapping.Close()
            && deviceAccesses==accesses && unmapCalls==unmaps);
    }
    for (unsigned slot=0;slot<2;slot++) {
        PioneerPostResetMapping mapping(profile);PrepareClose(mapping);Permit permit;
        if (slot==0) blockDbiTlbRestore=true;else blockTlbRestore=true;
        assert(!mapping.CloseQ35Guarded(permit) && mapping.q35CloseAttempted && !mapping.q35CloseComplete
            && !unmapCalls);
        const unsigned accesses=deviceAccesses;blockDbiTlbRestore=blockTlbRestore=false;
        assert(!mapping.CloseQ35Guarded(permit) && !mapping.Close() && deviceAccesses==accesses && !unmapCalls);
    }
    for (unsigned fault=0;fault<3;fault++) {
        PioneerPostResetMapping mapping(profile);PrepareClose(mapping);Permit permit;
        if (fault==0) mapping.areas[3]=mapping.areas[2];
        else if (fault==1) mapping.areas[3]=-1;
        else mapping.dbiArea=mapping.areas[2];
        assert(!mapping.CloseQ35Guarded(permit) && !mapping.q35CloseAttempted
            && !deviceAccesses && !unmapCalls);
    }
    puts("PASS Q35 guarded fleet, L2, DMA and mapping close; SIMULATED ONLY");
}
