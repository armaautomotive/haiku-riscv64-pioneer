/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4FpuCompactStreamLoader.h"
#include "../PioneerQ38Q4FpuCompactStreamCompute.h"
#include "../PioneerQ38Q4FpuCompactBlock.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using SL=PioneerQ38Q4FpuCompactStreamLayout;using Loader=PioneerQ38Q4FpuCompactStreamLoader;
struct StreamIO {
	struct Packet {SL::Read data;unsigned due,buffer,block;};
	unsigned width,route,events=0,reads=0,writes=0,ticks=0,issues=0,completed=0,headerWrites=0,headerReads=0,maxQueued=0;
	unsigned fault=0,dropCommand=99,reader=2,overlap=0,jobStart=0,jobs=0;
	std::array<unsigned,10> command;std::array<uint32_t,16> counters;
	std::vector<Packet> queued,responses;std::vector<uint8_t> ownedL1,dram;std::vector<uint8_t>& l1;
	unsigned copies[2]={},dataBlock[2]={~0U,~0U};bool ready[2]={};
	unsigned counterSeen=0;uint32_t creditSeen=0;bool emptySeen=false,busySeen=false;uint32_t observed[9]={};
	explicit StreamIO(unsigned k,unsigned xy=17|(12<<6),std::vector<uint8_t>* shared=nullptr)
		:width(k),route(xy),ownedL1(shared?0:0x180000,0xa7),dram(0x110000),l1(shared?*shared:ownedL1) {
		command.fill(0xcccccccc);counters.fill(UINT32_MAX-17);
		for(unsigned i=0;i<dram.size();i++)dram[i]=uint8_t((i*17+i/4608*13)^0x8b);
	}
	void Progress() {
		ticks++;
		if(!queued.empty() && fault!=4) {Packet p=queued.front();queued.erase(queued.begin());p.due=ticks+100;responses.push_back(p);}
		for(auto it=responses.begin();it!=responses.end();) {
			if(it->due>ticks){it++;continue;}
			const auto& p=*it;assert(reader!=p.buffer);memcpy(l1.data()+p.data.destination,dram.data()+p.data.source-SL::DramWeight,p.data.bytes);
			counters[2]++;counters[3]+=p.data.bytes/64;counters[4]++;counters[5]++;counters[14]++;completed++;copies[p.buffer]++;
			if(fault==5)counters[2]++;if(fault==6)counters[3]++;if(fault==7)counters[1]++;
			if(fault==8)counters[4]--;if(reader<2)overlap++;
			it=responses.erase(it);
		}
	}
	unsigned Read(unsigned a) {
		events++;reads++;Progress();
		if(SL::HeaderWord(a)){headerReads++;return l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;}
		if(a==0xffb20044)return (17<<12)|(12<<19)|129U|(fault==1?4U:0U);
		if(a==0xffb20048)return 0x10000;if(a==0xffb20148)return 129;
		if(a==0xffb20118)return 1U<<5;if(a==0xffb20130)return 2U<<10;
		if(a==0xffb20150 || a==0xffb20154)return 0;
		if(a==0xffb20100)return fault==2?0x4000:0x14000;if(a==0xffb20068)return fault==3?1:0;
		if(a==0xffb20064){if(queued.empty())emptySeen=true;return fault==16?0x11101010:0x10101010-unsigned(queued.size());}
		if(a==0xffb20040){busySeen=true;return fault==9 && issues?2:0;}
		if(a==0xffb20840 || a==0xffb21040 || a==0xffb21840)return fault==10?1:0;
		if(a>=0xffb20240 && a<0xffb202c0){unsigned value=a==0xffb20240?unsigned(responses.size()):(fault==11 && issues?1:0);if(!value)creditSeen|=1U<<((a-0xffb20240)/4);return value;}
		if(a>=0xffb20200 && a<0xffb20240){unsigned index=(a-0xffb20200)/4;const unsigned indices[]={1,2,3,4,5,8,10,12,14};for(unsigned i=0;i<9;i++)if(index==indices[i]){counterSeen|=1U<<i;observed[i]=counters[index];}return counters[index];}
		if(a>=0xffb20000 && a<0xffb20028)return command[(a-0xffb20000)/4]^(fault==13 && a==0xffb20008?1U:0U);
		assert(false && "NO scalar payload read");return 0;
	}
	void Write(unsigned a,unsigned v) {
		events++;writes++;Progress();
		if(SL::HeaderWord(a)){
			if(a==SL::HeaderAddress(0,0)){assert(queued.empty() && responses.empty() && reader==2);jobStart=issues;jobs++;copies[0]=copies[1]=0;ready[0]=ready[1]=false;}
			assert(!v && issues==jobStart);headerWrites++;if(fault!=12 || headerWrites!=1)memset(l1.data()+a,0,4);return;
		}
		if(a==0xffb20040) {
			assert(v==1 && queued.size()<16);unsigned which=(issues-jobStart)/SL::Packets,packet=(issues-jobStart)%SL::Packets,buffer=which&1;
			counterSeen=creditSeen=0;emptySeen=busySeen=false;
			SL::Read expected={};assert(SL::Packet(width,which,buffer,packet,expected));
			assert(command[0]==expected.source && command[3]==expected.destination && command[8]==expected.bytes && command[2]==route
				&& !command[1] && !command[4] && command[5]==129 && !command[6] && !command[7] && !command[9]);
			assert(!(expected.source&63) && !(expected.destination&63) && !(expected.bytes&63) && reader!=buffer);
			if(!packet){copies[buffer]=0;ready[buffer]=false;dataBlock[buffer]=which;}
			queued.push_back({expected,0,buffer,which});issues++;maxQueued=std::max(maxQueued,unsigned(queued.size()));return;
		}
		assert(a>=0xffb20000 && a<0xffb20028);unsigned i=(a-0xffb20000)/4;
		if(!issues && i==dropCommand)return;command[i]=v;
	}
	void Fence(){
		events++;Progress();
		if(issues && queued.empty() && responses.empty() && emptySeen && busySeen && counterSeen==511 && creditSeen==~0U) {
			const unsigned indices[]={1,2,3,4,5,8,10,12,14};bool stable=true;for(unsigned i=0;i<9;i++)stable&=observed[i]==counters[indices[i]];
			if(stable)for(unsigned b=0;b<2;b++)if(copies[b]==SL::Packets)ready[b]=true;
		}
	}
	void Math(unsigned buffer,unsigned block) {
		assert(buffer<2 && reader==buffer && copies[buffer]==SL::Packets && dataBlock[buffer]==block && ready[buffer]);
		for(unsigned i=0;i<40;i++)Progress();
	}
	void Exact()const {
		assert(issues==SL::Packets*(width/256) && completed==issues && queued.empty() && responses.empty() && maxQueued<=16 && overlap);
		assert(headerWrites==2*SL::Packets*16 && headerReads==headerWrites);
		std::vector<uint8_t> expected(l1.size(),0xa7);
		for(unsigned buffer=0;buffer<2;buffer++)for(unsigned packet=0;packet<SL::Packets;packet++) {
			SL::Read p={};assert(SL::Packet(width,width/256-2+buffer,buffer,packet,p));
			memset(expected.data()+p.destination-64,0,64);memcpy(expected.data()+p.destination,dram.data()+p.source-SL::DramWeight,p.bytes);
		}
		assert(l1==expected); // EVERY owned header/payload AND unowned byte
	}
};
struct StreamEvents {
	struct Event {unsigned kind,index,value;bool operator==(const Event& x)const{return kind==x.kind && index==x.index && value==x.value;}};
	std::vector<Event> all;void Op(unsigned v){all.push_back({0,0,v});}void Config(unsigned i,unsigned v){all.push_back({1,i,v});}void Drain(){all.push_back({2,0,0});}
};
[[maybe_unused]] static bool Drive(Loader& loader,StreamIO& io) {
	if(!loader.Begin(io,io.width,io.route) || !loader.Open(io,0,0))return false;
	for(unsigned s=0;s<2;s++)if(!loader.Slice(io))return false;
	if(!loader.Wait(io))return false;
	for(unsigned block=0;block<io.width/256;block++) {
		unsigned buffer=block&1;bool next=block+1<io.width/256;
		if(!loader.Acquire(block,buffer))return false;io.reader=buffer;
		if(next && !loader.Open(io,block+1,buffer^1))return false;
		for(unsigned group=0;group<8;group++){if(next && group<2 && !loader.Slice(io))return false;io.Math(buffer,block);}
		io.reader=2;if(!loader.Release(block,buffer) || (next && !loader.Wait(io)))return false;
	}
	return loader.Complete(io);
}
static void NoMore(Loader& l,StreamIO& io) {
	unsigned before=io.events;
	assert(!l.Begin(io,io.width,io.route) && !l.Open(io,0,0) && !l.Slice(io) && !l.Wait(io) && !l.Acquire(0,0) && !l.Release(0,0) && !l.Complete(io));
	assert(io.events==before);
}
#ifndef PIONEER_Q4_COMPACT_STREAM_LOADER_ONLY
int main() {
	unsigned passed=0,refused=0;
	for(unsigned width:{5120U,17408U})for(unsigned x:{17U,18U})for(unsigned y:{12U,15U,18U,21U}) {
		StreamIO io(width,x|(y<<6));Loader loader;assert(Drive(loader,io));io.Exact();NoMore(loader,io);passed++;
	}
	for(unsigned fault:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,16U}) {
		StreamIO io(5120);io.fault=fault;Loader loader;assert(!Drive(loader,io));NoMore(loader,io);refused++;
	}
	for(unsigned drop=0;drop<10;drop++){StreamIO io(5120);io.dropCommand=drop;Loader loader;assert(!Drive(loader,io) && !io.issues);NoMore(loader,io);refused++;}
	for(unsigned bad:{0U,256U,5119U,5121U,6144U,17409U,~0U}) {
		StreamIO io(5120);Loader loader;assert(!loader.Begin(io,bad,io.route) && !io.events);NoMore(loader,io);refused++;
	}
	for(unsigned bad:{0U,129U,16U|(12U<<6),17U|(14U<<6),~0U}) {
		StreamIO io(5120);Loader loader;assert(!loader.Begin(io,5120,bad) && !io.events);NoMore(loader,io);refused++;
	}
	for(unsigned misuse=0;misuse<8;misuse++) {
		StreamIO io(5120);Loader l;assert(l.Begin(io,5120,io.route));
		if(misuse==0)assert(!l.Open(io,1,0));if(misuse==1)assert(!l.Acquire(0,0));if(misuse==2)assert(!l.Release(0,0));
		if(misuse==3)assert(!l.Wait(io));if(misuse==4)assert(!l.Slice(io));if(misuse==5)assert(!l.Complete(io));
		if(misuse==6){assert(l.Open(io,0,0));assert(!l.Open(io,0,1));}
		if(misuse==7){assert(l.Open(io,0,0));assert(!l.Wait(io));}
		NoMore(l,io);refused++;
	}
	using Old=PioneerQ38Q4FpuCompactBlock;using New=PioneerQ38Q4FpuCompactStreamCompute;
	StreamEvents canonical,split;Old::Run(canonical);New::Begin(split);unsigned cold=unsigned(split.all.size());assert(New::LoadMetadata(split,0));
	for(unsigned g=0;g<8;g++)assert(New::Group(split,0,g));assert(canonical.all==split.all);
	for(unsigned buffer=0;buffer<2;buffer++) {
		StreamEvents block;assert(New::LoadMetadata(block,buffer));for(unsigned g=0;g<8;g++)assert(New::Group(block,buffer,g));
		assert(block.all.size()+cold==canonical.all.size());
		for(unsigned i=0;i<block.all.size();i++) {
			auto expected=canonical.all[cold+i];if(expected.kind==1 && (expected.index==76 || expected.index==124))expected.value+=buffer*0x1000;
			assert(expected==block.all[i] && !(block.all[i].kind==0 && block.all[i].value==0x10180000));
		}
	}
	StreamEvents noIO;assert(!New::LoadMetadata(noIO,2) && !New::Group(noIO,2,0) && !New::Group(noIO,0,8) && noIO.all.empty());
	printf("PASS OFFBOARD compact Q4 stream loader:%u valid/%u refused,full5120/17408 x8 route SHAPES,actual26 packets/80 flits perK256,two buffers/13-command slices/overlap/whole L1 exact/counter wrap,NO scalar payload/GDDR write/host IO;NO next IO after failure or completion;exact old compute events minus one-time init/descriptor translation;NOT native/hardware/quality/speed proof\n",passed,refused);
}
#endif

