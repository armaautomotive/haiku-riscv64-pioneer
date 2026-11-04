/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_BLOCK_OBSERVER_H
#define PIONEER_Q38_Q4_FPU_BLOCK_OBSERVER_H
#define PIONEER_Q38_Q4_FPU_DECODE_MODEL_ONLY
#include "pioneer_q38_q4_fpu_decode_test.cpp"
// OFFBOARD source-binding observer, NOT an UNPACK/Dst/MOVD2A emulator.
// Two original2304B byte-major tiles,32 output rows x256 K. Every source
// byte is preserved. A vector chunk is four K rows x8 even/odd output rows.
// No scalar learned products, runtime/device access or precision waiver.
static void CheckQ4FpuBlock(const uint8_t original[32][144],uint64_t& count)
{
	uint8_t tiled[2][2304];float reference[32][256];
	for(unsigned row=0;row<32;row++) {
		assert(PioneerDecodeQ4K(original[row],reference[row]));
		for(unsigned byte=0;byte<144;byte++)tiled[row/16][byte*16+row%16]=original[row][byte];
	}
	Q4FpuWords prep,decode[2];PioneerQ38Q4FpuPrepare(prep);
	PioneerQ38Q4FpuDecode<Q4FpuWords,false>(decode[0]);PioneerQ38Q4FpuDecode<Q4FpuWords,true>(decode[1]);
	bool visited[32][256]={};
	for(unsigned group=0;group<8;group++)for(unsigned half=0;half<2;half++)for(unsigned parity=0;parity<4;parity+=2) {
		Q4FpuMachine m;
		for(unsigned lane=0;lane<32;lane++) {
			unsigned row=(lane&7)*2+parity/2;
			auto byte=[&](unsigned b){return unsigned(tiled[half][b*16+row]);};
			m.lanes[1][lane]=PioneerFmaBits(PioneerHalfFloat(uint16_t(byte(0)|(byte(1)<<8))));
			m.lanes[2][lane]=PioneerFmaBits(PioneerHalfFloat(uint16_t(byte(2)|(byte(3)<<8))));
			if(group<4){m.lanes[3][lane]=byte(4+group)&63;m.lanes[4][lane]=byte(8+group)&63;}
			else {
				m.lanes[3][lane]=(byte(8+group)&15)|((byte(group)>>6)<<4);
				m.lanes[4][lane]=(byte(8+group)>>4)|((byte(4+group)>>6)<<4);
			}
		}
		m.Execute(prep);assert(m.okay);
		for(unsigned khalf=0;khalf<2;khalf++)for(unsigned block=0;block<16;block+=4) {
			for(unsigned lane=0;lane<32;lane++) {
				unsigned row=(lane&7)*2+parity/2,column=khalf*16+block+lane/8;
				const unsigned offset=256+(group/2)*512+column*16+row;
				assert(offset<2304);m.lanes[0][lane]=tiled[half][offset];
			}
			m.Execute(decode[group&1]);assert(m.okay);
			for(unsigned lane=0;lane<32;lane++) {
				unsigned row=half*16+(lane&7)*2+parity/2,column=group*32+khalf*16+block+lane/8;
				assert(!visited[row][column]);visited[row][column]=true;
				assert(m.lanes[0][lane]==Round(PioneerFmaBits(reference[row][column])));count++;
			}
		}
	}
	for(const auto& row:visited)for(bool value:row)assert(value);
}
#endif
