/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_PCI_IO_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_PCI_IO_H
#if !defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY) || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Role fleet PCI adapter is a non-loadable recovery witness only
#endif
#include "PioneerQwen35DataflowRoleRouteGuard.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleFleetRecoveryOwner.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleWritableViews.h"

// Adapter from the existing role recovery owner's exact ranges to the fixed
// Haiku BAR views. Scope must prove the caller already owns the driver lock,
// every relevant tile is held and quiet, private RAM is stable, and snapshots
// authorize the operation. This class does not establish those facts or reset
// a core. The real borrowed scope currently refuses every memory/control write.
template<class IO,class Scope,class Views> class PioneerQwen35RoleFleetPciIO {
    IO& fIO;
    Scope& fScope;
    Views& fViews;
    PioneerQwen35RoleRouteGuard<IO,Scope> fGuard;
    bool Select(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers && fScope.ProveExclusiveRewrite(fIO)
            // This prerequisite is independent of the selected TLB. Do not
            // reprogram any route if the whole-fleet proof is unavailable.
            && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
            && fIO.SelectRuntimeCore(core) && fIO.MapQ38Panel() && fIO.DataflowPanelMappingStable()
            && fGuard.ProveSelectedRoute(core);
    }
    bool ReadWords(unsigned core,unsigned which,unsigned offset,uint32_t* output,unsigned bytes) {
        using namespace PioneerQwen35;
        if (!output || !bytes || (offset&3) || (bytes&3) || offset>UINT32_MAX-bytes
            || !DataflowRolePhysicalReadAllowed(which,offset)
            || !DataflowRolePhysicalReadAllowed(which,offset+bytes-4) || !Select(core)) return false;
        for (unsigned n=0;n<bytes/4;n++)
            if (!fViews.Read(core,which,offset+n*4,output[n],fGuard)) return false;
        return true;
    }
    bool WriteWords(unsigned core,unsigned which,unsigned offset,const uint32_t* input,unsigned bytes) {
        return input && PioneerQwen35::DataflowRolePhysicalWriteRange(which,offset,bytes)
            && Select(core) && fViews.WriteWords(core,which,offset,input,bytes,fGuard);
    }
    // A selected-tile status scan is necessary but not sufficient: Scope
    // must separately prove no fleet-wide/incoming traffic can start.
    bool SelectedHeldAndQuiet(unsigned core) {
        using namespace PioneerQwen35;
        for (unsigned pass=0;pass<2;pass++) {
            if (fIO.ResetRead()!=0x47800 || fIO.ResetRead()!=0x47800) return false;
            for (unsigned noc=0;noc<2;noc++) {
                const unsigned base=noc?0x10000:0;
                uint32_t first=0,second=0;
                if (!DataflowNocDecodeLocalRoute(fIO.ResidentNocRead(base+DataflowNocLogicalIdOffset),first)
                    || !DataflowNocDecodeLocalRoute(fIO.ResidentNocRead(base+DataflowNocLogicalIdOffset),second)
                    || first!=second || !DataflowNocRouteMatchesWorker(first,core)) return false;
                for (unsigned i=0;i<4;i++)
                    if (fIO.ResidentNocRead(base+0x40+i*0x800)) return false;
                for (unsigned i=0;i<32;i++)
                    if (fIO.ResidentNocRead(base+0x240+i*4)&0xffU) return false;
            }
            if (fIO.ResetRead()!=0x47800 || !fGuard.ProveSelectedRoute(core)) return false;
            fIO.Pause();
        }
        return true;
    }
public:
    PioneerQwen35RoleFleetPciIO(IO& io,Scope& scope,Views& views)
        :fIO(io),fScope(scope),fViews(views),fGuard(io,scope) {}
    bool AllRolesHeld(unsigned core) {
        return Select(core) && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
            && SelectedHeldAndQuiet(core) && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
            && fGuard.ProveSelectedRoute(core);
    }
    bool DefaultBootRouting(unsigned core) {
        using namespace PioneerQwen35;
        if (!Select(core) || !fScope.ProveRoleDefaultBootRouting(fIO,core)) return false;
        uint32_t trisc1=0,trisc2=0,ncrisc1=0,ncrisc2=0;
        return fViews.Read(core,DataflowRolePhysicalControl,0x234,trisc1,fGuard)
            && fViews.Read(core,DataflowRolePhysicalControl,0x23c,ncrisc1,fGuard)
            && fViews.Read(core,DataflowRolePhysicalControl,0x234,trisc2,fGuard)
            && fViews.Read(core,DataflowRolePhysicalControl,0x23c,ncrisc2,fGuard)
            && trisc1==trisc2 && ncrisc1==ncrisc2 && !(trisc1&1U) && !(ncrisc1&1U)
            && fScope.ProveRoleDefaultBootRouting(fIO,core) && fGuard.ProveSelectedRoute(core);
    }
    bool LocalRamStable(unsigned core) {
        return Select(core) && fScope.ProveRoleLocalRamStable(fIO,core)
            && fGuard.ProveSelectedRoute(core);
    }
    bool Read(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        using namespace PioneerQwen35;
        return DataflowRoleExtraL1Range(address,bytes)
            && ReadWords(core,DataflowRolePhysicalExtraCode,address-DataflowRoleExtraL1Address,out,bytes);
    }
    bool Write(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        using namespace PioneerQwen35;
        return DataflowRoleExtraL1Range(address,bytes)
            && WriteWords(core,DataflowRolePhysicalExtraCode,address-DataflowRoleExtraL1Address,in,bytes);
    }
    bool ReadBoot(unsigned core,unsigned address,uint32_t& value) {
        using namespace PioneerQwen35;
        const unsigned which=address==DataflowRoleBriscBootAddress?DataflowRolePhysicalBriscBoot
            :address==DataflowRoleNcriscBootAddress?DataflowRolePhysicalNcriscBoot:DataflowRolePhysicalWindowCount;
        return ReadWords(core,which,0,&value,sizeof(value));
    }
    bool WriteBoot(unsigned core,unsigned address,uint32_t value) {
        using namespace PioneerQwen35;
        const unsigned which=address==DataflowRoleBriscBootAddress?DataflowRolePhysicalBriscBoot
            :address==DataflowRoleNcriscBootAddress?DataflowRolePhysicalNcriscBoot:DataflowRolePhysicalWindowCount;
        return WriteWords(core,which,0,&value,sizeof(value));
    }
    bool ReadShim(unsigned core,unsigned address,uint32_t* out,unsigned bytes) {
        using namespace PioneerQwen35;
        return address>=DataflowRoleReaderShimAddress && address<DataflowRoleWriterShimLimit
            && bytes<=DataflowRoleWriterShimLimit-address
            && ReadWords(core,DataflowRolePhysicalShim,address-DataflowRoleReaderShimAddress,out,bytes);
    }
    bool WriteShim(unsigned core,unsigned address,const uint32_t* in,unsigned bytes) {
        using namespace PioneerQwen35;
        return address>=DataflowRoleReaderShimAddress && address<DataflowRoleWriterShimLimit
            && bytes<=DataflowRoleWriterShimLimit-address
            && WriteWords(core,DataflowRolePhysicalShim,address-DataflowRoleReaderShimAddress,in,bytes);
    }
    bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* out,unsigned bytes) {
        using namespace PioneerQwen35;
        if (!DataflowRoleLocalRamRange(role,address,bytes)) return false;
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        return ReadWords(core,role==DataflowRoleReader?DataflowRolePhysicalBriscRam:DataflowRolePhysicalNcriscRam,
            address-base,out,bytes);
    }
    bool WriteRam(unsigned core,unsigned role,uint32_t address,const uint32_t* in,unsigned bytes) {
        using namespace PioneerQwen35;
        if (!DataflowRoleLocalRamRange(role,address,bytes)) return false;
        const uint32_t base=role==DataflowRoleReader?DataflowRoleBriscLocalRam:DataflowRoleNcriscLocalRam;
        return WriteWords(core,role==DataflowRoleReader?DataflowRolePhysicalBriscRam:DataflowRolePhysicalNcriscRam,
            address-base,in,bytes);
    }
    bool ReadResetDisable(unsigned core,uint32_t& value) {
        return ReadWords(core,PioneerQwen35::DataflowRolePhysicalControl,0x224,&value,sizeof(value));
    }
    bool WriteResetDisable(unsigned core,uint32_t desired) {
        uint32_t expected=0;
        return ReadResetDisable(core,expected) && Select(core)
            && fViews.WriteResetDisable(core,expected,desired,fGuard);
    }
};
#endif
