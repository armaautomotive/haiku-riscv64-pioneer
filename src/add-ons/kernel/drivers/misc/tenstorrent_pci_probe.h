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
#include "BlackholeArcClockGuard.h"
#include "BlackholeTensixResetProbe.h"
#include "BlackholeL1ScratchProbe.h"
#include "BlackholeBriscSmokeProbe.h"
#include "BlackholeDramRouteProbe.h"
#include "BlackholeDramSamplesProbe.h"
#include "BlackholeTensixMatmulProbe.h"
#include "BlackholeClockedMatmulProbe.h"
#include "BlackholeComputeNocProbe.h"
#include "BlackholeResidentReadProbe.h"
#include "BlackholeResidentMathProbe.h"
#include "BlackholeHostPathProbe.h"
#include "PioneerHostPage.h"
#include "BlackholeHostDmaAudit.h"
#include "BlackholeHostReadProbe.h"
#include "BlackholeHostWriteProbe.h"
#include "PioneerResidentSession.h"
#include "PioneerResidentStore.h"
#include "PioneerRuntimeMatvec.h"
#include "PioneerResidentStream.h"

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
#define TT_PCI_PROBE_PIONEER_ARC_CLOCK_CYCLE (B_DEVICE_OP_CODES_END + 0x546a)
#define TT_PCI_PROBE_PIONEER_ARC_CLOCK_CYCLE_SUPPORT (B_DEVICE_OP_CODES_END + 0x546b)
#define TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL (B_DEVICE_OP_CODES_END + 0x546c)
#define TT_PCI_PROBE_PIONEER_CLOCKED_MATMUL_SUPPORT (B_DEVICE_OP_CODES_END + 0x546d)
#define TT_PCI_PROBE_PIONEER_SFPU_LOAD (B_DEVICE_OP_CODES_END + 0x546e)
#define TT_PCI_PROBE_PIONEER_SFPU_LOAD_SUPPORT (B_DEVICE_OP_CODES_END + 0x546f)
#define TT_PCI_PROBE_PIONEER_SFPU_STORE (B_DEVICE_OP_CODES_END + 0x5470)
#define TT_PCI_PROBE_PIONEER_SFPU_STORE_SUPPORT (B_DEVICE_OP_CODES_END + 0x5471)
#define TT_PCI_PROBE_PIONEER_OPERAND_SMOKE (B_DEVICE_OP_CODES_END + 0x5472)
#define TT_PCI_PROBE_PIONEER_OPERAND_SUPPORT (B_DEVICE_OP_CODES_END + 0x5473)
#define TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SMOKE (B_DEVICE_OP_CODES_END + 0x5474)
#define TT_PCI_PROBE_PIONEER_MATRIX_EXEC_SUPPORT (B_DEVICE_OP_CODES_END + 0x5475)
#define TT_PCI_PROBE_PIONEER_ZEROACC_SMOKE (B_DEVICE_OP_CODES_END + 0x5476)
#define TT_PCI_PROBE_PIONEER_ZEROACC_SUPPORT (B_DEVICE_OP_CODES_END + 0x5477)
#define TT_PCI_PROBE_PIONEER_MOVD_SMOKE (B_DEVICE_OP_CODES_END + 0x5478)
#define TT_PCI_PROBE_PIONEER_MOVD_SUPPORT (B_DEVICE_OP_CODES_END + 0x5479)
#define TT_PCI_PROBE_PIONEER_KERNEL_OPERAND (B_DEVICE_OP_CODES_END + 0x547a)
#define TT_PCI_PROBE_PIONEER_KERNEL_OPERAND_SUPPORT (B_DEVICE_OP_CODES_END + 0x547b)
#define TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC (B_DEVICE_OP_CODES_END + 0x547c)
#define TT_PCI_PROBE_PIONEER_KERNEL_MATRIX_EXEC_SUPPORT (B_DEVICE_OP_CODES_END + 0x547d)
#define TT_PCI_PROBE_PIONEER_KERNEL_MATMUL (B_DEVICE_OP_CODES_END + 0x547e)
#define TT_PCI_PROBE_PIONEER_KERNEL_MATMUL_SUPPORT (B_DEVICE_OP_CODES_END + 0x547f)
#define TT_PCI_PROBE_PIONEER_DST_READ (B_DEVICE_OP_CODES_END + 0x5480)
#define TT_PCI_PROBE_PIONEER_DST_READ_SUPPORT (B_DEVICE_OP_CODES_END + 0x5481)
#define TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP (B_DEVICE_OP_CODES_END + 0x5482)
#define TT_PCI_PROBE_PIONEER_SRCA_ROUNDTRIP_SUPPORT (B_DEVICE_OP_CODES_END + 0x5483)
#define TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP (B_DEVICE_OP_CODES_END + 0x5484)
#define TT_PCI_PROBE_PIONEER_SRCB_ROUNDTRIP_SUPPORT (B_DEVICE_OP_CODES_END + 0x5485)
#define TT_PCI_PROBE_PIONEER_PURE_SRCA (B_DEVICE_OP_CODES_END + 0x5486)
#define TT_PCI_PROBE_PIONEER_PURE_SRCA_SUPPORT (B_DEVICE_OP_CODES_END + 0x5487)
#define TT_PCI_PROBE_PIONEER_PURE_SRCB (B_DEVICE_OP_CODES_END + 0x5488)
#define TT_PCI_PROBE_PIONEER_PURE_SRCB_SUPPORT (B_DEVICE_OP_CODES_END + 0x5489)
#define TT_PCI_PROBE_PIONEER_PURE_MATMUL (B_DEVICE_OP_CODES_END + 0x548a)
#define TT_PCI_PROBE_PIONEER_PURE_MATMUL_SUPPORT (B_DEVICE_OP_CODES_END + 0x548b)
#define TT_PCI_PROBE_PIONEER_UNPACK_MATMUL (B_DEVICE_OP_CODES_END + 0x548c)
#define TT_PCI_PROBE_PIONEER_UNPACK_MATMUL_SUPPORT (B_DEVICE_OP_CODES_END + 0x548d)
#define TT_PCI_PROBE_PIONEER_UNPACK_SRCA (B_DEVICE_OP_CODES_END + 0x548e)
#define TT_PCI_PROBE_PIONEER_UNPACK_SRCA_SUPPORT (B_DEVICE_OP_CODES_END + 0x548f)
#define TT_PCI_PROBE_PIONEER_UNPACK_SRCB (B_DEVICE_OP_CODES_END + 0x5490)
#define TT_PCI_PROBE_PIONEER_UNPACK_SRCB_SUPPORT (B_DEVICE_OP_CODES_END + 0x5491)
#define TT_PCI_PROBE_PIONEER_UNPACK_CONFIG (B_DEVICE_OP_CODES_END + 0x5492)
#define TT_PCI_PROBE_PIONEER_UNPACK_CONFIG_SUPPORT (B_DEVICE_OP_CODES_END + 0x5493)
#define TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL (B_DEVICE_OP_CODES_END + 0x5494)
#define TT_PCI_PROBE_PIONEER_IDENTITY_MATMUL_SUPPORT (B_DEVICE_OP_CODES_END + 0x5495)
#define TT_PCI_PROBE_PIONEER_DENSE_MATMUL (B_DEVICE_OP_CODES_END + 0x5496)
#define TT_PCI_PROBE_PIONEER_DENSE_MATMUL_SUPPORT (B_DEVICE_OP_CODES_END + 0x5497)
#define TT_PCI_PROBE_PIONEER_DENSE_SRCA (B_DEVICE_OP_CODES_END + 0x5498)
#define TT_PCI_PROBE_PIONEER_DENSE_SRCA_SUPPORT (B_DEVICE_OP_CODES_END + 0x5499)
#define TT_PCI_PROBE_PIONEER_PATTERN_SRCB (B_DEVICE_OP_CODES_END + 0x549a)
#define TT_PCI_PROBE_PIONEER_PATTERN_SRCB_SUPPORT (B_DEVICE_OP_CODES_END + 0x549b)
#define TT_PCI_PROBE_PIONEER_DATA_TILE (B_DEVICE_OP_CODES_END + 0x549c)
#define TT_PCI_PROBE_PIONEER_DATA_TILE_SUPPORT (B_DEVICE_OP_CODES_END + 0x549d)
#define TT_PCI_PROBE_PIONEER_DATA_BATCH (B_DEVICE_OP_CODES_END + 0x549e)
#define TT_PCI_PROBE_PIONEER_DATA_BATCH_SUPPORT (B_DEVICE_OP_CODES_END + 0x549f)
#define TT_PCI_PROBE_PIONEER_COMPUTE_NOC (B_DEVICE_OP_CODES_END + 0x54a0)
#define TT_PCI_PROBE_PIONEER_COMPUTE_NOC_SUPPORT (B_DEVICE_OP_CODES_END + 0x54a1)
#define TT_PCI_PROBE_PIONEER_RESIDENT_READ (B_DEVICE_OP_CODES_END + 0x54a2)
#define TT_PCI_PROBE_PIONEER_RESIDENT_READ_SUPPORT (B_DEVICE_OP_CODES_END + 0x54a3)
#define TT_PCI_PROBE_PIONEER_RESIDENT_MATH (B_DEVICE_OP_CODES_END + 0x54a4)
#define TT_PCI_PROBE_PIONEER_RESIDENT_MATH_SUPPORT (B_DEVICE_OP_CODES_END + 0x54a5)
#define TT_PCI_PROBE_PIONEER_RESIDENT_GEMV (B_DEVICE_OP_CODES_END + 0x54a6)
#define TT_PCI_PROBE_PIONEER_RESIDENT_GEMV_SUPPORT (B_DEVICE_OP_CODES_END + 0x54a7)
#define TT_PCI_PROBE_PIONEER_HOST_PATH (B_DEVICE_OP_CODES_END + 0x54a8)
#define TT_PCI_PROBE_PIONEER_HOST_PATH_SUPPORT (B_DEVICE_OP_CODES_END + 0x54a9)
#define TT_PCI_PROBE_PIONEER_HOST_PAGE (B_DEVICE_OP_CODES_END + 0x54aa)
#define TT_PCI_PROBE_PIONEER_HOST_PAGE_SUPPORT (B_DEVICE_OP_CODES_END + 0x54ab)
#define TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT (B_DEVICE_OP_CODES_END + 0x54ac)
#define TT_PCI_PROBE_PIONEER_HOST_DMA_AUDIT_SUPPORT (B_DEVICE_OP_CODES_END + 0x54ad)
#define TT_PCI_PROBE_PIONEER_HOST_READ (B_DEVICE_OP_CODES_END + 0x54ae)
#define TT_PCI_PROBE_PIONEER_HOST_READ_SUPPORT (B_DEVICE_OP_CODES_END + 0x54af)
#define TT_PCI_PROBE_PIONEER_HOST_READ_PAGE (B_DEVICE_OP_CODES_END + 0x54b0)
#define TT_PCI_PROBE_PIONEER_HOST_READ_PAGE_SUPPORT (B_DEVICE_OP_CODES_END + 0x54b1)
#define TT_PCI_PROBE_PIONEER_HOST_WRITE (B_DEVICE_OP_CODES_END + 0x54b2)
#define TT_PCI_PROBE_PIONEER_HOST_WRITE_SUPPORT (B_DEVICE_OP_CODES_END + 0x54b3)
#define TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE (B_DEVICE_OP_CODES_END + 0x54b4)
#define TT_PCI_PROBE_PIONEER_HOST_WRITE_PAGE_SUPPORT (B_DEVICE_OP_CODES_END + 0x54b5)
#define TT_PCI_PROBE_PIONEER_SESSION_BEGIN (B_DEVICE_OP_CODES_END + 0x54b6)
#define TT_PCI_PROBE_PIONEER_SESSION_END (B_DEVICE_OP_CODES_END + 0x54b7)
#define TT_PCI_PROBE_PIONEER_SESSION_STATUS (B_DEVICE_OP_CODES_END + 0x54b8)
#define TT_PCI_PROBE_PIONEER_SESSION_SUPPORT (B_DEVICE_OP_CODES_END + 0x54b9)
#define TT_PCI_PROBE_PIONEER_STORE_UPLOAD (B_DEVICE_OP_CODES_END + 0x54ba)
#define TT_PCI_PROBE_PIONEER_STORE_READ (B_DEVICE_OP_CODES_END + 0x54bb)
#define TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC (B_DEVICE_OP_CODES_END + 0x54bc)
#define TT_PCI_PROBE_PIONEER_STREAM_UPLOAD (B_DEVICE_OP_CODES_END + 0x54bd)
#define TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_DMA (B_DEVICE_OP_CODES_END + 0x54be)
#define TT_PCI_PROBE_PIONEER_RUNTIME_MATVEC_FP16 (B_DEVICE_OP_CODES_END + 0x54bf)

