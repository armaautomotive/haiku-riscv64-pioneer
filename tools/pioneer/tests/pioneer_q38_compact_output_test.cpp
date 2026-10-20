/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_OUTPUT_TRANSFER_MODEL_ONLY
#include "pioneer_q38_output_transfer_test.cpp"
int main()
{
	unsigned layouts=0,transfers=0,faults=0;
	for (unsigned peers=1;peers<=119;peers++) for (unsigned seed=0;seed<16;seed++) {
		uint32_t words=0,expected=0;
		for (unsigned peer=0;peer<peers;peer++) {
			unsigned groups=1+(peer*11+seed)%16;
			assert(words==expected && PioneerQ38CompactOutputAppend(groups,words));
			expected+=groups*16; assert(words==expected);
		}
		assert(PioneerQ38CompactOutputBytes(peers,words)==expected*4); layouts++;
		for (bool edge:{false,true}) {
			Fabric f(peers,edge,expected*4); PioneerQ38OutputTransfer<Fabric> transfer(f);
			assert(transfer.Run(f.d.data()) && transfer.Complete() && !transfer.Unsafe());
			assert(f.host==f.source && f.posted.empty() && f.packets==2*((expected*4+4095)/4096));
			unsigned before=f.io; assert(!transfer.Run(f.d.data()) && before==f.io); transfers++;
		}
	}
	for (uint32_t words:{0U,16U,119U*256U}) for (unsigned bad:{0U,17U,0xffffffffU}) {
		uint32_t original=words; assert(!PioneerQ38CompactOutputAppend(bad,words) && words==original);
	}
	for (uint32_t words:{1U,15U,17U,119U*256U+16U,0xffffffffU}) {
		uint32_t original=words; assert(!PioneerQ38CompactOutputAppend(1,words) && words==original);
	}
	{ uint32_t full=119*256; assert(!PioneerQ38CompactOutputAppend(1,full) && full==119*256); }
	for (unsigned peers:{0U,120U,0xffffffffU}) assert(!PioneerQ38CompactOutputBytes(peers,16));
	for (unsigned bytes:{64U,192U,4032U,4096U,4160U,7616U,121856U}) {
		unsigned peers=(bytes+1023)/1024;
		for (unsigned field=0;field<16;field++) {
			Fabric f(peers,false,bytes); if (field==3) f.d[field]=0;
			else if (field==2) f.d[field]=120; else f.d[field]^=1;
			PioneerQ38OutputTransfer<Fabric> transfer(f);
			assert(!transfer.Run(f.d.data()) && !f.io && !f.packets);
		}
		unsigned packets=2*((bytes+4095)/4096);
		for (unsigned packet=1;packet<=packets;packet++) for (unsigned fault=0;fault<5;fault++) {
			if ((fault==2 && !(packet&1)) || (fault==3 && (packet&1))) continue;
			Fabric f(peers,false,bytes);
			if (!fault) f.fail=packet; else if (fault==1) f.badCounter=packet;
			else if (fault==2) f.dropWrite=packet; else if (fault==3) f.corruptRead=packet; else f.sourceChange=packet;
			PioneerQ38OutputTransfer<Fabric> transfer(f);
			assert(!transfer.Run(f.d.data()) && transfer.Unsafe() && !transfer.Complete());
			assert(f.packets==(fault==2?packet+1:packet));
			unsigned before=f.io; assert(!transfer.Run(f.d.data()) && before==f.io); faults++;
		}
		for (unsigned badBytes:{0U,bytes-1,bytes+1,peers*64-64,peers*1024+64,0xffffffffU}) {
			Fabric f(peers,false,bytes); f.d[7]=badBytes; PioneerQ38OutputTransfer<Fabric> transfer(f);
			assert(!transfer.Run(f.d.data()) && !f.io && !f.packets);
		}
	}
	printf("PASS OFFLINE compact active-row layout:%u heterogeneous layouts/%u full-bit ordered transfers/%u sticky transport faults,ALL1..119 peers/1..16 groups/high48-bit owner/counter wrap,64-byte tails,invalid extents ZERO IO;NO root/driver/native qualification\n",layouts,transfers,faults);
}
