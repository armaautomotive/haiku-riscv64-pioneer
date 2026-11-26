/* Distributed under the MIT License. */
#ifndef PIONEER_PCI_PROFILE_H
#define PIONEER_PCI_PROFILE_H

#include <stdint.h>

enum { TT_PIONEER_RANGE_COUNT = 5 };

struct tt_pioneer_range {
	uint64_t host;
	uint64_t pci;
	uint64_t size;
	uint32_t type;
	uint32_t flags;
};

struct tt_pioneer_profile {
	uint32_t version;
	uint32_t stage;
	int32_t status;
	uint32_t ready;
	uint32_t card_bus;
	uint32_t root_bus;
	uint32_t root_command;
	uint32_t root_buses;
	uint32_t root_prefetch;
	uint32_t root_prefetch_base_high;
	uint32_t root_prefetch_limit_high;
	uint32_t root_memory;
	uint64_t controller;
	uint64_t config;
	uint64_t config_length;
	tt_pioneer_range ranges[TT_PIONEER_RANGE_COUNT];
	uint64_t bar0_host_start;
	uint64_t bar0_host_end;
	uint64_t bar2_host_start;
	uint64_t bar2_host_end;
};

#ifdef _KERNEL_MODE
#include <PCI.h>
#include <device_manager.h>
status_t GetPioneerPciProfile(device_manager_info*, const pci_info&,
	tt_pioneer_profile&);
// The callback runs synchronously while the validated device-tree nodes,
// including the PCI controller node, remain referenced. It may use the
// controller cookie only until it returns. The private device-manager pin
// service must be available to retain the initialized card, root port and
// their ancestors; bare node references are insufficient. Pins do not prevent removal or
// establish DMA, MMIO, topology stability or firmware exclusion.
typedef status_t (*pioneer_pci_controller_operation)(void* controllerCookie,
	void* argument);
status_t WithPioneerPciController(device_manager_info*, const pci_info&,
	tt_pioneer_profile&, pioneer_pci_controller_operation, void* argument);
// The nodes and cookie are borrowed for this callback only. A consumer that
// needs them across calls must acquire its own initialized-driver pins and
// module references while the callback holds the existing pins.
typedef status_t (*pioneer_pci_controller_nodes_operation)(void* controllerCookie,
	device_node* card, device_node* rootPort, void* argument);
status_t WithPioneerPciControllerNodes(device_manager_info*, const pci_info&,
	tt_pioneer_profile&, pioneer_pci_controller_nodes_operation, void* argument);
#endif

#endif
