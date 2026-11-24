/* SPDX-License-Identifier: MIT */
// The real PCI L1 writer and resident installer over simulated held workers.
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define main q35_pci_fixture_main
#include "pioneer_qwen35_dataflow_pci_memory_test.cpp"
#undef main
#define Q35_PUBLISHER_FIXTURE_ONLY
#include "pioneer_qwen35_graph_publication_test.cpp"
#include "../PioneerQwen35DataflowResidentTemplateInstaller.h"

struct TemplateRaw : MockPciRaw {
    uint32_t descriptor[DataflowCoreCount][DataflowGraphRunBytes/4]{};
    int dropCore=-1;bool crossCorrupt=false;
    TemplateRaw() { for (unsigned i=0;i<DataflowCoreCount;i++) reset[i]=0x47800; }
    uint32_t Q38PanelRead(unsigned address) const {
        if (address>=DataflowGraphRunAddress && address<DataflowGraphRunAddress+DataflowGraphRunBytes)
            return descriptor[selected][(address-DataflowGraphRunAddress)/4];
        return MockPciRaw::Q38PanelRead(address);
    }
    void Q38PanelWrite(unsigned address,uint32_t value) {
        if (address>=DataflowGraphRunAddress && address<DataflowGraphRunAddress+DataflowGraphRunBytes) {
            const unsigned index=(address-DataflowGraphRunAddress)/4;
            if (int(selected)!=dropCore || index) descriptor[selected][index]=value;
            ++writes;
            if (crossCorrupt && selected==119 && index==DataflowGraphRunBytes/4-1)
                descriptor[0][0]^=1;
            return;
        }
        MockPciRaw::Q38PanelWrite(address,value);
    }
};
struct TemplateIO {
    TemplateRaw& raw;PioneerQwen35DataflowPciMemory<TemplateRaw> pci;
    const DataflowStreamPlan* expected;bool owner=true,allHeld=true,retained=false;
    explicit TemplateIO(TemplateRaw& r,const DataflowStreamPlan& p):raw(r),pci(r),expected(&p) {}
    bool ResidentTemplateOwnerAllowed(const DataflowStreamPlan& p,const DataflowDynamicTokenContext&,unsigned slot) {
        return owner && &p==expected && slot==0;
    }
    bool ResidentTemplateFleetHeld() { return allHeld; }
    bool ResidentTemplateCoreHeld(unsigned core) { return pci.CoreHeld(core); }
    void RetainResidentTemplates() { retained=true; }
    bool WriteCoreMemory(unsigned core,unsigned address,const void* data,unsigned bytes) {
        return pci.WriteCoreMemory(core,address,data,bytes);
    }
    bool ReadCoreMemory(unsigned core,unsigned address,void* data,unsigned bytes) {
        return pci.ReadCoreMemory(core,address,data,bytes);
    }
};
int main() {
    (void)&q35_pci_fixture_main;(void)&q35_publisher_fixture_main;
    auto f=std::make_unique<Fixture>();auto plan=std::make_unique<DataflowStreamPlan>();
    assert(DataflowStreamPlanBuilder::Build(f->packed,f->manifest,f->workspace,f->rope,f->routes,
        99,DataflowGraphDecodeSteps,*plan));
    DataflowDynamicTokenContext seed{};assert(DataflowDynamicTokenContext::Build(f->manifest,0,0,seed));
    {
        auto raw=std::make_unique<TemplateRaw>();TemplateIO io(*raw,*plan);DataflowResidentTemplateInstaller installer;
        assert(installer.Install(*plan,f->routes,seed,0,100,io)==DataflowResidentTemplateInstalled
            && installer.IsInstalled() && raw->writes==DataflowCoreCount*(DataflowGraphRunBytes/4));
        for (unsigned core=0;core<DataflowCoreCount;core++) {
            DataflowGraphRunDescriptor descriptor{};std::memcpy(&descriptor,raw->descriptor[core],sizeof(descriptor));
            assert(DataflowGraphRunValid(descriptor,core) && descriptor.token.token==0 && descriptor.token.position==0);
        }
        assert(installer.VerifyBeforeRelease(io));
        raw->descriptor[37][0]^=1;
        assert(!installer.VerifyBeforeRelease(io) && installer.IsRetained() && io.retained
            && installer.FailedCore()==37);
        assert(installer.Install(*plan,f->routes,seed,0,100,io)==DataflowResidentTemplateInvalid);
    }
    {
        auto raw=std::make_unique<TemplateRaw>();TemplateIO io(*raw,*plan);DataflowResidentTemplateInstaller installer;
        std::swap(plan->binding[118],plan->binding[119]);std::swap(plan->lease[118],plan->lease[119]);
        assert(installer.Install(*plan,f->routes,seed,0,100,io)==DataflowResidentTemplatePreflight
            && installer.FailedCore()==118 && !raw->writes);
        std::swap(plan->binding[118],plan->binding[119]);std::swap(plan->lease[118],plan->lease[119]);
    }
    {
        auto raw=std::make_unique<TemplateRaw>();TemplateIO io(*raw,*plan);DataflowResidentTemplateInstaller installer;
        raw->dropCore=37;
        assert(installer.Install(*plan,f->routes,seed,0,100,io)==DataflowResidentTemplateRetained
            && installer.IsRetained() && installer.FailedCore()==37 && io.retained);
        const unsigned writes=raw->writes;
        assert(installer.Install(*plan,f->routes,seed,0,100,io)==DataflowResidentTemplateInvalid && raw->writes==writes);
    }
    {
        auto raw=std::make_unique<TemplateRaw>();TemplateIO io(*raw,*plan);DataflowResidentTemplateInstaller installer;
        raw->crossCorrupt=true;
        assert(installer.Install(*plan,f->routes,seed,0,100,io)==DataflowResidentTemplateRetained
            && installer.FailedCore()==0 && installer.IsRetained() && io.retained);
    }
    puts("PASS held resident graph templates with real PCI L1 writer; SIMULATED ONLY");
}
