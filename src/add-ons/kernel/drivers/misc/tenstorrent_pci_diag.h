/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Read-only P100A PCI configuration snapshot. No BAR sizing or MMIO is done.
 */
#ifndef TENSTORRENT_PCI_DIAG_H
#define TENSTORRENT_PCI_DIAG_H

#include <Drivers.h>

#define TT_PCI_DIAG_DEVICE "misc/tenstorrent_pci_diag"
#define TT_PCI_DIAG_VERSION 1
#define TT_PCI_DIAG_GET_SNAPSHOT (B_DEVICE_OP_CODES_END + 0x5450)

typedef struct tt_pci_diag_snapshot {
	uint32 version;
	uint16 vendor_id;
	uint16 device_id;
	uint16 subsystem_vendor_id;
	uint16 subsystem_id;
	uint8 bus;
	uint8 device;
	uint8 function;
	uint8 revision;
	uint16 command;
	uint16 status;
	uint32 bar[6];
} tt_pci_diag_snapshot;

#endif
