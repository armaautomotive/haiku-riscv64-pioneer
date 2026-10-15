/* SPDX-License-Identifier: MIT */
// Offline numerical adapter only. Open() refuses, so the native CLI cannot
// accidentally report a hardware pass when compiled against this test shim.
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeTensixTileData.h"
#include <stdio.h>
#include <string.h>
struct tt_pioneer_data_tile { tt_tensix_tile_data input; uint32_t output[256]; };
static float PioneerTileFloat(uint32_t u) { float f; memcpy(&f,&u,4); return f; }
static uint16_t PioneerTileBf16(float f) {
	uint32_t u; memcpy(&u,&f,4);
	if ((u&0x7f800000)==0) return (u>>16)&0x8000;
	return (u+0x7fff+((u>>16)&1))>>16;
}
struct PioneerTileBackend {
	uint64_t calls=0;
	bool Open() { return false; }
	bool Run(tt_pioneer_data_tile& tile) {
		if(!BlackholeTileDataValid(tile.input)) return false;
		calls++;
		for(unsigned row=0;row<16;row++) for(unsigned column=0;column<16;column++) {
			float value=0;
			for(unsigned k=0;k<16;k++) value+=PioneerTileFloat(uint32_t(tile.input.activations[row*16+k])<<16)*PioneerTileFloat(uint32_t(tile.input.weights[k*16+column])<<16);
			memcpy(&tile.output[row*16+column],&value,4);
		}
		return true;
	}
	bool RunBatch(const tt_tensix_tile_data* input,uint32_t output[][256],unsigned count) {
		if(count==0 || count>TT_DATA_TILE_BATCH_MAX) return false;
		for(unsigned i=0;i<count;i++) {
			tt_pioneer_data_tile tile={}; tile.input=input[i]; if(!Run(tile)) return false;
			memcpy(output[i],tile.output,sizeof(tile.output));
		}
		return true;
	}
};
