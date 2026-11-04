/* SPDX-License-Identifier: MIT */
#include "../PioneerNativeDramBandLoader.h"
#include "../PioneerNativeDramBandCompute.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using L=PioneerNativeDramBandLayout;
struct IO {
	unsigned width,route,events=0,issues=0,completions=0,headerWrites=0,headerReads=0,scalarPayloadReads=0;
	unsigned fault=0,maxQueue=0,dropped=99;
	std::array<unsigned,10> command;
	std::array<uint32_t,16> counters;
	std::vector<L::Read> pending;
	std::vector<unsigned char> l1,dram;
	explicit IO(unsigned k,unsigned xy=17|(12<<6)):width(k),route(xy),l1(0x180000,0xa7),dram(0x110000) {
		command.fill(0xcccccccc);counters.fill(UINT32_MAX-17);
		for(unsigned i=0;i<dram.size();i++)dram[i]=uint8_t((i*17+i/1088*13)^0x8b);
	}
	bool Header(unsigned a)const {
		for(unsigned operand=0;operand<2;operand++) {
			const unsigned start=(operand?L::Input:L::Weight)-L::Header,stride=operand?L::InputStride:L::WeightStride;
			if(a>=start && a-start<L::Tiles(width)*stride && (a-start)%stride<L::Header)return true;
		}
		return false;
	}
	void Flush() {
		assert(pending.size()==16);maxQueue=std::max(maxQueue,unsigned(pending.size()));
		for(const auto& p:pending) {
			assert(!(p.source&31) && !(p.destination&31) && !(p.bytes&63) && p.bytes<=16384);
			if(PioneerNativeDramLoadVariant::Role==83)assert(!(p.source&63) && !(p.destination&63));
			memcpy(l1.data()+p.destination,dram.data()+p.source-L::DramWeight,p.bytes);
			counters[2]++;counters[3]+=p.bytes/64;counters[4]++;counters[5]++;counters[14]++;completions++;
		}
		pending.clear();
		if(fault==5)counters[2]++;
		if(fault==6)counters[3]++;
		if(fault==7)counters[1]++;
		if(fault==8)counters[2]--;
	}
	unsigned Read(unsigned a) {
		events++;
		if(Header(a)){headerReads++;return l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;}
		if(a==0xffb20044)return (17<<12)|(12<<19)|129U|(fault==1?4U:0U);
		if(a==0xffb20048)return 0x10000;
		if(a==0xffb20148)return 129;
		if(a==0xffb20118)return 1U<<5;
		if(a==0xffb20130)return 2U<<10;
		if(a==0xffb20150 || a==0xffb20154)return 0;
		if(a==0xffb20100)return fault==2?0x4000:0x14000;
		if(a==0xffb20068)return fault==3?1:0;
		if(a==0xffb20064) {
			if(pending.size()==16 && fault!=4)Flush();
			return 0x10101010-unsigned(pending.size());
		}
		if(a==0xffb20040)return fault==9 && issues?2:0;
		if(a==0xffb20840 || a==0xffb21040 || a==0xffb21840)return fault==10?1:0;
		if(a>=0xffb20240 && a<0xffb202c0)return fault==11 && issues?1:0;
		if(a>=0xffb20200 && a<0xffb20240)return counters[(a-0xffb20200)/4];
		if(a>=0xffb20000 && a<0xffb20028)return command[(a-0xffb20000)/4];
		scalarPayloadReads++;assert(false);return 0;
	}
	void Write(unsigned a,unsigned v) {
		events++;
		if(Header(a)) {
			headerWrites++;assert(v==0);
			if(fault!=12 || headerWrites!=1)memset(l1.data()+a,0,4);
			return;
		}
		if(a==0xffb20040) {
			assert(v==1 && pending.size()<16);
			L::Read expected={};assert(L::Packet(width,issues/L::Tiles(width),issues%L::Tiles(width),expected));
			assert(command[0]==expected.source && command[3]==expected.destination && command[8]==expected.bytes);
			assert(!command[1] && command[2]==route && !command[4] && command[5]==129 && !command[6] && !command[7] && !command[9]);
			pending.push_back(expected);issues++;return;
		}
		assert(a>=0xffb20000 && a<0xffb20028);
		const unsigned index=(a-0xffb20000)/4;
		if(issues==0 && index==dropped)return;
		command[index]=v;
	}
	void Fence(){events++;}
	void Exact()const {
		assert(issues==2*L::Tiles(width) && completions==issues && maxQueue==16 && pending.empty());
		assert(headerWrites==2*(L::Header/4)*L::Tiles(width) && headerReads==headerWrites && !scalarPayloadReads);
		std::vector<uint8_t> expected(l1.size(),0xa7);
		for(unsigned operand=0;operand<2;operand++)for(unsigned tile=0;tile<L::Tiles(width);tile++) {
			L::Read p={};assert(L::Packet(width,operand,tile,p));
			memset(expected.data()+p.destination-L::Header,0,L::Header);
			memcpy(expected.data()+p.destination,dram.data()+p.source-L::DramWeight,p.bytes);
		}
		assert(l1==expected); // whole L1,including every unowned byte/guard
	}
};
struct Events {
	struct Event {unsigned kind,index,value;};std::vector<Event> all;
	void Op(unsigned word){all.push_back({0,0,word});}
	void Config(unsigned index,unsigned value){all.push_back({1,index,value});}
	void Drain(){all.push_back({2,0,0});}
};
#ifndef PIONEER_NATIVE_DRAM_LOADER_TEST_ONLY
int main() {
	unsigned passed=0,refused=0;
	for(unsigned k:{5120U,6144U,17408U}) {
		for(unsigned x:{17U,18U})for(unsigned y:{12U,15U,18U,21U}) {
			IO io(k,x|(y<<6));PioneerNativeDramBandLoader loader;assert(loader.Run(io,k,io.route));io.Exact();
			const unsigned before=io.events;assert(!loader.Run(io,k,io.route) && io.events==before);passed++;
		}
		Events canonical,translated;assert(PioneerQ38NativeBfpBand::Run<4>(canonical,k));assert(PioneerNativeDramBandCompute::Run(translated,k));
		assert(canonical.all.size()==translated.all.size());unsigned weights=0,inputs=0;bool active=false;
		for(unsigned i=0;i<canonical.all.size();i++) {
			auto old=canonical.all[i];const auto got=translated.all[i];
			assert(old.kind==got.kind && old.index==got.index);
			if(old.kind==1 && old.index==112 && old.value==0x00200015)active=true;
			if(old.kind==1 && old.index==76)old.value=L::Weight/16-1+(active?weights++*(L::WeightStride/16):0);
			if(old.kind==1 && old.index==124)old.value=L::Input/16-1+(active?inputs++*(L::InputStride/16):0);
			assert(old.value==got.value);
		}
		assert(weights==L::Tiles(k) && inputs==weights);passed++;
	}
	for(unsigned fault=1;fault<=12;fault++) {
		IO io(5120);io.fault=fault;PioneerNativeDramBandLoader loader;assert(!loader.Run(io,5120,io.route));
		const unsigned before=io.events;assert(!loader.Run(io,5120,io.route) && io.events==before && !io.scalarPayloadReads);refused++;
	}
	for(unsigned drop=0;drop<10;drop++) {
		IO io(5120);io.dropped=drop;PioneerNativeDramBandLoader loader;assert(!loader.Run(io,5120,io.route) && !io.issues);refused++;
	}
	for(unsigned bad:{0U,32U,5119U,5121U,17409U,~0U}) {
		IO io(5120);PioneerNativeDramBandLoader loader;assert(!loader.Run(io,bad,io.route) && !io.events);
		Events noIO;assert(!PioneerNativeDramBandCompute::Run(noIO,bad) && noIO.all.empty());refused++;
	}
	for(unsigned bad:{0U,129U,16U|(12U<<6),17U|(14U<<6),~0U}) {
		IO io(5120);PioneerNativeDramBandLoader loader;assert(!loader.Run(io,5120,bad) && !io.events);refused++;
	}
	L::Read preserved={1,2,3};
	assert(!L::Packet(5120,2,0,preserved) && preserved.source==1 && preserved.destination==2 && preserved.bytes==3);
	assert(!L::Packet(5120,0,160,preserved) && preserved.source==1);refused+=2;
	{
		Events out;PioneerNativeDramBandCompute::Descriptors<Events> adapter(out,5120);
		adapter.Config(112,0x00200015);const auto before=out.all.size();adapter.Config(76,0);
		adapter.Op(1);adapter.Drain();adapter.Config(1,2);assert(!adapter.Complete() && out.all.size()==before);refused++;
	}
	printf("PASS OFFBOARD card-side packed-GDDR loader:%u valid/%u refused;3 widths/8 route SHAPES,whole L1/headers/guards exact,16-request queue/counter wrap,C%u every tile,NO scalar payload reads/GDDR writes/PCIe,exact old4-phase operation sequence except descriptors;one-shot failures/no further IO;NO native ownership/hardware/full-model quality/speed proof\n",passed,refused,L::Header);
}
#endif
