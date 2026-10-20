/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_AFFINE_TEST_MODEL_ONLY
#include "pioneer_q38_vector_affine_test.cpp"
#include "../PioneerQ38ZeroMinimumAffine.h"
int main()
{
	AffineWords full,shorter; PioneerQ38VectorAffine(full); PioneerQ38ZeroMinimumAffine(shorter);
	assert(full.words.size()==14 && shorter.words.size()==7);
	uint32_t seed=0x172903bf; unsigned cases=0;
	auto random=[&]() { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; };
	for (unsigned trial=0;trial<32768;trial++) {
		AffineVector old,next; uint32_t expected[32];
		for (unsigned lane=0;lane<32;lane++) {
			uint32_t coef;
			assert(PioneerQ38BinaryScale(uint16_t((random()%0x7c00)|(random()&0x8000)),int(random()%256)-128,coef));
			unsigned exp=95+random()%49;
			uint32_t scale=(exp<<23)|(exp==143?0:random()&0x7fffff)|(random()&0x80000000);
			int dot=int(random()%524289)-262144,sum=int(random()%4097)-2048;
			uint32_t acc=((47+random()%147)<<23)|(random()&0x7fffff)|(random()&0x80000000);
			if (!(trial%17)) coef=random()&0x80000000;
			if (!(trial%19)) scale=random()&0x80000000;
			if (!(trial%13)) dot=0;
			if (!(trial%3)) acc=0; // induction base is +0,NOT arbitrary signed zero
			float product=(PioneerFmaFloat(coef)*PioneerFmaFloat(scale))*float(dot);
			if (!(trial%11) && product!=0) acc=PioneerFmaBits(-product); // exact cancellation
			old.lr[0][lane]=SM(dot); old.lr[1][lane]=coef; old.lr[2][lane]=0;
			old.lr[3][lane]=scale; old.lr[4][lane]=SM(sum); old.lr[5][lane]=acc;
			float correction=(0.0f*PioneerFmaFloat(scale))*float(sum);
			expected[lane]=PioneerFmaBits(PioneerFmaFloat(acc)+(product+correction));
			assert(std::isfinite(PioneerFmaFloat(expected[lane])) && expected[lane]!=0x80000000);
		}
		next=old;
		// Prove discarded minimum/sum registers really are NEVER consumed.
		for (unsigned lane=0;lane<32;lane++) next.lr[2][lane]=next.lr[4][lane]=0x7fcabcde;
		next.ready[2]=next.ready[4]=100000;
		for (auto op:full.words) old.Op(op);
		for (auto op:shorter.words) next.Op(op);
		assert(old.okay && next.okay);
		for (unsigned lane=0;lane<32;lane++) {
			assert(old.lr[0][lane]==expected[lane] && next.lr[0][lane]==expected[lane]); cases++;
		}
	}
	// The +0 induction is NECESSARY,not a blanket signed-zero relaxation.
	AffineVector old,next;
	for (unsigned lane=0;lane<32;lane++) {
		old.lr[0][lane]=SM(1); old.lr[1][lane]=0x80000000;
		old.lr[3][lane]=0x3f800000; old.lr[4][lane]=SM(1); old.lr[5][lane]=0x80000000;
	}
	next=old; for (auto op:full.words) old.Op(op); for (auto op:shorter.words) next.Op(op);
	assert(old.okay && next.okay && old.lr[0][0]==0 && next.lr[0][0]==0x80000000);
	printf("PASS OFFLINE zero-minimum affine:%u exact original/RISC/independent ISA comparisons,normal-or-POSITIVE-zero induction,all32 lanes,signed coefficient/scale zeros,cancellation,unused registers poisoned;14->7 instructions;negative-zero accumulator counterexample retained;NO image/driver/native/cycle-speed qualification\n",cases);
}
