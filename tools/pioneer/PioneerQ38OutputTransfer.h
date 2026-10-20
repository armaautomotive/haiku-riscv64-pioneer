/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_OUTPUT_TRANSFER_H
#define PIONEER_Q38_OUTPUT_TRANSFER_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE prerequisite ONLY. No installed driver/root selects this helper.
// Shape is NOT authority: a future kernel must FIRST own/prove the entire
// private1MiB host mapping, ALL120 roles, original source seals and BME.
// All peers must be at proved WAIT before this final root-only transfer.
// Physical words are KERNEL-PRIVATE, never an ioctl/public descriptor.
// Source80000..9dc00 and readback75000..76000 are INSIDE the existing root
// L1 owner, disjoint from code/stack/descriptors/finished proof records.
enum { PIONEER_Q38_OUTPUT_MAGIC=0x314f3851,PIONEER_Q38_OUTPUT_OWNER=0x100000 };
static inline bool PioneerQ38OutputTransferShape(const uint32_t* d)
{
	if (!d || d[0]!=PIONEER_Q38_OUTPUT_MAGIC || d[1]!=1 || !d[2] || d[2]>119
		|| !d[3] || d[3]>=0x7fffffff || (d[4]&0xfffff) || !d[5] || d[5]>0xffff
		|| d[6]!=PIONEER_Q38_OUTPUT_OWNER || d[7]!=d[2]*1024) return false;
	for (unsigned i=8;i<16;i++) if (d[i]) return false;
	return true;
}
// IO supplies fixed root MMIO/L1 accesses and a fence, not allocation,
// reset/restore/END/release capabilities. A failure is sticky: NO later IO.
// NoC ACK alone is NOT host visibility. Each write is followed by an
// ordered host read AND an on-card bitwise comparison to the original.
// This is transport integrity, not CPU learned-matrix/result validation.
template<class IO> class PioneerQ38OutputTransfer {
	IO& io;
	uint32_t used=0,unsafe=0,complete=0; // qualified word-store subset
	static unsigned Source(unsigned i) {
		return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;
	}
	bool Sources(const uint32_t* d) {
		for (unsigned i=0;i<16;i++) if (io.ReadReg(Source(i))!=d[16+i]) return false;
		return true;
	}
	bool Idle() {
		for (unsigned i=0;i<4;i++) if (io.ReadReg(0x40+i*0x800)) return false;
		for (unsigned i=0;i<32;i++) if (io.ReadReg(0x240+i*4)&255) return false;
		return true;
	}
	bool Packet(bool write,unsigned at,unsigned bytes,const uint32_t* d) {
		if (!bytes || bytes>4096 || (bytes&1023) || (at&4095) || at>=d[7] || bytes>d[7]-at
			|| !Sources(d) || !Idle()) return false;
		const unsigned offsets[]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
		uint32_t expected[4];
		for (unsigned i=0;i<4;i++) expected[i]=io.ReadReg(offsets[i])+(i==1?bytes/64:1);
		const uint32_t command[]={write?0x80000+at:d[4]+at,write?0U:d[5],write?129U:1555U,
			write?d[4]+at:0x75000,write?d[5]:0U,write?1555U:129U,0,write?0x212U:0,bytes,0};
		for (unsigned i=0;i<10;i++) io.WriteReg(i*4,command[i]);
		for (unsigned i=0;i<10;i++) if (io.ReadReg(i*4)!=command[i]) return false;
		io.Fence(); io.WriteReg(0x40,1); (void)io.ReadReg(0x40); io.Fence();
		for (unsigned attempt=0;attempt<65536;attempt++) {
			bool done=!io.ReadReg(0x40);
			for (unsigned i=0;i<4;i++) done &= io.ReadReg(offsets[i])==expected[i];
			if (done) return Idle() && Sources(d);
		}
		return false;
	}
public:
	explicit PioneerQ38OutputTransfer(IO& transport):io(transport) {}
	bool Unsafe() const { return unsafe; }
	bool Complete() const { return complete; }
	bool Run(const uint32_t* d) {
		if (used || unsafe || !d) return false;
		uint32_t frozen[32]; for (unsigned i=0;i<32;i++) frozen[i]=d[i];
		if (!PioneerQ38OutputTransferShape(frozen)) return false;
		used=true;
		for (unsigned at=0;at<frozen[7];at+=4096) {
			unsigned bytes=frozen[7]-at; if (bytes>4096) bytes=4096;
			if (!Packet(true,at,bytes,frozen) || !Packet(false,at,bytes,frozen)) { unsafe=true; return false; }
			for (unsigned w=0;w<bytes/4;w++) if (io.ReadL1(0x80000+at+w*4)!=io.ReadL1(0x75000+w*4)) {
				unsafe=true; return false;
			}
		}
		for (unsigned i=0;i<32;i++) if (frozen[i]!=d[i]) { unsafe=true; return false; }
		io.Fence(); complete=true; return true;
	}
};
#endif
