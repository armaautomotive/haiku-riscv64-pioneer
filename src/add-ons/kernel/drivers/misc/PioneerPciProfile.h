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
#endif

#endif
