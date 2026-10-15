/* SPDX-License-Identifier: MIT */
#ifndef TENSTORRENT_HAIKU_TILE_H
#define TENSTORRENT_HAIKU_TILE_H
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static inline float PioneerTileFloat(uint32_t bits)
{
	float value; memcpy(&value,&bits,4); return value;
}
static inline uint16_t PioneerTileBf16(float value)
{
	uint32_t bits; memcpy(&bits,&value,4);
	// The device flushes denormals. Explicitly normalize this at the API edge.
	if ((bits & 0x7f800000) == 0) return (bits>>16)&0x8000;
	bits += 0x7fff + ((bits>>16)&1); // round to nearest, ties to even
	return bits>>16;
}
static inline bool PioneerTileSucceeded(const tt_pioneer_matmul_result& r)
{
	const tt_tensix_matmul_result& m = r.math;
	const tt_pioneer_mmio_result& p = r.baseline.control.control.control.pci;
	return m.version == 26 && m.state == TT_MATMUL_DATA_EXECUTED
		&& m.started && m.completed && m.stopped && m.context_reset
		&& m.fixtures == 2 && m.phase == TT_UNPACK_FINAL_PHASE
		&& m.marker == 0x4d415431 && m.pc[0] == TT_UNPACK_PARK_PC && m.pc[1] == TT_UNPACK_PARK_PC
		&& m.l1_restored && m.tlb_restored && !m.recovery_required
		&& r.baseline.dram.routes_verified == 7 && r.baseline.dram.cores_checked == 120
		&& p.version == 1 && p.state == TT_PIONEER_MMIO_READ_OK && p.restored
		&& p.commands_after[0] == 0 && p.commands_after[1] == 0;
}
class PioneerTileBackend {
	int fd;
	bool failed;
	bool batchChecked;
public:
	uint64_t calls;
	uint64_t transactions;
	PioneerTileBackend() : fd(-1), failed(false), batchChecked(false), calls(0), transactions(0) {}
	~PioneerTileBackend() { if (fd >= 0) close(fd); }
	bool Open() {
		if (fd >= 0 || failed) return false;
		fd = open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
		if (fd < 0) { perror("P100A tile open"); failed = true; return false; }
		tt_matmul_support s = {};
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_DATA_TILE_SUPPORT,&s,sizeof(s)) != 0
			|| s.version != 39 || s.code_words != TT_UNPACK_CODE_WORDS
			|| s.footprint_words != TT_UNPACK_IMAGE_WORDS || s.entry != 0x6000
			|| s.code_base != 0x70000 || s.park_pc != TT_UNPACK_PARK_PC
			|| s.results_base != 0x17f800 || s.reset_mask != 0x87ffff || s.running_mask != 0x46800) {
			fprintf(stderr,"P100A data-only tile ABI unavailable; no card operation submitted\n");
			failed = true; return false;
		}
		return true;
	}
	bool Run(tt_pioneer_data_tile& tile) {
		if (fd < 0 || failed || !BlackholeTileDataValid(tile.input)) return false;
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_DATA_TILE,&tile,sizeof(tile)) != 0) {
			perror("P100A data tile ioctl"); failed = true; return false;
		}
		calls++;
		transactions++;
		if (!PioneerTileSucceeded(tile.result)) {
			fprintf(stderr,"P100A STOP: execution state %u completed/stopped %u/%u restored %u/%u/%u cold %u; no retries\n",
				tile.result.math.state,tile.result.math.completed,tile.result.math.stopped,
				tile.result.math.l1_restored,tile.result.math.tlb_restored,
				tile.result.baseline.control.control.control.pci.restored,tile.result.math.recovery_required);
			failed = true; return false;
		}
		return true;
	}
	bool RunBatch(const tt_tensix_tile_data* input,uint32_t output[][256],unsigned count) {
		if (fd < 0 || failed || count == 0 || count > TT_DATA_TILE_BATCH_MAX) return false;
		for (unsigned i=0;i<count;i++) if (!BlackholeTileDataValid(input[i])) return false;
		if (!batchChecked) {
			tt_matmul_support s = {};
			if (ioctl(fd,TT_PCI_PROBE_PIONEER_DATA_BATCH_SUPPORT,&s,sizeof(s)) != 0
				|| s.version != 40 || s.code_words != TT_UNPACK_CODE_WORDS
				|| s.footprint_words != TT_UNPACK_IMAGE_WORDS || s.park_pc != TT_UNPACK_PARK_PC
				|| s.entry != 0x6000 || s.code_base != 0x70000 || s.results_base != 0x17f800
				|| s.reset_mask != 0x87ffff || s.running_mask != 0x46800) {
				fprintf(stderr,"P100A batch ABI unavailable; no batch submitted\n"); failed=true; return false;
			}
			batchChecked=true;
		}
		tt_pioneer_data_batch batch = {}; batch.version=1; batch.count=count;
		memcpy(batch.input,input,count*sizeof(*input));
		if (ioctl(fd,TT_PCI_PROBE_PIONEER_DATA_BATCH,&batch,sizeof(batch)) != 0) {
			perror("P100A data batch ioctl"); failed=true; return false;
		}
		transactions++;
		if (batch.completed != count || !PioneerTileSucceeded(batch.result)) {
			fprintf(stderr,"P100A batch STOP: completed %u/%u state %u cold %u; no retries\n",
				batch.completed,count,batch.result.math.state,batch.result.math.recovery_required);
			failed=true; return false;
		}
		calls+=count;
		memcpy(output,batch.output,count*sizeof(batch.output[0]));
		return true;
	}
	bool Multiply16(const float* weights, const float* activations, float* output) {
		tt_pioneer_data_tile tile = {}; tile.input.version = 1;
		for (unsigned i = 0; i < 256; i++) {
			tile.input.weights[i] = PioneerTileBf16(weights[i]);
			tile.input.activations[i] = PioneerTileBf16(activations[i]);
		}
		if (!Run(tile)) return false;
		for (unsigned i = 0; i < 256; i++) output[i] = PioneerTileFloat(tile.output[i]);
		return true;
	}
private:
	PioneerTileBackend(const PioneerTileBackend&);
	PioneerTileBackend& operator=(const PioneerTileBackend&);
};
#endif
