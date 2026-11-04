/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_BLACKHOLE_ARITHMETIC_OBSERVER_H
#define PIONEER_BLACKHOLE_ARITHMETIC_OBSERVER_H
#include "../PioneerBlackholeBf16Arithmetic.h"
// Additional diagnostic wrapper ONLY. The original instruction observer still
// validates every opcode/configuration/source-bank operation unchanged. Override
// ONLY the requested defined output row with this separate integer prediction;
// undefined broadcast replicas never become observable output or a reference.
struct PioneerBlackholeArithmeticObserver {
	template<class Issue> static void Matrix(NativeBfpMachine& machine,unsigned op,bool broadcast,Issue issue) {
		const unsigned ar=machine.rwcA&(broadcast?0x30:0x38),br=machine.rwcB&(broadcast?63:0x38);
		const unsigned phase=(machine.fidelity+machine.thread[11])&3,dr=op&1023;
		assert(ar<=48 && (dr==32 || dr==40));
		const auto& a=machine.source[0][machine.matrixBank[0]];
		const auto& b=machine.source[1][machine.matrixBank[1]];
		if(!a.owner || !b.owner)throw NativeBfpMachine::Unknown;
		uint32_t predicted[16];
		for(unsigned c=0;c<16;c++) {
			uint16_t av[16],bv[16];
			for(unsigned k=0;k<16;k++) {
				const uint32_t ab=NativeBfpMachine::Bits(a.values[ar+k][c]),bb=NativeBfpMachine::Bits(b.values[br][k]);
				assert(!(ab&65535) && !(bb&65535));av[k]=uint16_t(ab>>16);bv[k]=uint16_t(bb>>16);
			}
			const uint32_t previous=machine.valid[dr]?NativeBfpMachine::Bits(machine.destination[dr][c]):0;
			assert(PioneerBlackholeBf16Arithmetic::Instruction(av,bv,phase,previous,predicted[c]));
		}
		issue(); // unchanged original lifecycle/opcode observer must accept FIRST
		if(phase!=machine.dropPhase)for(unsigned c=0;c<16;c++)machine.destination[dr][c]=NativeBfpMachine::Float(predicted[c]);
	}
};
#endif
