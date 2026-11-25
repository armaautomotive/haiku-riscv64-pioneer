/* SPDX-License-Identifier: MIT */
// Qualification only: no PCI module, device enumeration, BARs or card IO.
#include <Drivers.h>
#include <KernelExport.h>
#include <device_manager.h>
#include <string.h>
#include "../../../../system/kernel/device_manager/PioneerDeviceManagerPinModule.h"
#include "../../../../../tools/pioneer/PioneerDeviceManagerPinProbe.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;

static status_t callback(void* opaque)
{
	uint32* count = static_cast<uint32*>(opaque);
	++*count;
	return B_CANCELED;
}

static status_t probe_open(const char*, uint32, void** cookie)
{
	*cookie = NULL;
	return B_OK;
}
static status_t probe_close(void*) { return B_OK; }
static status_t probe_free(void*) { return B_OK; }
static status_t probe_control(void*, uint32 operation, void* data, size_t size)
{
	if (operation != PIONEER_DM_PIN_PROBE || size != sizeof(pioneer_dm_pin_probe_result))
		return B_BAD_VALUE;
	pioneer_dm_pin_probe_result result = {};
	result.version = 1;
	result.managerModuleStatus = result.invalidStatus = result.rootStatus = B_NO_INIT;
	result.callbackStatus = B_NO_INIT;
	pioneer_dm_driver_pin_module_info* pin = NULL;
	result.pinModuleStatus = get_module(PIONEER_DEVICE_MANAGER_PIN_MODULE, (module_info**)&pin);
	if (result.pinModuleStatus == B_OK) {
		device_manager_info* dm = NULL;
		if (pin != NULL && pin->run != NULL)
			result.managerModuleStatus = get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&dm);
		if (result.managerModuleStatus == B_OK && dm != NULL
			&& dm->get_root_node != NULL && dm->put_node != NULL) {
			status_t ignored = B_NO_INIT;
			result.invalidStatus = pin->run(NULL, callback, &result.callbacks, &ignored);
			device_node* root = dm->get_root_node();
			if (root != NULL) {
				result.rootStatus = pin->run(root, callback, &result.callbacks, &result.callbackStatus);
				dm->put_node(root);
			}
		}
		if (result.managerModuleStatus == B_OK)
			put_module(B_DEVICE_MANAGER_MODULE_NAME);
		put_module(PIONEER_DEVICE_MANAGER_PIN_MODULE);
	}
	return user_memcpy(data, &result, sizeof(result));
}
static status_t probe_read(void*, off_t, void*, size_t*) { return B_NOT_ALLOWED; }
static status_t probe_write(void*, off_t, const void*, size_t*) { return B_NOT_ALLOWED; }

status_t init_hardware() { return B_OK; }
status_t init_driver() { return B_OK; }
void uninit_driver() {}
const char** publish_devices()
{
	static const char* names[] = { "misc/pioneer_dm_pin_probe", NULL };
	return names;
}
device_hooks* find_device(const char* name)
{
	static device_hooks hooks = { probe_open, probe_close, probe_free, probe_control,
		probe_read, probe_write, NULL, NULL, NULL, NULL };
	return strcmp(name, "misc/pioneer_dm_pin_probe") == 0 ? &hooks : NULL;
}
