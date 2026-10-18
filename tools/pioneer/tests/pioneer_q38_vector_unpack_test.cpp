/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_LAYOUT_MACHINE_ONLY
#include "pioneer_q38_vector_layout_test.cpp"
#include "../PioneerQ38VectorUnpack.h"
#include "../PioneerQwen38Gguf.h"
struct UnpackEvent { unsigned kind,index; uint32_t value; };
struct UnpackProgram {
	std::vector<UnpackEvent> events;
	void Config(unsigned index,uint32_t value) { events.push_back({1,index,value}); }
	void Op(uint32_t value) { events.push_back({2,0,value}); }
	void Drain() { events.push_back({3,0,0}); }
};
// Independent strict subset of ISA8a34e25e UNPACR/MOVD2A/SETADC semantics.
// Delayed unpack publication owns BOTH configuration and L1 until drain.
// This models ownership/order,not cycle-level silicon scheduling/latency.
struct UnpackMachine:VectorLayout {
	const std::vector<uint8_t>& tile;
	bool okay=true,pending=false,flip=false,owner[2]={},clobbered[2]={};
	unsigned unpackBank=0,mathBank=0,pendingBank=0,base=0,output=0,reads=0,unpacks=0;
	unsigned cfg[224]={},tc5=4,x0=7,x1=31,y=23,z=3,w=2;
	UnpackMachine(const std::vector<uint8_t>& bytes,unsigned bank):tile(bytes),unpackBank(bank),mathBank(bank) {
		cfg[1]=0xa0220000; cfg[64]=0x100011; cfg[65]=16; cfg[72]=1;
		cfg[73]=3; cfg[76]=0x7070; cfg[84]=0x400040;
		conditional=true; for (bool& enabled:flags) enabled=false;
	}
	bool Require(bool value) { okay &= value; return okay; }
	void Event(const UnpackEvent& e) {
		if (!okay) return;
		if (e.kind==1) {
			if (!Require(!pending && e.index<224)) return;
			cfg[e.index]=e.value; return;
		}
		if (e.kind==3) {
			if (pending) {
				for (unsigned i=0;i<256;i++) { Raw(output+i/16,i%16,tile[base+i]); reads++; }
				clobbered[pendingBank]=true;
				if (flip) { owner[pendingBank]=true; unpackBank^=1; }
				pending=false;
			}
			return;
		}
		if (!Require(e.kind==2)) return;
		uint32_t op=e.value;
		if (op==0xb2050000 || op==0xb2050004) {
			if (Require(!pending)) tc5=op&65535; return;
		}
		if (op==0x3700000f) { Require(!pending); return; }
		if ((op>>24)==0x5e) {
			if (!Require(!pending && ((op>>21)&7)==1)) return;
			x0=op&1023; x1=(op>>10)&1023; return;
		}
		if (op==0x5120000a) { y=0; return; }
		if (op==0x5420000f) { z=w=0; return; }
		if ((op>>24)==0x42) {
			if (!Require(!pending && (op==0x42000081 || op==0x420000c1)
				&& cfg[1]==0xa03c8000 && !cfg[49] && cfg[64]==0x10001e && cfg[65]==16
				&& cfg[72]==14 && cfg[73]==0x13 && tc5==0 && !x0 && x1==255 && !y && !z && !w
				&& !owner[unpackBank] && (cfg[84]==0x400040 || cfg[84]==0x400140))) return;
			unsigned address=(cfg[76]+1)*16;
			if (!Require(address>=0x90000 && address-0x90000<=tile.size()
				&& tile.size()-(address-0x90000)>=256)) return;
			base=address-0x90000; output=((cfg[84]&65535)/16)-4;
			if (!Require(output==0 || output==16)) return;
			pending=true; flip=op&64; pendingBank=unpackBank; unpacks++; return;
		}
		if (!Require(!pending && owner[mathBank] && clobbered[mathBank]
			&& cfg[1]==0xa0220000 && cfg[64]==0x100011 && cfg[65]==16
			&& cfg[72]==1 && cfg[73]==3 && cfg[76]==0x7070 && cfg[84]==0x400040 && tc5==4)) return;
		if (op!=0x8a000002 && !Require(!conditional)) return;
		if ((op>>24)==8 && !Require(cfg[220]==0)) return;
		VectorLayout::Instruction(op);
	}
	bool Complete() const { return okay && !pending && owner[mathBank] && !owner[mathBank^1] && unpackBank==(mathBank^1); }
};
int main(int argc,char** argv)
{
	assert(argc==1 || argc==2);
	(void)Read; (void)Word;
	unsigned panels=0,weights=0,faults=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned trial=0;trial<64;trial++) {
		std::vector<uint8_t> tile(PioneerQ38PanelTileBytes(type));
		uint32_t seed=0x9de72135^(trial*0x3a917d2b)^type;
		for (auto& byte:tile) { seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; byte=uint8_t(seed); }
		for (unsigned row=0;row<16;row++) {
			unsigned at=(type==14?208:0)*16+row; tile[at]=0; tile[at+16]=0x3c;
			if (type==12) { tile[32+row]=0; tile[48+row]=0x38; }
		}
		for (unsigned panel=0;panel<(type==8?2U:16U);panel++) {
			UnpackProgram p; assert(PioneerQ38VectorUnpack(p,type,panel));
			PioneerQ38Panel expected; assert(PioneerQ38UnpackPanel(type,tile.data(),unsigned(tile.size()),panel,&expected));
			for (unsigned bank=0;bank<2;bank++) {
				auto m=std::make_unique<UnpackMachine>(tile,bank);
				for (const auto& event:p.events) m->Event(event);
				assert(m->Complete() && m->reads==(type==14?512U:256U) && m->unpacks==(type==14?2U:1U));
				for (unsigned i=0;i<256;i++) { assert(m->moves[i]==1 && m->moved[i]==uint16_t(expected.operand[i/2]>>(16*(i%2)))); weights++; }
				for (unsigned row=32;row<1024;row++) if (row<128 || row>=160) for (unsigned col=0;col<16;col++) assert(!m->written[row][col]);
			}
			panels++;
			// Drop each required drain/config/publication/address field in
			// isolation. A malformed model MUST NOT qualify final operands.
			if (!trial) for (unsigned fault=0;fault<13;fault++) {
				auto events=p.events;
				bool changed=false;
				for (unsigned i=0;i<events.size() && !changed;i++) {
					auto& e=events[i];
					if (fault==0 && e.kind==3 && i && events[i-1].kind==2 && (events[i-1].value>>24)==0x42) { events.erase(events.begin()+i); changed=true; }
					if (fault==1 && e.kind==1 && e.index==1 && e.value==0xa03c8000) { e.value^=0x8000; changed=true; }
					if (fault==2 && e.kind==1 && e.index==73 && e.value==0x13) { e.value=3; changed=true; }
					if (fault==3 && e.kind==2 && e.value==0xb2050000) { e.value=0xb2050004; changed=true; }
					if (fault==4 && e.kind==1 && e.index==76) { e.value=0x8ffe; changed=true; }
					if (fault==5 && e.kind==2 && (e.value>>24)==0x5e) { e.value=0x5e200000; changed=true; }
					if (fault==6 && e.kind==1 && e.index==1 && e.value==0xa0220000) { e.value=0xa03c8000; changed=true; }
					if (fault==7 && e.kind==2 && (e.value>>24)==0x42) { e.value^=64; changed=true; }
					if (fault==8 && e.kind==1 && e.index==84) { e.value=0x400000; changed=true; }
					if (fault==9 && e.kind==1 && e.index==64 && e.value==0x10001e) { e.value^=16; changed=true; }
					if (fault==10 && e.kind==2 && e.value==0x5120000a) { events.erase(events.begin()+i); changed=true; }
					if (fault==11 && e.kind==2 && e.value==0xb2050004) { e.value=0xb2050000; changed=true; }
					if (fault==12 && e.kind==2 && e.value==0x8a000002) { events.erase(events.begin()+i); changed=true; }
				}
				assert(changed); auto bad=std::make_unique<UnpackMachine>(tile,0);
				for (const auto& e:events) bad->Event(e);
				assert(!bad->Complete()); faults++;
			}
		}
	}
	unsigned realTiles=0,realPanels=0;
	if (argc==2) {
		FILE* file=fopen(argv[1],"rb"); assert(file); PioneerQwen38Gguf model; assert(model.Open(file));
		for (unsigned tensor=0;tensor<model.Tensors().size();tensor++) {
			const auto& t=model.Tensors()[tensor]; if (!t.type) continue;
			unsigned bytes=PioneerQ38BlockBytes(t.type),rowBytes=unsigned(t.bytes/t.outputs);
			for (unsigned end=0;end<2;end++) {
				std::vector<uint8_t> tile(PioneerQ38PanelTileBytes(t.type));
				unsigned first=end?unsigned(t.outputs)-16:0;
				for (unsigned row=0;row<16;row++) {
					uint8_t raw[210]={}; assert(model.ReadPacked(tensor,uint64_t(first+row)*rowBytes+(end?rowBytes-bytes:0),bytes,raw,bytes));
					for (unsigned b=0;b<bytes;b++) tile[b*16+row]=raw[b];
				}
				for (unsigned panel=0;panel<(t.type==8?2U:16U);panel++) {
					UnpackProgram p; assert(PioneerQ38VectorUnpack(p,t.type,panel));
					PioneerQ38Panel expected; assert(PioneerQ38UnpackPanel(t.type,tile.data(),unsigned(tile.size()),panel,&expected));
					for (unsigned bank=0;bank<2;bank++) {
						auto m=std::make_unique<UnpackMachine>(tile,bank);
						for (const auto& e:p.events) m->Event(e);
						assert(m->Complete());
						for (unsigned i=0;i<256;i++) { assert(m->moves[i]==1 && m->moved[i]==uint16_t(expected.operand[i/2]>>(16*(i%2)))); weights++; }
					}
					realPanels++;
				}
				realTiles++;
			}
		}
		assert(realTiles==996 && model.Stable()); fclose(file);
	}
	for (unsigned type:{0U,8U,12U,14U,99U}) { UnpackProgram p; assert(!PioneerQ38VectorUnpack(p,type,type==8?2:16) && p.events.empty()); }
	printf("PASS OFFLINE raw UNPACR-to-Dst/vector decode:%u synthetic panels+%u real panels/%u real tiles/%u exact weights,both SrcA banks,unsigned byte conversion,context/header/256-byte bounds,two-plane no-early-flip,delayed unpack publication,config restore,explicit all32 lane enable,Dst canaries;%u malformed sequences rejected;NO image/native/cycle-performance qualification\n",panels,realPanels,realTiles,weights,faults);
}
