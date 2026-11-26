/* SPDX-License-Identifier: MIT */
// Temporary qualification of software node references only. No PCI config,
// driver-method calls, BAR mappings, DMA, firmware commands or model access.
#include <Drivers.h>
#include <KernelExport.h>
#include <device_manager.h>
#include <drivers/bus/PCI.h>
#include <string.h>
#include "../../../../system/kernel/device_manager/PioneerDeviceManagerPinModule.h"
#include "../../../../../tools/pioneer/PioneerPciPinProbe.h"
#include "PioneerPciPinnedNodes.h"
#include "PioneerPciPinProbeLogic.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;
static status_t probe_open(const char*, uint32, void** cookie) { *cookie = NULL; return B_OK; }
static status_t probe_close(void*) { return B_OK; }
static status_t probe_free(void*) { return B_OK; }
static status_t probe_control(void*, uint32 operation, void* data, size_t size)
{
	if (operation != PIONEER_PCI_PIN_PROBE || size != sizeof(pioneer_pci_pin_probe_result))
		return B_BAD_VALUE;
	pioneer_pci_pin_probe_result result = {};
	result.version = 1;
	result.pinStatus = result.cardDriverStatus = result.portDriverStatus = B_NO_INIT;
	device_manager_info* dm = NULL;
	result.managerStatus = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&dm);
	if (result.managerStatus == B_OK) {
		if (dm != NULL) {
			PioneerPciPinProbeCall call{dm, &result};
			result.pinStatus = PioneerWithPinnedPciNodes(dm, PioneerPciPinProbeCall::Check, &call);
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
	static const char* names[] = { "misc/pioneer_pci_pin_probe", NULL }; return names;
}
device_hooks* find_device(const char* name) {
	static device_hooks hooks = { probe_open, probe_close, probe_free, probe_control,
		probe_read, probe_write, NULL, NULL, NULL, NULL };
	return strcmp(name, "misc/pioneer_pci_pin_probe") == 0 ? &hooks : NULL;
}
