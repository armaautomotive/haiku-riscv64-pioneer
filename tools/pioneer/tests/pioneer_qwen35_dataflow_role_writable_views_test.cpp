/* SPDX-License-Identifier: MIT */
// Tests actual Haiku map flags with simulated memory, routes and permissions.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY 1
#include <KernelExport.h>
#include "../PioneerQwen35DataflowRoleWritableViews.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowRoleHaikuMap.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
static constexpr uint64_t BarStart=0x4900000000ULL,BarEnd=0x491fffffffULL;
static unsigned maps=0,unmaps=0;
static int failMap=-1,nullMap=-1,failUnmap=-1;
static std::vector<std::vector<uint32_t>> memory;
static std::vector<bool> mapped;
area_id map_physical_memory(const char*,phys_addr_t physical,size_t bytes,uint32_t addressMode,
    uint32_t flags,void** output) {
    ++maps;
    unsigned i=0; DataflowRolePhysicalWindow plan{};
    for (;i<=DataflowRolePhysicalControl;i++) {
        assert(DataflowRolePhysicalWindowPlan(i,plan));
        if (physical==BarStart+plan.offset) break;
    }
    assert(i<=DataflowRolePhysicalControl && bytes==plan.bytes && addressMode==B_ANY_KERNEL_ADDRESS);
    assert((flags&B_KERNEL_READ_AREA) && (flags&B_KERNEL_WRITE_AREA) && (flags&B_UNCACHED_MEMORY));
    if (int(i)==failMap) return -1;
    assert(!mapped[i]);
    if (memory[i].empty()) memory[i].assign(bytes/4,0);
    assert(memory[i].size()==bytes/4); mapped[i]=true;
    *output=int(i)==nullMap?nullptr:memory[i].data(); return int32_t(i);
}
status_t delete_area(area_id area) {
    assert(area>=0 && unsigned(area)<mapped.size() && mapped[area]);
    ++unmaps;
    if (area==failUnmap) { failUnmap=-1; return -1; }
    mapped[area]=false; return B_OK;
}
struct Guard {
    bool pinned=true,route=true,read=true,write=true,control=true;
    bool ProvePinnedBar(uint64_t start,uint64_t end) { return pinned && start==BarStart && end==BarEnd; }
    bool ProveSelectedRoute(unsigned core) { return route && core==3; }
    bool ProveReadAllowed(unsigned,unsigned,unsigned) { return read; }
    bool ProveWriteAllowed(unsigned,unsigned,unsigned,unsigned) { return write; }
    bool ProveControlWriteAllowed(unsigned,uint32_t,uint32_t) { return control; }
};
struct Platform {
    PioneerQwen35RoleHaikuMap actual;
    Guard* guard=nullptr;
    unsigned writes=0,driftAfter=0,reads=0,driftAfterReads=0;
    bool corruptReadback=false;
    bool MapReadWrite(uint64_t p,unsigned n,int32_t& a,volatile uint32_t*& v)
        { return actual.MapReadWrite(p,n,a,v); }
    bool Unmap(int32_t area) { return actual.Unmap(area); }
    uint32_t ReadWord(const volatile uint32_t* p) {
        const uint32_t value=actual.ReadWord(p);
        ++reads;
        if (driftAfterReads && reads==driftAfterReads) guard->route=false;
        return corruptReadback?value^1U:value;
    }
    void WriteWord(volatile uint32_t* p,uint32_t value) {
        actual.WriteWord(p,value); ++writes;
        if (driftAfter && writes==driftAfter) guard->route=false;
    }
};
static void Reset() {
    maps=unmaps=0; failMap=nullMap=failUnmap=-1;
    memory.assign(DataflowRolePhysicalControl+1,{});
    mapped.assign(DataflowRolePhysicalControl+1,false);
}
int main()
{
    {
        Reset(); Guard guard; Platform platform; platform.guard=&guard;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(BarStart,BarEnd,guard) && maps==DataflowRolePhysicalControl+1);
        uint32_t value=99,words[]={0x11223344,0xaabbccdd};
        const unsigned extraTail=DataflowRoleExtraL1Limit-DataflowRoleExtraL1Address-8;
        assert(views.Read(3,DataflowRolePhysicalBriscBoot,0,value,guard) && !value);
        assert(views.WriteWords(3,DataflowRolePhysicalBriscBoot,0,words,4,guard));
        assert(views.WriteWords(3,DataflowRolePhysicalNcriscBoot,0,words+1,4,guard));
        assert(views.WriteWords(3,DataflowRolePhysicalShim,0xf8,words,8,guard));
        assert(views.WriteWords(3,DataflowRolePhysicalExtraCode,extraTail,words,8,guard));
        assert(views.WriteWords(3,DataflowRolePhysicalBriscRam,0x1ff8,words,8,guard));
        assert(views.WriteWords(3,DataflowRolePhysicalNcriscRam,0x1ff8,words,8,guard));
        assert(views.Matches(3,DataflowRolePhysicalExtraCode,extraTail,words,2,guard));
        uint32_t mismatch[]={words[0],words[1]^1};
        assert(!views.Matches(3,DataflowRolePhysicalExtraCode,extraTail,mismatch,2,guard));
        assert(!views.Matches(3,DataflowRolePhysicalBriscRam,0x1ff8,words,2,guard));
        assert(!views.Matches(3,DataflowRolePhysicalExtraCode,extraTail+4,words,2,guard));
        guard.read=false; assert(!views.Matches(3,DataflowRolePhysicalExtraCode,extraTail,words,2,guard));
        guard.read=true; platform.driftAfterReads=platform.reads+1;
        assert(!views.Matches(3,DataflowRolePhysicalExtraCode,extraTail,words,2,guard));
        assert(!guard.route); guard.route=true; platform.driftAfterReads=0;
        std::vector<uint32_t> longImage(65,0x72341609U);
        for (unsigned i=0;i<longImage.size();i++) memory[DataflowRolePhysicalExtraCode][i]=longImage[i];
        assert(views.Matches(3,DataflowRolePhysicalExtraCode,0,longImage.data(),longImage.size(),guard));
        longImage[64]^=1;
        assert(!views.Matches(3,DataflowRolePhysicalExtraCode,0,longImage.data(),longImage.size(),guard));
        memory[DataflowRolePhysicalExtraCode][0]=memory[DataflowRolePhysicalExtraCode][1]=0;
        const unsigned before=platform.writes;
        assert(!views.WriteWords(3,DataflowRolePhysicalBriscBoot,0,words,8,guard));
        assert(!views.WriteWords(3,DataflowRolePhysicalShim,0xfc,words,8,guard));
        assert(!views.WriteWords(3,DataflowRolePhysicalExtraCode,extraTail+4,words,8,guard));
        assert(!views.WriteWords(3,DataflowRolePhysicalControl,0x1b0,words,4,guard));
        assert(!views.WriteWords(3,DataflowRolePhysicalControl,0x224,words,4,guard));
        assert(!views.WriteWords(3,DataflowRolePhysicalPc,0x138,words,4,guard));
        assert(!views.WriteWords(3,DataflowRolePhysicalNoc0,0x40,words,4,guard));
        assert(platform.writes==before);
        memory[DataflowRolePhysicalControl][0x224/4]=0x15;
        assert(!views.WriteResetDisable(3,0x14,0x3d4,guard));
        assert(!views.WriteResetDisable(3,0x15,0x3d4,guard)); // changes unrelated bit
        guard.control=false; assert(!views.WriteResetDisable(3,0x15,0x3d5,guard));
        guard.control=true; assert(views.WriteResetDisable(3,0x15,0x3d5,guard));
        assert(memory[DataflowRolePhysicalControl][0x224/4]==0x3d5);
        assert(views.WriteResetDisable(3,0x3d5,0x15,guard));
        guard.write=false; assert(!views.WriteWords(3,DataflowRolePhysicalExtraCode,0,words,4,guard));
        guard.write=true; platform.driftAfter=platform.writes+1;
        assert(!views.WriteWords(3,DataflowRolePhysicalExtraCode,0,words,8,guard));
        assert(memory[DataflowRolePhysicalExtraCode][0]==words[0]
            && memory[DataflowRolePhysicalExtraCode][1]==0);
        guard.route=true; platform.driftAfter=0; platform.corruptReadback=true;
        assert(!views.WriteWords(3,DataflowRolePhysicalExtraCode,8,words,4,guard));
        platform.corruptReadback=false;
        assert(views.Close() && !views.HasMappings() && unmaps==DataflowRolePhysicalControl+1);
    }
    {
        Reset(); Guard guard; Platform platform; platform.guard=&guard;
        DataflowRoleWritableViews<Platform> views(platform);
        failMap=4;
        assert(!views.Open(BarStart,BarEnd,guard) && views.HasMappings());
        failUnmap=2; assert(!views.Close() && views.HasMappings());
        assert(views.Close() && !views.HasMappings());
    }
    {
        Reset(); Guard guard; Platform platform; platform.guard=&guard;
        DataflowRoleWritableViews<Platform> views(platform);
        nullMap=3;
        assert(!views.Open(BarStart,BarEnd,guard) && views.HasMappings());
        assert(views.Close() && !views.HasMappings() && unmaps==4);
    }
    {
        Reset(); Guard guard; Platform platform; platform.guard=&guard;
        DataflowRoleWritableViews<Platform> views(platform);
        const uint32_t word=0xdeadbeefU;
        assert(views.Open(BarStart,BarEnd,guard));
        assert(views.WriteWords(3,DataflowRolePhysicalExtraCode,0,&word,4,guard));
        failUnmap=2; assert(!views.Close() && views.HasMappings() && !views.Opened());
        guard.pinned=false; assert(!views.ReopenForRecovery(guard)); guard.pinned=true;
        failMap=4; assert(!views.ReopenForRecovery(guard)); failMap=-1;
        nullMap=5; assert(!views.ReopenForRecovery(guard)); nullMap=-1;
        assert(views.ReopenForRecovery(guard) && views.Opened());
        uint32_t value=0;
        assert(views.Read(3,DataflowRolePhysicalExtraCode,0,value,guard) && value==word);
        assert(views.Close() && !views.HasMappings());
    }
    std::puts("PASS scoped writable role views, reset-mask restrictions and cleanup faults; SIMULATED ONLY");
}
