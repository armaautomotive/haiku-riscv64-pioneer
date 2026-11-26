/* SPDX-License-Identifier: MIT */
// Qualification only: cached PCI identity and read-only PCI/FDT profile,
// then a fixed no-op controller lease. No config writes, BARs, DMA or reset.
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <device_manager.h>
#include <drivers/bus/PCI.h>
#include <string.h>
#include "PioneerPciProfile.h"
#include "../../../../system/kernel/device_manager/PioneerDeviceManagerPinModule.h"
#include "../../busses/pci/ecam/PioneerSg2042RecoveryModule.h"
#include "../../busses/pci/ecam/PioneerSg2042LeaseProbeModule.h"
#include "../../../../../tools/pioneer/PioneerEcamLeaseProbe.h"
#include "PioneerPciPinnedNodes.h"
#include "PioneerPcieRootResetReady.h"
#define PIONEER_ECAM_CONTENTION_QUALIFICATION
#include "PioneerEcamLeaseProbeLogic.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static status_t probe_open(const char*, uint32, void** cookie) { *cookie = NULL; return B_OK; }
static status_t probe_close(void*) { return B_OK; }
static status_t probe_free(void*) { return B_OK; }
static bool find_card(pci_module_info* pci, pci_info& card, uint32& cards)
{
	if (pci == NULL || pci->get_nth_pci_info == NULL) return false;
	pci_info info;
	for (long i = 0; pci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (info.vendor_id == 0x1e52 && info.device_id == 0xb140
			&& info.u.h0.subsystem_vendor_id == 0x1e52 && info.u.h0.subsystem_id == 0x0043
			&& (info.header_type & PCI_header_type_mask) == 0) {
			card = info; ++cards;
		}
	}
	return cards == 1;
}
// PIONEER_ECAM_CONTENTION_SEMAPHORES_BEGIN
static bool safe_semaphores(sem_id ready, sem_id release)
{
	if (ready < B_OK || release < B_OK || ready == release) return false;
	thread_info thread = {}; sem_info first = {}, second = {};
	return get_thread_info(find_thread(NULL), &thread) == B_OK
		&& get_sem_info(ready, &first) == B_OK && get_sem_info(release, &second) == B_OK
		&& first.team == thread.team && second.team == thread.team
		&& (first.count == 0 || first.count == -1) && second.count == 0;
}
// PIONEER_ECAM_CONTENTION_SEMAPHORES_END
struct RootResetReadyCall {
	device_manager_info* dm;
	pioneer_ecam_root_reset_ready* result;
	struct RootIO {
		pci_device_module_info* pci;
		pci_device* device;
		uint32 endpointReads=0;
		uint32 RootRead(uint16 offset, uint8 width) { return pci->read_pci_config(device, offset, width); }
		uint32 CardRead(uint16, uint8) { ++endpointReads; return UINT32_MAX; }
	};
	static status_t Run(device_node*, device_node* port, void* opaque) {
		auto& call=*static_cast<RootResetReadyCall*>(opaque);
		pci_device_module_info* pci=NULL; pci_device* device=NULL;
		if (port==NULL || call.dm->get_driver(port,(driver_module_info**)&pci,(void**)&device)!=B_OK
			|| pci==NULL || device==NULL || pci->info.info.name==NULL
			|| strcmp(pci->info.info.name,"bus_managers/pci/driver_v1")!=0
			|| pci->read_pci_config==NULL) return B_UNSUPPORTED;
		RootIO io{pci,device};
		call.result->ready=PioneerPcieRootResetReady(io)?1:0;
		call.result->endpointReads=io.endpointReads;
		return B_OK;
	}
};
static status_t probe_control(void*, uint32 operation, void* data, size_t size)
{
	if (operation == PIONEER_ECAM_ROOT_RESET_READY) {
		if (size != sizeof(pioneer_ecam_root_reset_ready)) return B_BAD_VALUE;
		pioneer_ecam_root_reset_ready result={};result.version=1;result.status=B_NO_INIT;
		device_manager_info* dm=NULL;
		status_t status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&dm);
		if (status==B_OK) {
			if (dm!=NULL) {
				RootResetReadyCall call{dm,&result};
				result.status=PioneerWithPinnedPciNodes(dm,RootResetReadyCall::Run,&call);
			}
			put_module(B_DEVICE_MANAGER_MODULE_NAME);
		} else result.status=status;
		return user_memcpy(data,&result,sizeof(result));
	}
	if (operation == PIONEER_ECAM_LEASE_STATE) {
		if (size != sizeof(pioneer_ecam_lease_state)) return B_BAD_VALUE;
		pioneer_ecam_lease_state state = {}; state.version = 1; state.stateStatus = B_NO_INIT; state.waiter = -1;
		device_manager_info* dm = NULL;
		status_t status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&dm);
		if (status == B_OK) {
			if (dm != NULL) {
				PioneerEcamLeaseStateCall call{dm, &state};
				state.stateStatus = PioneerWithPinnedPciNodes(dm, PioneerEcamLeaseStateCall::Run, &call);
			}
			put_module(B_DEVICE_MANAGER_MODULE_NAME);
		} else state.stateStatus = status;
		return user_memcpy(data, &state, sizeof(state));
	}
	if (operation == PIONEER_ECAM_LEASE_HOLD) {
		if (size != sizeof(pioneer_ecam_lease_hold)) return B_BAD_VALUE;
		pioneer_ecam_lease_hold hold = {};
		status_t status = user_memcpy(&hold, data, sizeof(hold));
		if (status != B_OK) return status;
		if (hold.version != 1 || !safe_semaphores(hold.ready, hold.release)) return B_BAD_VALUE;
		hold.profileStatus = hold.holdStatus = hold.callbackStatus = B_NO_INIT;
		hold.profileStage = hold.profileReady = 0;
		device_manager_info* dm = NULL; pci_module_info* pci = NULL;
		status = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&dm);
		if (status == B_OK) {
			status = get_module(B_PCI_MODULE_NAME, (module_info**)&pci);
			if (status == B_OK) {
				pci_info card = {}; uint32 cards = 0;
				if (dm != NULL && find_card(pci, card, cards)) {
					tt_pioneer_profile profile = {};
					PioneerEcamLeaseHoldCall call{&hold};
					hold.profileStatus = WithPioneerPciController(dm, card, profile, PioneerEcamLeaseHoldCall::Run, &call);
					hold.profileStage = profile.stage; hold.profileReady = profile.ready;
				}
				put_module(B_PCI_MODULE_NAME);
			}
			put_module(B_DEVICE_MANAGER_MODULE_NAME);
		}
		return user_memcpy(data, &hold, sizeof(hold));
	}
	if (operation != PIONEER_ECAM_LEASE_PROBE || size != sizeof(pioneer_ecam_lease_probe_result))
		return B_BAD_VALUE;
	pioneer_ecam_lease_probe_result result = {};
	result.version = 1;
	result.pciStatus = result.profileStatus = result.leaseModuleStatus = result.leaseStatus = B_NO_INIT;
	result.leaseCallbackStatus = result.recoveryModuleStatus = result.recoveryStatus = B_NO_INIT;
	device_manager_info* dm = NULL;
	result.managerStatus = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&dm);
	if (result.managerStatus == B_OK) {
		pci_module_info* pci = NULL;
		result.pciStatus = get_module(B_PCI_MODULE_NAME, (module_info**)&pci);
		if (result.pciStatus == B_OK) {
			pci_info card = {};
			if (dm != NULL && pci != NULL) {
				if (find_card(pci, card, result.cards)) {
					tt_pioneer_profile profile = {};
					PioneerEcamLeaseProbeCall call{&result};
					result.profileStatus = WithPioneerPciController(dm, card, profile,
						PioneerEcamLeaseProbeCall::Run, &call);
					result.profileStage = profile.stage;
					result.profileReady = profile.ready;
				}
			}
			put_module(B_PCI_MODULE_NAME);
		}
		put_module(B_DEVICE_MANAGER_MODULE_NAME);
	}
	return user_memcpy(data, &result, sizeof(result));
}
static status_t probe_read(void*, off_t, void*, size_t*) { return B_NOT_ALLOWED; }
static status_t probe_write(void*, off_t, const void*, size_t*) { return B_NOT_ALLOWED; }
status_t init_hardware() { return B_OK; }
status_t init_driver() { return B_OK; }
void uninit_driver() {}
const char** publish_devices() {
	static const char* names[] = { "misc/pioneer_ecam_lease_probe", NULL }; return names;
}
device_hooks* find_device(const char* name) {
	static device_hooks hooks = { probe_open, probe_close, probe_free, probe_control,
		probe_read, probe_write, NULL, NULL, NULL, NULL };
	return strcmp(name, "misc/pioneer_ecam_lease_probe") == 0 ? &hooks : NULL;
}
