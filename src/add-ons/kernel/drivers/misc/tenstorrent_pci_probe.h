/* Distributed under the MIT License. */
#ifndef TENSTORRENT_PCI_PROBE_H
#define TENSTORRENT_PCI_PROBE_H

#include <Drivers.h>
#include "BlackholeBar4Probe.h"
#include "PioneerPciProfile.h"
#include "BlackholePioneerMmioProbe.h"
#include "BlackholeArcScratchProbe.h"
#include "BlackholeArcTelemetryProbe.h"
#include "BlackholeArcQueueProbe.h"
#include "BlackholeTensixResetProbe.h"
#include "BlackholeL1ScratchProbe.h"
#include "BlackholeBriscSmokeProbe.h"
#include "BlackholeDramRouteProbe.h"
#include "BlackholeDramSamplesProbe.h"
#include "BlackholeTensixMatmulProbe.h"

#define TT_PCI_PROBE_DEVICE "misc/tenstorrent_pci_probe"
#define TT_PCI_PROBE_BAR4_ADDRESS (B_DEVICE_OP_CODES_END + 0x5451)
#define TT_PCI_PROBE_PIONEER_PROFILE (B_DEVICE_OP_CODES_END + 0x5452)
#define TT_PCI_PROBE_PIONEER_NOC_ID (B_DEVICE_OP_CODES_END + 0x5453)
#define TT_PCI_PROBE_PIONEER_ARC_SCRATCH (B_DEVICE_OP_CODES_END + 0x5454)
#define TT_PCI_PROBE_PIONEER_TELEMETRY (B_DEVICE_OP_CODES_END + 0x5455)
#define TT_PCI_PROBE_PIONEER_TENSIX_RESET (B_DEVICE_OP_CODES_END + 0x5456)
#define TT_PCI_PROBE_PIONEER_L1_SCRATCH (B_DEVICE_OP_CODES_END + 0x5457)
#define TT_PCI_PROBE_PIONEER_BRISC_SMOKE (B_DEVICE_OP_CODES_END + 0x5458)
#define TT_PCI_PROBE_PIONEER_BRISC_SUPPORT (B_DEVICE_OP_CODES_END + 0x5459)
#define TT_PCI_PROBE_PIONEER_DRAM_ROUTE (B_DEVICE_OP_CODES_END + 0x545a)
#define TT_PCI_PROBE_PIONEER_DRAM_SUPPORT (B_DEVICE_OP_CODES_END + 0x545b)
#define TT_PCI_PROBE_PIONEER_DRAM_ALL_ROUTES (B_DEVICE_OP_CODES_END + 0x545c)
#define TT_PCI_PROBE_PIONEER_DRAM_SAMPLES (B_DEVICE_OP_CODES_END + 0x545d)
#define TT_PCI_PROBE_PIONEER_DRAM_SAMPLES_SUPPORT (B_DEVICE_OP_CODES_END + 0x545e)
#define TT_PCI_PROBE_PIONEER_TENSIX_MATMUL (B_DEVICE_OP_CODES_END + 0x545f)
#define TT_PCI_PROBE_PIONEER_MATMUL_SUPPORT (B_DEVICE_OP_CODES_END + 0x5460)
#define TT_PCI_PROBE_PIONEER_TRISC_SMOKE (B_DEVICE_OP_CODES_END + 0x5461)
#define TT_PCI_PROBE_PIONEER_TRISC_SUPPORT (B_DEVICE_OP_CODES_END + 0x5462)
#define TT_PCI_PROBE_PIONEER_TRISC_RESET_SMOKE (B_DEVICE_OP_CODES_END + 0x5463)
#define TT_PCI_PROBE_PIONEER_TRISC_RESET_SUPPORT (B_DEVICE_OP_CODES_END + 0x5464)
#define TT_PCI_PROBE_PIONEER_TENSIX_CONFIG (B_DEVICE_OP_CODES_END + 0x5465)
#define TT_PCI_PROBE_PIONEER_TENSIX_CONFIG_SUPPORT (B_DEVICE_OP_CODES_END + 0x5466)
#define TT_PCI_PROBE_PIONEER_OPERATING_TELEMETRY (B_DEVICE_OP_CODES_END + 0x5467)
#define TT_PCI_PROBE_PIONEER_ARC_CLOCK_QUERY (B_DEVICE_OP_CODES_END + 0x5468)
#define TT_PCI_PROBE_PIONEER_ARC_QUERY_SUPPORT (B_DEVICE_OP_CODES_END + 0x5469)

struct tt_arc_query_support { uint32_t version, message, queue, entries, csm_bytes; };

struct tt_matmul_support {
	uint32_t version, code_words, footprint_words, entry, code_base, park_pc, results_base, reset_mask, running_mask;
};

struct tt_dram_samples_support {
	uint32_t version, banks, regions, words, enabled_columns, enabled_gddr;
};

struct tt_dram_route_support {
	uint32_t version, bank, source_words, id_words, translated_x, translated_y;
};

struct tt_brisc_support {
	uint32_t version, code_words, footprint_words, code_base, result_base, park_pc;
};

struct tt_pioneer_arc_result {
	tt_pioneer_mmio_result pci;
	tt_arc_scratch_result arc;
};

struct tt_pioneer_telemetry_result {
	tt_pioneer_arc_result control;
	tt_arc_telemetry_result telemetry;
};

struct tt_pioneer_arc_query_result {
	tt_pioneer_telemetry_result control;
	tt_arc_queue_result query;
};

struct tt_pioneer_tensix_result {
	tt_pioneer_telemetry_result control;
	tt_tensix_reset_result tensix;
};

struct tt_pioneer_l1_result {
	tt_pioneer_tensix_result control;
	tt_l1_scratch_result l1;
};

struct tt_pioneer_brisc_result {
	tt_pioneer_tensix_result control;
	tt_brisc_smoke_result brisc;
};

struct tt_pioneer_dram_route_result {
	tt_pioneer_tensix_result control;
	tt_dram_route_result dram;
};

struct tt_pioneer_dram_samples_result {
	tt_pioneer_tensix_result control;
	tt_dram_samples_result dram;
};

struct tt_pioneer_matmul_result {
	tt_pioneer_dram_samples_result baseline;
	tt_tensix_matmul_result math;
};

#endif