struct tt_arc_query_support { uint32_t version, message, queue, entries, csm_bytes; };
struct tt_arc_clock_support { uint32_t version, query, busy, idle, before_mhz, max_mhz; };

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

struct tt_pioneer_arc_clock_result {
	tt_pioneer_telemetry_result control;
	tt_arc_clock_guard_result clock;
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

struct tt_pioneer_clocked_matmul_result {
	tt_pioneer_matmul_result matmul;
	tt_arc_clock_guard_result clock;
};

struct tt_pioneer_data_tile {
	tt_tensix_tile_data input;
	uint32_t output[256]; // FP32 bits, valid only after fully restored execution
	tt_pioneer_matmul_result result;
};

struct tt_pioneer_data_batch {
	uint32_t version, count, completed, reserved;
	tt_tensix_tile_data input[TT_DATA_TILE_BATCH_MAX];
	uint32_t output[TT_DATA_TILE_BATCH_MAX][256];
	tt_pioneer_matmul_result result;
};

struct tt_pioneer_compute_noc_result {
	tt_pioneer_dram_samples_result baseline;
	tt_compute_noc_result noc;
};

struct tt_pioneer_resident_read_result {
	tt_pioneer_compute_noc_result baseline;
	tt_resident_read_result read;
};

struct tt_pioneer_resident_math_result {
	tt_pioneer_compute_noc_result baseline;
	tt_resident_math_result compute;
};

struct tt_pioneer_resident_gemv_result {
	uint32_t version,inputs; // version 1; inputs exactly 1024, 5120 or 6144
	tt_pioneer_resident_math_result result;
};

struct tt_pioneer_host_path_result {
	tt_pioneer_compute_noc_result baseline;
	tt_host_path_result path;
};

struct tt_pioneer_host_dma_audit_result {
	tt_pioneer_host_path_result baseline;
	tt_host_dma_audit_result audit;
};

struct tt_pioneer_host_read_result {
	tt_pioneer_host_dma_audit_result baseline;
	tt_host_read_result read;
};

struct tt_pioneer_host_write_result {
	tt_pioneer_host_dma_audit_result baseline;
	tt_host_write_result write;
};

struct tt_pioneer_resident_page {
	uint32_t version,reserved;
	uint64_t offset;
	uint32_t data[1024];
	tt_resident_store_result result;
};

struct tt_pioneer_runtime_matvec {
	uint32_t version,inputs,outputs,reserved;
	uint64_t weight_offset;
	uint32_t input[TT_RUNTIME_MATVEC_INPUTS_MAX/2]; // packed BF16, never kernel floating-point
	uint32_t output[TT_RUNTIME_MATVEC_OUTPUTS_MAX]; // FP32 bits, no reference substitution
	tt_runtime_matvec_result result;
};

struct tt_pioneer_resident_stream {
	uint32_t version,reserved;
	uint64_t offset;
	uint32_t data[1024];
	tt_resident_stream_result result;
};

#endif
