/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SPAN_MACHINE_ONLY
#include "pioneer_q38_span_image_test.cpp"
static void Work(Q38SpanMachine& m) { m.descriptor[2]=m.memory[0x140008]=2; }
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned bank=0;bank<7;bank++) for (bool edge:{false,true}) for (unsigned bytes:{4096U,20480U,1048576U}) {
		Q38SpanMachine work(argv[1],bytes,bank,edge); Work(work); assert(work.Run()); work.Verify();
		assert(work.transfers==2*((bytes+16383)/16384) && !work.mirrorStores && !work.echoStores);
		Q38SpanMachine checked(argv[1],bytes,bank,edge); assert(checked.Run()); checked.Verify();
		assert(checked.transfers==7*((bytes+16383)/16384));
	}
	for (unsigned packet:{1U,2U,128U}) for (unsigned fault=0;fault<3;fault++) {
		Q38SpanMachine m(argv[1],1048576); Work(m);
		if (!fault) m.failPacket=packet; else if (fault==1) m.badCounter=packet; else m.changeSourceAfter=packet;
		assert(!m.Run() && m.transfers==packet);
	}
	for (unsigned source=0;source<16;source++) {
		Q38SpanMachine m(argv[1],4096); Work(m); m.seals[source]^=1; assert(!m.Run() && !m.transfers);
	}
	for (unsigned packet:{0U,1U}) {
		Q38SpanMachine m(argv[1],4096); Work(m); m.corruptPacket=int(packet);
		assert(m.Run() && m.gddr!=m.golden && m.hostData==m.golden); // intentionally NO numeric validator
	}
	puts("PASS ACTUAL Work span: all7 banks/edges/4KiB..1MiB, TWO packets/chunk, ZERO host writes/CPU-RISC data copies, strict7-packet mode preserved, ALL command/source/counter/ACK/idle faults inert-park, deliberate numeric corruption accepted ONLY in Work; NO DEVICE IO");
}
