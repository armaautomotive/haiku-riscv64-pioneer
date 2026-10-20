/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38DeltaStateLayout.h"
#include <cassert>
#include <cstdio>
#include <vector>
int main()
{
	std::vector<uint32_t> source(PIONEER_Q38_GDN_CHECKPOINT_WORDS),copy(source.size(),0xdeadbeef),half(PIONEER_Q38_GDN_HALF_WORDS,0xdeadbeef);
	for (unsigned i=0;i<source.size();i++) source[i]=0x3f000000+i;
	for (unsigned h=0;h<48;h++) for (unsigned side=0;side<2;side++) {
		assert(PioneerQ38GdnKeyHead(h)==h%16 && PioneerQ38GdnImportHalf(source.data(),unsigned(source.size()),h,side,half.data(),unsigned(half.size())));
		for (unsigned key=0;key<128;key++) for (unsigned value=0;value<64;value++)
			assert(half[key*64+value]==source[h*16384+(side*64+value)*128+key]);
		assert(PioneerQ38GdnExportHalf(half.data(),unsigned(half.size()),h,side,copy.data(),unsigned(copy.size())));
	}
	assert(copy==source && PioneerQ38GdnKeyHead(48)==~0U && PioneerQ38GdnKeyHead(17)==1); // NOT head/3
	for (unsigned index:{0U,8191U}) for (uint32_t bad:{1U,0x80000001U,0x7f800000U,0x7fc00001U}) {
		std::vector<uint32_t> before(half.size(),0xdeadbeef); half=before;
		unsigned key=index/64,value=index%64,at=47*16384+(64+value)*128+key,saved=source[at]; source[at]=bad;
		assert(!PioneerQ38GdnImportHalf(source.data(),unsigned(source.size()),47,1,half.data(),unsigned(half.size())) && half==before); source[at]=saved;
		half.assign(half.size(),0); half[index]=bad; auto checkpoint=copy;
		assert(!PioneerQ38GdnExportHalf(half.data(),unsigned(half.size()),47,1,copy.data(),unsigned(copy.size())) && copy==checkpoint);
	}
	assert(!PioneerQ38GdnImportHalf(source.data(),unsigned(source.size()),48,0,half.data(),unsigned(half.size())));
	assert(!PioneerQ38GdnImportHalf(source.data(),unsigned(source.size()),0,2,half.data(),unsigned(half.size())));
	assert(!PioneerQ38GdnImportHalf(source.data(),unsigned(source.size())-1,0,0,half.data(),unsigned(half.size())));
	assert(!PioneerQ38GdnImportHalf(source.data(),unsigned(source.size()),0,0,half.data(),unsigned(half.size())-1));
	assert(!PioneerQ38GdnImportHalf(source.data(),unsigned(source.size()),0,0,source.data(),unsigned(half.size())));
	assert(!PioneerQ38GdnImportHalf(nullptr,unsigned(source.size()),0,0,half.data(),unsigned(half.size())));
	assert(!PioneerQ38GdnExportHalf(half.data(),unsigned(half.size()),48,0,copy.data(),unsigned(copy.size())));
	assert(!PioneerQ38GdnExportHalf(half.data(),unsigned(half.size()),0,2,copy.data(),unsigned(copy.size())));
	assert(!PioneerQ38GdnExportHalf(half.data(),unsigned(half.size())-1,0,0,copy.data(),unsigned(copy.size())));
	assert(!PioneerQ38GdnExportHalf(half.data(),unsigned(half.size()),0,0,copy.data(),unsigned(copy.size())-1));
	assert(!PioneerQ38GdnExportHalf(source.data(),unsigned(half.size()),0,0,source.data(),unsigned(source.size())));
	puts("PASS OFFLINE GDN checkpoint adapter:ALL786432 state words/48 heads x2 halves,GGML value-key transpose and tiled h%16 grouping,exact round trip,bad shape/alias/nonfinite/subnormal inputs refuse before output;NOT per-token CPU processing/native registry/transport");
}
