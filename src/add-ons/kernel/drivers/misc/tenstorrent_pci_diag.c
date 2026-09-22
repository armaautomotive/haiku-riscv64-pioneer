/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * A deliberately read-only first step toward a native Blackhole driver.
 */

#include "tenstorrent_pci_diag.h"

#include <KernelExport.h>
#include <PCI.h>
#include <string.h>
#include <vm/vm.h>


#define TT_VENDOR_ID 0x1e52
#define BLACKHOLE_DEVICE_ID 0xb140


int32 api_version = B_CUR_DRIVER_API_VERSION;

static pci_module_info* sPCI;
static pci_info sDevice;
static bool sFound;
static const char* sDeviceNames[] = {TT_PCI_DIAG_DEVICE, NULL};
static const char* sNoDevices[] = {NULL};


static bool
find_card(pci_module_info* pci, pci_info* card)
{
	pci_info info;
	for (long index = 0; pci->get_nth_pci_info(index, &info) == B_OK;
			index++) {
		if (info.vendor_id == TT_VENDOR_ID
			&& info.device_id == BLACKHOLE_DEVICE_ID) {
			*card = info;
			return true;
		}
	}
	return false;
}


status_t
init_hardware(void)
{
	pci_module_info* pci;
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&pci);
	if (status != B_OK)
		return status;

	pci_info card;
	bool found = find_card(pci, &card);
	put_module(B_PCI_MODULE_NAME);
	return found ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
init_driver(void)
{
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&sPCI);
	if (status != B_OK)
		return status;

	sFound = find_card(sPCI, &sDevice);
	if (!sFound) {
		put_module(B_PCI_MODULE_NAME);
		sPCI = NULL;
		return B_ENTRY_NOT_FOUND;
	}
	return B_OK;
}


void
uninit_driver(void)
{
	if (sPCI != NULL)
		put_module(B_PCI_MODULE_NAME);
	sPCI = NULL;
	sFound = false;
}


const char**
publish_devices(void)
{
	return sFound ? sDeviceNames : sNoDevices;
}


static status_t
diag_open(const char* name, uint32 flags, void** cookie)
{
	if (!sFound || strcmp(name, TT_PCI_DIAG_DEVICE) != 0)
		return B_ENTRY_NOT_FOUND;
	*cookie = &sDevice;
	return B_OK;
}


static status_t
diag_close(void* cookie)
{
	return B_OK;
}


static status_t
diag_free(void* cookie)
{
	return B_OK;
}


static status_t
diag_control(void* cookie, uint32 op, void* arg, size_t length)
{
	if (op != TT_PCI_DIAG_GET_SNAPSHOT)
		return B_BAD_VALUE;
	if (arg == NULL || length < sizeof(tt_pci_diag_snapshot))
		return B_BAD_VALUE;

	const pci_info* card = (const pci_info*)cookie;
	tt_pci_diag_snapshot snapshot = {0};
	snapshot.version = TT_PCI_DIAG_VERSION;
	snapshot.vendor_id = card->vendor_id;
	snapshot.device_id = card->device_id;
	snapshot.subsystem_vendor_id = card->u.h0.subsystem_vendor_id;
	snapshot.subsystem_id = card->u.h0.subsystem_id;
	snapshot.bus = card->bus;
	snapshot.device = card->device;
	snapshot.function = card->function;
	snapshot.revision = card->revision;

	uint32 commandStatus = sPCI->read_pci_config(card->bus, card->device,
		card->function, 0x04, 4);
	snapshot.command = commandStatus & 0xffff;
	snapshot.status = commandStatus >> 16;
	for (int i = 0; i < 6; i++) {
		snapshot.bar[i] = sPCI->read_pci_config(card->bus, card->device,
			card->function, 0x10 + i * 4, 4);
	}

	return user_memcpy(arg, &snapshot, sizeof(snapshot));
}


static status_t
diag_read(void* cookie, off_t position, void* buffer, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}


static status_t
diag_write(void* cookie, off_t position, const void* buffer, size_t* length)
{
	*length = 0;
	return B_NOT_ALLOWED;
}


device_hooks*
find_device(const char* name)
{
	static device_hooks hooks = {
		diag_open,
		diag_close,
		diag_free,
		diag_control,
		diag_read,
		diag_write
	};
	return strcmp(name, TT_PCI_DIAG_DEVICE) == 0 ? &hooks : NULL;
}
