/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#define main qwen35_original_pci_memory_test_main
#include "pioneer_qwen35_dataflow_pci_memory_test.cpp"
#undef main
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FreshResidentRecoveryIO.h"
#include <memory>
#include <vector>

struct FreshRaw : MockPciRaw {
    bool fleetEnabled=true;
    unsigned runtimeCore=0,accesses=0,panelWrites=0;
    uint32_t tlbs[2][3]{};
    std::vector<uint32_t> fleetPanel=std::vector<uint32_t>(120*DataflowPciL1ViewWords);
    uint32_t DramTlbRead(unsigned slot,unsigned word) { ++accesses;return tlbs[slot][word]; }
    void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) { ++accesses;tlbs[slot][word]=value; }
    uint32_t ResidentNocRead(unsigned offset) {
        ++accesses;
        return (offset&0xffffU)==PioneerQwen35::DataflowNocLogicalIdOffset
            ?PioneerQwen35::DataflowBarrierWorkerRoute(runtimeCore):0;
    }
    unsigned ResetRead() { selected=runtimeCore;++accesses;return MockPciRaw::ResetRead(); }
    uint32_t Q38PanelRead(unsigned address) {
        selected=runtimeCore;++accesses;assert(address>=DataflowPciL1ViewBase
            && address<DataflowPciL1ViewBase+DataflowPciL1ViewBytes);
        return fleetPanel[runtimeCore*DataflowPciL1ViewWords+(address-DataflowPciL1ViewBase)/4];
    }
    void Q38PanelWrite(unsigned address,uint32_t value) {
        selected=runtimeCore;++accesses;++panelWrites;++writes;
        assert(address>=DataflowPciL1ViewBase && address<DataflowPciL1ViewBase+DataflowPciL1ViewBytes);
        if (!dropPanelWrite) fleetPanel[runtimeCore*DataflowPciL1ViewWords+(address-DataflowPciL1ViewBase)/4]=value;
    }
    unsigned RuntimeCodeRead(unsigned word) { selected=runtimeCore;++accesses;return MockPciRaw::RuntimeCodeRead(word); }
    void RuntimeCodeWrite(unsigned word,unsigned value) {
        selected=runtimeCore;++accesses;MockPciRaw::RuntimeCodeWrite(word,value);
    }
};
struct AccessCheck {
    FreshRaw& fresh;
    bool allowed=true,denyAfterWrite=false;
    unsigned calls=0,accessesAtLoss=0;
    bool operator()() {
        ++calls;
        if (!allowed || (denyAfterWrite && fresh.panelWrites)) {
            if (allowed) accessesAtLoss=fresh.accesses;
            allowed=false;return false;
        }
        return true;
    }
};
static int FullFleetTest() {
    auto old=std::make_unique<MockPciRaw>();auto target=std::make_unique<FreshRaw>();
    auto retained=std::make_unique<PioneerQwen35DataflowPciMemory<MockPciRaw>>(*old);
    for (unsigned core=0;core<120;core++) {
        old->boot[core]=0x12000000U+core;old->panel[0]=0x34000000U+core;target->reset[core]=0x47800;
        assert(retained->CaptureCoreState(core));
    }
    AccessCheck allowed{*target};
    PioneerQ35FreshResidentRecoveryIO<MockPciRaw,FreshRaw,AccessCheck> io(*old,*target,allowed);
    auto fresh=std::make_unique<PioneerQwen35DataflowPciMemory<decltype(io)>>(io);
    auto snapshotCheck=[](unsigned core,const DataflowPciCoreSnapshot& snapshot) {
        return core<120 && snapshot.core==core && snapshot.captured && snapshot.localWords;
    };
    for (unsigned end=120;end;end--)
        assert(retained->FabricRestoreParentCore(end-1,*fresh,snapshotCheck));
    for (unsigned core=0;core<120;core++)
        assert(retained->FabricVerifyParentCore(core,*fresh,snapshotCheck) && target->boot[core]==old->boot[core]
            && target->fleetPanel[core*DataflowPciL1ViewWords]==0x34000000U+core);
    std::puts("PASS retained parent snapshots restored on all 120 simulated cores; NO HARDWARE");
    return 0;
}
int main(int argc,char**) {
    if (argc>1) return FullFleetTest();
    for (unsigned scenario=0;scenario<4;scenario++) {
        auto old=std::make_unique<MockPciRaw>();auto target=std::make_unique<FreshRaw>();
        auto retained=std::make_unique<PioneerQwen35DataflowPciMemory<MockPciRaw>>(*old);
        old->boot[3]=0x11223344;old->panel[0]=0x12345678;old->trustedImage[3]=true;
        assert(retained->CaptureCoreState(3) && retained->FabricParentSnapshotPresent(3)
            && !retained->FabricParentSnapshotAbsent(3) && retained->FabricParentSnapshotAbsent(4)
            && !retained->FabricParentSnapshotAbsent(120));
        target->reset[3]=0x47800;target->fleetPanel[3*DataflowPciL1ViewWords]=0x55555555;
        AccessCheck allowed{*target};
        PioneerQ35FreshResidentRecoveryIO<MockPciRaw,FreshRaw,AccessCheck> io(*old,*target,allowed);
        auto fresh=std::make_unique<PioneerQwen35DataflowPciMemory<decltype(io)>>(io);
        bool snapshotAuthority=scenario!=1;
        auto snapshotCheck=[&](unsigned core,const DataflowPciCoreSnapshot& snapshot) {
            return snapshotAuthority && core==3 && snapshot.core==3 && snapshot.captured && snapshot.localWords;
        };
        if (scenario==2) allowed.denyAfterWrite=true;
        if (scenario==3) target->dropPanelWrite=true;
        const bool restored=retained->FabricRestoreParentCore(3,*fresh,snapshotCheck);
        if (!scenario) {
            assert(restored && !old->trustedImage[3] && target->fleetPanel[3*DataflowPciL1ViewWords]==old->panel[0]
                && target->boot[3]==old->boot[3] && retained->FabricVerifyParentCore(3,*fresh,snapshotCheck));
            target->fleetPanel[3*DataflowPciL1ViewWords]^=1;
            assert(!retained->FabricVerifyParentCore(3,*fresh,snapshotCheck));
        } else {
            assert(!restored);
            if (scenario==1) assert(!target->accesses && old->trustedImage[3]);
            if (scenario==2) assert(target->panelWrites==1 && target->accesses==allowed.accessesAtLoss);
            const unsigned accesses=target->accesses;
            snapshotAuthority=true;allowed.allowed=true;allowed.denyAfterWrite=false;target->dropPanelWrite=false;
            if (scenario==1) {
                assert(retained->FabricRestoreParentCore(3,*fresh,snapshotCheck)
                    && retained->FabricVerifyParentCore(3,*fresh,snapshotCheck));
                continue; // denial before consumption may be retried
            }
            assert(!retained->FabricRestoreParentCore(3,*fresh,snapshotCheck)
                && !retained->FabricVerifyParentCore(3,*fresh,snapshotCheck) && target->accesses==accesses);
        }
    }
    std::puts("PASS retained parent snapshot restored through guarded fresh IO; SIMULATED ONLY");
}
