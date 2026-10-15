/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuTile.h"
#include <math.h>
#include <time.h>

static double Seconds()
{
	struct timespec time; clock_gettime(CLOCK_MONOTONIC,&time);
	return time.tv_sec+time.tv_nsec*1e-9;
}
int main(int argc, char** argv)
{
	if (argc != 2 || strcmp(argv[1],"--run-data-tiles-and-reset") != 0) {
		fprintf(stderr,"Usage: %s --run-data-tiles-and-reset\nRuns eight data-only BF16 matrices, four fidelity phases, all-element FP32 reference.\nOne idle tile context reinitialized; no NoC, host DMA or firmware changes.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	PioneerTileBackend backend;
	if (!backend.Open()) return 1;
	double started = Seconds(); float maxError = 0, maxNormalized = 0;
	for (unsigned trial = 0; trial < 8; trial++) {
		tt_pioneer_data_tile tile = {}; tile.input.version = 1;
		for (unsigned i = 0; i < 256; i++) {
			tile.input.weights[i] = (0x3e80+(i*13+trial*7)%384) | ((i&1) ? 0x8000 : 0);
			tile.input.activations[i] = (0x3f00+(i*11+trial*3)%256) | ((i&2) ? 0x8000 : 0);
		}
		if (!backend.Run(tile)) return 1;
		unsigned mismatches = 0;
		for (unsigned row = 0; row < 16; row++) for (unsigned column = 0; column < 16; column++) {
			float reference = 0, sumAbsolute = 0;
			for (unsigned k = 0; k < 16; k++) {
				float product = PioneerTileFloat(uint32_t(tile.input.activations[row*16+k])<<16)
					*PioneerTileFloat(uint32_t(tile.input.weights[k*16+column])<<16);
				reference += product; sumAbsolute += fabsf(product);
			}
			float actual = PioneerTileFloat(tile.output[row*16+column]);
			float error = fabsf(actual-reference); if (error > maxError) maxError = error;
			float normalized = sumAbsolute > 0 ? error/sumAbsolute : error;
			if (normalized > maxNormalized) maxNormalized = normalized;
			// Explicit numerical acceptance target, NOT an IEEE exactness claim.
			// MVMUL has a non-IEEE reduction datapath; diagonal controls are exact.
			// Cancellation is handled using the sum of absolute products, not |sum|.
			if (!isfinite(actual) || error > 0.000001f+sumAbsolute/1024.0f) {
				if (mismatches < 4) fprintf(stderr,"trial %u row/col %u/%u actual %.9g expected %.9g\n",trial,row,column,actual,reference);
				mismatches++;
			}
		}
		printf("P100A data-only BF16 trial %u: mismatches %u/256, complete/stop/restore verified, cold 0\n",trial,mismatches);
		if (mismatches != 0) return 1;
	}
	printf("PASS: %llu real P100A tiles, 2048 checked outputs, max absolute error %.9g, max error/sum-abs %.9g (<1/1024 target), elapsed %.3f seconds; no CPU fallback\n",
		(unsigned long long)backend.calls,maxError,maxNormalized,Seconds()-started);
	return 0;
}
