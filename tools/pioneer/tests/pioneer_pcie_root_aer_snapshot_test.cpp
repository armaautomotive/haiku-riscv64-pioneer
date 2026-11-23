/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieRootAerSnapshot.h"

struct FakeRoot {
	uint32_t words[1024]={};
	unsigned reads=0;
	uint32_t RootRead(uint16_t offset,uint8_t width) {
		assert((width==1 || width==2 || width==4) && offset+width<=4096);
		assert((offset&3)+width<=4);reads++;
		const uint32_t word=words[offset/4]>>((offset&3)*8);
		return width==1?word&0xff:width==2?word&0xffff:word;
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
	io.words[0]=0x20421e30;io.words[0x34/4]=0x40;
	io.words[0x40/4]=0x00420010; // PCIe v2 root port, no next capability
	io.words[0x48/4]=0x00060000;io.words[0x50/4]=0x12340000;
	io.words[0x100/4]=0x00010001;io.words[0x104/4]=1;
	io.words[0x108/4]=2;io.words[0x10c/4]=3;io.words[0x130/4]=4;io.words[0x134/4]=5;
	io.words[0x11c/4]=6;io.words[0x120/4]=7;io.words[0x124/4]=8;io.words[0x128/4]=9;
	tt_pioneer_pcie_trace_sample sample{};
	assert(PioneerPcieRootTraceSample(io,sample) && sample.valid==1
		&& sample.device_status==6 && sample.link_status==0x1234 && sample.uncorrectable==1
		&& sample.uncorrectable_mask==2 && sample.severity==3 && sample.root_status==4
		&& sample.source==5 && sample.header[0]==6 && sample.header[3]==9);
	io.words[0x40/4]=0x00020010; // endpoint, not a root port
	assert(!PioneerPcieRootTraceSample(io,sample) && !sample.valid);
	io.words[0x40/4]=0x00424010; // conventional-capability loop
	assert(!PioneerPcieRootTraceSample(io,sample) && !sample.valid);
	std::puts("PASS root-port AER snapshot validates bounded capability traversal; READ-ONLY SIMULATION");
}
