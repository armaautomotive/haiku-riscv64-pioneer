/* SPDX-License-Identifier: MIT */
// Qualification only: cached PCI identity and read-only PCI/FDT profile,
// then a fixed no-op controller lease. No config writes, BARs, DMA or reset.
#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <device_manager.h>
#include <string.h>
#include "PioneerPciProfile.h"
#include "../../busses/pci/ecam/PioneerSg2042RecoveryModule.h"
#include "../../busses/pci/ecam/PioneerSg2042LeaseProbeModule.h"
#include "../../../../../tools/pioneer/PioneerEcamLeaseProbe.h"
#include "PioneerEcamLeaseProbeLogic.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static status_t probe_open(const char*, uint32, void** cookie) { *cookie = NULL; return B_OK; }
static status_t probe_close(void*) { return B_OK; }
static status_t probe_free(void*) { return B_OK; }
static status_t probe_control(void*, uint32 operation, void* data, size_t size)
{
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
				pci_info info;
				for (long i = 0; pci->get_nth_pci_info(i, &info) == B_OK; i++) {
					if (info.vendor_id == 0x1e52 && info.device_id == 0xb140
						&& info.u.h0.subsystem_vendor_id == 0x1e52 && info.u.h0.subsystem_id == 0x0043
						&& (info.header_type & PCI_header_type_mask) == 0) {
						card = info; ++result.cards;
					}
				}
				if (result.cards == 1) {
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
