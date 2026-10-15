/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuTile.h"
#include <math.h>
int main(int argc, char** argv)
{
	if (argc != 2 || strcmp(argv[1],"--run-diagonal-controls-and-reset") != 0) return 2;
	setvbuf(stdout,NULL,_IONBF,0);
	PioneerTileBackend backend; if (!backend.Open()) return 1;
	for (unsigned trial = 0; trial < 9; trial++) {
		tt_pioneer_data_tile tile = {}; tile.input.version = 1;
		for (unsigned i = 0; i < 256; i++) {
			uint16_t a = 0x3f80 + (i*13)%128, b = 0x3f80+(i*11)%128;
			tile.input.weights[i] = i/16 == i%16 ? (trial == 0 ? 0x3f80 : a) : 0;
			tile.input.activations[i] = trial == 1 ? (i/16 == i%16 ? 0x3f80 : 0) : b;
		}
		if (trial == 3 || trial == 4) {
			memset(tile.input.weights,0,sizeof(tile.input.weights));
			memset(tile.input.activations,0,sizeof(tile.input.activations));
			tile.input.weights[0] = tile.input.activations[0] = trial == 3 ? 0x3f81 : 0x3f87;
		}
		if (trial >= 5) for (unsigned i = 0; i < 256; i++) {
			tile.input.weights[i] = trial == 5 ? 0x3f81 : trial == 7 ? 0x3f80 : 0x3f80+(i*13)%128;
			tile.input.activations[i] = trial == 5 ? 0x3f81 : trial == 6 ? 0x3f80 : 0x3f80+(i*11)%128;
		}
		if (!backend.Run(tile)) return 1;
		unsigned mismatches = 0; float maximum = 0;
		for (unsigned row = 0; row < 16; row++) for (unsigned col = 0; col < 16; col++) {
			float reference = 0;
			for (unsigned k = 0; k < 16; k++) reference += PioneerTileFloat(uint32_t(tile.input.activations[row*16+k])<<16)*PioneerTileFloat(uint32_t(tile.input.weights[k*16+col])<<16);
			float actual = PioneerTileFloat(tile.output[row*16+col]), error = fabsf(actual-reference);
			if (error > maximum) maximum = error;
			if (error > 0.000001f) {
				if (mismatches < 8) printf("trial %u row/col %u/%u actual %.9g expected %.9g error %.9g bits %08x\n",trial,row,col,actual,reference,actual-reference,tile.output[row*16+col]);
				mismatches++;
			}
		}
		printf("DIAGNOSTIC trial %u mismatches %u max-error %.9g; cold 0, complete/restored\n",trial,mismatches,maximum);
	}
	return 0;
}
