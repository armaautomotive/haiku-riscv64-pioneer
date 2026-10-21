/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <array>
#include <cstring>
#define PIONEER_Q38_BLOCK_AFFINE_DST
#include "../PioneerQ38CachedAffine.h"
#include "../PioneerQ38BlockAffineDst.h"
int main() {
	std::array<uint8_t,2304> tile; uint32_t seed=0x7138492;
	const uint16_t halves[]={0,0x8000,1,0x8001,0x3ff,0x400,0x3c00,0xbc00,0x7bff,0xfbff};
	for (unsigned trial=0;trial<1024;trial++) {
		for (auto& b:tile) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; b=uint8_t(seed); }
		for (unsigned row=0;row<16;row++) for (unsigned kind=0;kind<2;kind++) {
			uint16_t h=halves[(trial+row*3+kind*7)%10];
			tile[kind*32+row]=uint8_t(h); tile[kind*32+16+row]=uint8_t(h>>8);
		}
		PioneerQ38CachedAffine cache,old; PioneerQ38Panel panel={},before={};
		std::array<uint32_t,258> scratch; scratch.fill(0xdeadbeef);
		assert(cache.Prepare(12,tile.data(),2304) && old.Prepare(12,tile.data(),2304));
		assert(cache.BlockQ4(&panel,scratch.data()+1,256) && !std::memcmp(&panel,&before,sizeof(panel)));
		assert(scratch.front()==0xdeadbeef && scratch.back()==0xdeadbeef);
		for (unsigned p=0;p<16;p++) {
			PioneerQ38Panel direct={}; assert(PioneerQ38UnpackPanel(12,tile.data(),2304,p,&direct) && old.Panel(p,&panel));
			for (unsigned row=0;row<16;row++) {
				assert(scratch[1+(p/2)*32+row]==direct.coefficient[row] && direct.coefficient[row]==panel.coefficient[row]);
				assert(scratch[1+(p/2)*32+16+row]==direct.minimum[row] && direct.minimum[row]==panel.minimum[row]);
			}
		}
		assert(!cache.BlockQ4(&panel,scratch.data()+1,256) && !cache.Panel(0,&panel));
	}
	for (unsigned fault=0;fault<9;fault++) {
		PioneerQ38CachedAffine cache; PioneerQ38Panel panel={}; std::array<uint32_t,256> scratch; scratch.fill(0xdeadbeef);
		assert(cache.Prepare(12,tile.data(),2304));
		volatile uint32_t* target=scratch.data(); unsigned count=256; auto* out=&panel;
		if (!fault) target=nullptr;
		if (fault==1) count=255;
		if (fault==2) count=257;
		if (fault==3) out=nullptr;
		if (fault==4) target=reinterpret_cast<volatile uint32_t*>(tile.data());
		if (fault==5) target=reinterpret_cast<volatile uint32_t*>(&cache);
		if (fault==6) target=reinterpret_cast<volatile uint32_t*>(&panel);
		if (fault==7) target=reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uintptr_t>(scratch.data())+1);
		if (fault==8) assert(cache.Panel(0,&panel));
		assert(!cache.BlockQ4(out,target,count));
		for (auto v:scratch) assert(v==0xdeadbeef);
		assert(!cache.Panel(1,&panel));
	}
	puts("PASS OFFLINE block-Q4 staging:1024 fixtures ALL8 groups/16 lanes exact to original per-panel and independent scalar unpack;zero/sign/subnormal/max-finite HALF,untouched panel/canaries,one-shot/size/null/alignment/alias/partial-owner refusals;NO image/native/speed qualification");
}
