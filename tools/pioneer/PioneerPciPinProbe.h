/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PCI_PIN_PROBE_H
#define PIONEER_PCI_PIN_PROBE_H
#include <Drivers.h>
enum { PIONEER_PCI_PIN_PROBE = B_DEVICE_OP_CODES_END + 0x351 };
struct pioneer_pci_pin_probe_result {
	uint32 version;
	status_t managerStatus;
	status_t pinStatus;
	status_t cardDriverStatus;
	status_t portDriverStatus;
	uint32 callbacks;
	uint32 matchingDrivers;
};
#endif
