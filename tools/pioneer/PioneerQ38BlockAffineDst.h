/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BLOCK_AFFINE_DST_H
#define PIONEER_Q38_BLOCK_AFFINE_DST_H
// OFFLINE ONLY. Fixed L1 scratch97000..97400 is disjoint from ALL original
// input/sum/scales/tile/panel/stack/code extents. Dst32 rows128..191 map to
// physical256..383 (cfg220=0),disjoint from raw0..31,dot32..47,decoded64..79
// and original coef/min/acc96..111. FULL padding initialization is required
// BEFORE these partial writes. Same FP32 UNPACK/values/drains as old affine,
// but configure the metadata format ONCE for ALL8 Q4 groups in the block.
template<class E> static __attribute__((noinline)) bool PioneerQ38BlockAffineDst(E& e) {
	e.Op(0x3700000f); e.Op(0x5e203c00); e.Op(0x5120000a); e.Op(0x5420000f);
	e.Op(0xb2050000); e.Drain();
	e.Config(1,0xa0200000); e.Config(64,0x100010); e.Config(65,1); e.Config(72,0);
	for (unsigned group=0;group<8;group++) for (unsigned kind=0;kind<2;kind++) {
		unsigned row=128+group*8+kind*4,address=0x97000+group*128+kind*64;
		e.Config(84,0x00400000+(row+4)*16); e.Config(76,(address-16)/16);
		e.Op(0x42000081); e.Drain(); // EXACT original partial FP32 UNPACK,no flip
	}
	e.Config(1,0xa03c8000); e.Config(64,0x10001e); e.Config(65,16); e.Config(72,14); e.Config(84,0x00400040);
	// Return ONLY with all config/RWC work drained:the caller next writes
	// the original SrcB L1 operands,not another queued GPU consumer.
	e.Op(0xb2050004); e.Op(0x5e23fc00); e.Op(0x3700000f); e.Drain();
	return true;
}
#endif
