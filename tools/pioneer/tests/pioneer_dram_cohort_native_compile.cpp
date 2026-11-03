/* SPDX-License-Identifier: MIT */
// Actual kernel SDK instantiation ONLY,never linked/installed/run.
#define PIONEER_DRAM_COHORT_NATIVE_CONTEXT 1
#define PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT 1
#define PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
extern "C" {
 extern int32 pioneer_dram_cohort_api_version;
 status_t pioneer_dram_cohort_init_hardware();status_t pioneer_dram_cohort_init_driver();
 void pioneer_dram_cohort_uninit_driver();const char** pioneer_dram_cohort_publish_devices();
 device_hooks* pioneer_dram_cohort_find_device(const char*);
}
#define api_version pioneer_dram_cohort_api_version
#define init_hardware pioneer_dram_cohort_init_hardware
#define init_driver pioneer_dram_cohort_init_driver
#define uninit_driver pioneer_dram_cohort_uninit_driver
#define publish_devices pioneer_dram_cohort_publish_devices
#define find_device pioneer_dram_cohort_find_device
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
// Real private kernel admission,not fixture-set ready fields. Uncalled.
extern "C" status_t pioneer_dram_cohort_private_kernel_initialize(){
 if(sPCI || sReloadGuard || sLock>=0 || sDramSweepNativeContext || sDramCohortNativeContext)return B_BUSY;
 status_t s=get_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE,(module_info**)&sReloadGuard);if(s!=B_OK)return s;
 if(sReloadGuard->version!=1)s=B_BAD_VALUE;else s=sReloadGuard->attach(&sReloadGeneration);
 if(s!=B_OK){put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE);sReloadGuard=NULL;return s;}
 s=get_module(B_PCI_MODULE_NAME,(module_info**)&sPCI);
 if(s==B_OK && !FindCard(sPCI,sCard))s=B_ENTRY_NOT_FOUND;
 if(s==B_OK){sLock=create_sem(1,"P100A private eight-participant cohort");if(sLock<0)s=sLock;}
 if(s==B_OK)return B_OK;
 if(sPCI){put_module(B_PCI_MODULE_NAME);sPCI=NULL;}
 const status_t detached=sReloadGuard->detach(sReloadGeneration,true);
 if(detached!=B_OK)sReloadGuard->fault(sReloadGeneration);
 put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE);sReloadGuard=NULL;return s;
}
extern "C" status_t pioneer_dram_cohort_private_open(const char* name,uint32 flags,void** cookie){return ProbeOpen(name,flags,cookie);}
extern "C" status_t pioneer_dram_cohort_private_run(PioneerProbeCookie* fd,unsigned sweeps,unsigned depth,uint32 nonce,PioneerDramCohortNativeReply& output){
 return RunPioneerDramCohortNative(fd,sweeps,depth,nonce,output);
}
extern "C" status_t pioneer_dram_cohort_private_control(void* fd,uint32 op,void* data,size_t size){return ProbeControl(fd,op,data,size);}
extern "C" status_t pioneer_dram_cohort_expected_refusal(){return B_NOT_ALLOWED;}
extern "C" void* pioneer_dram_cohort_expected_null(){return NULL;}
extern "C" void pioneer_dram_cohort_expected_inert(){}
