/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieRootAerSnapshot.h"

struct FakeRoot {
	uint32_t words[1024]={};
	unsigned reads=0;
	uint32_t RootRead(uint16_t offset,uint8_t width) {
		assert(width==4 && !(offset&3) && offset<4096);reads++;return words[offset/4];
	}
};
int main() {
	FakeRoot io;uint32_t status,source;
	io.words[0]=0x20421e30;io.words[0x100/4]=0x00010001;
	io.words[0x104/4]=0x20;io.words[0x134/4]=0x01000000;
	assert(PioneerPcieRootAerSnapshot(io,status,source) && status==0x20 && source==0x01000000);
	io.words[0x100/4]=0x18010003;io.words[0x180/4]=0x00010001;
	io.words[0x184/4]=0x1000;io.words[0x1b4/4]=0x22220000;
	assert(PioneerPcieRootAerSnapshot(io,status,source) && status==0x1000 && source==0x22220000);
	io.words[0x180/4]=0x10010003;
	assert(!PioneerPcieRootAerSnapshot(io,status,source) && status==UINT32_MAX);
	io.words[0x100/4]=0xffe10003;
	assert(!PioneerPcieRootAerSnapshot(io,status,source));
	io.words[0x100/4]=0x00010001;io.words[0x104/4]=UINT32_MAX;
	assert(!PioneerPcieRootAerSnapshot(io,status,source));
	io.words[0]=0;
	assert(!PioneerPcieRootAerSnapshot(io,status,source));
	std::puts("PASS root-port AER snapshot validates bounded capability traversal; READ-ONLY SIMULATION");
}
