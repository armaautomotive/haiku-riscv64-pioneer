/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PCI_PIN_PROBE_LOGIC_H
#define PIONEER_PCI_PIN_PROBE_LOGIC_H
// Call only while both node driver-initialization references are held.
// This inspects software metadata, not PCI configuration or card registers.
struct PioneerPciPinProbeCall {
	device_manager_info* dm;
	pioneer_pci_pin_probe_result* result;
	static status_t Check(device_node* card, device_node* port, void* argument) {
		PioneerPciPinProbeCall& call = *static_cast<PioneerPciPinProbeCall*>(argument);
		++call.result->callbacks;
		driver_module_info* cardDriver = NULL;
		driver_module_info* portDriver = NULL;
		void* cardCookie = NULL;
		void* portCookie = NULL;
		call.result->cardDriverStatus = call.dm->get_driver(card, &cardDriver, &cardCookie);
		call.result->portDriverStatus = call.dm->get_driver(port, &portDriver, &portCookie);
		const char* expected = "bus_managers/pci/driver_v1";
		if (call.result->cardDriverStatus == B_OK && call.result->portDriverStatus == B_OK
			&& cardDriver != NULL && portDriver != NULL && cardCookie != NULL && portCookie != NULL
			&& cardDriver->info.name != NULL && portDriver->info.name != NULL
			&& strcmp(cardDriver->info.name, expected) == 0
			&& strcmp(portDriver->info.name, expected) == 0) {
			call.result->matchingDrivers = 1;
			return B_OK;
		}
		return B_UNSUPPORTED;
	}
};
#endif
