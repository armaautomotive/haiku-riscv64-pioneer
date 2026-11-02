/* SPDX-License-Identifier: MIT */
// Actual kernel SDK/driver instantiation ONLY. Never linked/installed/run.
#define PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT 1
#define PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
extern "C" {
 extern int32 pioneer_dram_context_api_version;
 status_t pioneer_dram_context_init_hardware();
 status_t pioneer_dram_context_init_driver();
 void pioneer_dram_context_uninit_driver();
 const char** pioneer_dram_context_publish_devices();
 device_hooks* pioneer_dram_context_find_device(const char*);
}
#define api_version pioneer_dram_context_api_version
#define init_hardware pioneer_dram_context_init_hardware
#define init_driver pioneer_dram_context_init_driver
#define uninit_driver pioneer_dram_context_uninit_driver
#define publish_devices pioneer_dram_context_publish_devices
#define find_device pioneer_dram_context_find_device
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"

// Genuine kernel module/semaphore/generation admission, NOT fixture-injected
// ready fields. This private compile witness prevents whole-TU optimization
// from replacing real scope tests with false just because conventional init
// is inert. It creates NO public driver hook and is NEVER invoked by tests.
extern "C" status_t pioneer_dram_context_private_kernel_initialize(){
 if(sPCI || sReloadGuard || sLock>=0 || sDramSweepNativeContext)return B_BUSY;
 status_t status=get_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE,(module_info**)&sReloadGuard);
 if(status!=B_OK)return status;
 if(sReloadGuard->version!=1)status=B_BAD_VALUE;
 else status=sReloadGuard->attach(&sReloadGeneration);
 if(status!=B_OK){put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE);sReloadGuard=NULL;return status;}
 status=get_module(B_PCI_MODULE_NAME,(module_info**)&sPCI);
 if(status==B_OK && !FindCard(sPCI,sCard))status=B_ENTRY_NOT_FOUND;
 if(status==B_OK){sLock=create_sem(1,"P100A DRAM private compile witness");if(sLock<0)status=sLock;}
 if(status==B_OK)return B_OK;
 if(sPCI){put_module(B_PCI_MODULE_NAME);sPCI=NULL;}
 const status_t detached=sReloadGuard->detach(sReloadGeneration,true);
 if(detached!=B_OK)sReloadGuard->fault(sReloadGeneration);
 put_module(PIONEER_DRIVER_RELOAD_GUARD_MODULE);sReloadGuard=NULL;return status;
}
extern "C" status_t pioneer_dram_context_private_open(const char* name,uint32 flags,void** cookie){
 return ProbeOpen(name,flags,cookie);
}
extern "C" status_t pioneer_dram_context_private_run(PioneerProbeCookie* fd,unsigned bank,
 unsigned shift,unsigned sweeps,uint32 nonce,PioneerDramSweepNativeReply& result){
 return RunPioneerDramReadSweepNative(fd,bank,shift,sweeps,nonce,result);
}
extern "C" status_t pioneer_dram_context_private_control(void* fd,uint32 op,void* data,size_t size){
 return ProbeControl(fd,op,data,size);
}
extern "C" status_t pioneer_dram_context_expected_refusal(){return B_NOT_ALLOWED;}
extern "C" void* pioneer_dram_context_expected_null(){return NULL;}
extern "C" void pioneer_dram_context_expected_inert(){}
