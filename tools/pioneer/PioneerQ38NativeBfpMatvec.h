/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP_MATVEC_H
#define PIONEER_Q38_NATIVE_BFP_MATVEC_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Unqualified native BFP prototype is not a driver,Work mode or CPU reference
#endif

// NEW numerical/operand path,NOT the current integer-overlay worker.
// Entry:exclusive reset/idle FPU+unpacker context,private Config0/thread0,
// no live Dst,source-bank or prior compute owner. Setup does not PROVE that
// launch invariant. No installed driver/Assistant selects these operations.
//
// Fixed owned L1 data only:
// 90000:one cached32x32 weight tile (64 exponents then4 native faces),
// 91000:BF16 activations,8 repeated rows of K[0:16],then8 of K[16:32].
// Both have a private16-byte header immediately BEFORE the payload.
// Weights are output-row-major. UNP0 transposes EACH16x16 face into SrcA;
// a single unpack fills all64 SrcA rows. SrcB holds both activation halves.
// Outputs are FP32 Dst rows32 and40 (read only after final completion).
//
// Facts audited against official tt-isa8a34e25e and cfg81989dcdb,not vendor
// code copied:UNPACR_Regular,SrcASrcB,MVMUL,SETRWC,MatrixUnit. Explicit
// formats avoid incompletely characterized implied-SrcB behavior. BFP4 A
// needs phases0/2 with BF16 B (NOT phase0 alone);BFP8 A needs ALL4. Only
// the final matrix instruction releases/flips BOTH source banks. No raw
// GGUF decode,affine metadata,SFPU bit manipulation,NoC,host IO or fallback.
// This is an instruction producer,NOT native ownership,watchdog,bank-plan,
// model quality,physical performance or Work-promotion authority.
struct PioneerQ38NativeBfpMatvec {
	static_assert(sizeof(unsigned)==4,"RV32/32-bit instruction words required");
	static constexpr unsigned WeightAddress=0x90000,InputAddress=0x91000;
	// Pinned blackhole/tensix_types.h: Bfp4_b=7,Bfp8_b=6 (10 is LF8).
	static constexpr unsigned Format(unsigned bits) { return bits==4?7:bits==8?6:0; }
	static constexpr unsigned WeightBytes(unsigned bits) { return bits==4?576:bits==8?1088:0; }
	template<class E> static bool Setup(E& e,unsigned bits) {
		const unsigned format=Format(bits);if(!format)return false;
		e.Op(0xb2000000);e.Drain(); // select Config0 BEFORE other thread writes
		for(unsigned word=1;word<68;word++)e.Op(0xb2000000|(word<<16));
		e.Op(0xb2020001);e.Op(0xb2030001); // disable implied formats
		e.Op(0xb2050004); // supported address override for SrcA/SrcB unpack
		// AddrMod1:only fidelity increments;A4 phases0,2 or A8 phases0,1,2,3.
		e.Op(bits==4?0xb21d4000:0xb21d2000);e.Drain();
		e.Config(0,0x42b0|format);
		e.Config(1,0x60a00000|(format<<17)); // FP32 FPU/SFPU,no integer math
		e.Config(2,0);e.Config(3,0);e.Config(6,0);e.Config(7,0);e.Config(220,0);
		// Only channel0/1 context0 is used. Explicit bases/offsets remove
		// any dependence on a previous tile's register contents.
		e.Config(49,0);e.Config(50,0x100);e.Config(56,0);e.Config(57,0);
		e.Config(58,0);e.Config(59,0);e.Config(61,0);e.Config(62,0);
		e.Config(64,0x01000010|format); // X256,uncompressed,EXP section present
		e.Config(65,0x00040001);e.Config(66,1);e.Config(67,0); // Y1,Z4,W1,header16
		e.Config(70,8);e.Config(71,0); // explicit x4 throttle,NOT FP8-E4M3
		e.Config(72,0x120|format); // same BFP output,transpose WITHIN each face
		e.Config(73,0x000f000f);e.Config(74,0x17fff);e.Config(75,0);
		e.Config(76,WeightAddress/16-1);e.Config(84,0x00400040);
		e.Config(86,0x01000100);e.Config(92,0); // context X256,zero tile offset
		e.Config(112,0x01000015);e.Config(113,0x00010001);
		e.Config(114,1);e.Config(115,0);
		e.Config(118,8);e.Config(119,0);
		e.Config(120,0x25);e.Config(121,0x000f000f);
		e.Config(122,0x17fff);e.Config(123,0);e.Config(124,InputAddress/16-1);
		e.Config(140,0);e.Drain();return true;
	}
	template<class E> static bool Run(E& e,unsigned bits) {
		if(!Format(bits))return false;
		e.Op(0x3700000f); // ALL RWCs and fidelity start at zero
		e.Op(0x10180000); // sole private Dst:invalidate ALL,both physical halves
		e.Op(0x5e2ffc00); // UNP0 input1024 datums;X0=0/X1=1023
		e.Op(0x5e43fc00); // UNP1 input256 BF16 datums
		e.Op(0x5160000b);e.Op(0x5460000f); // preserve X1;all Y/Z/W zero
		e.Op(0x420000c1);e.Op(0x428000c1);e.Drain(); // publish complete A/B
		for(unsigned face=0;face<4;face++) {
			if(face) { // SETRWC's immediate is only4bits;increment by8 TWICE
				e.Op(0x37040201);e.Op(0x37040201);
			}
			e.Op(face&1?0x3700200a:0x3700000a); // B row8/0,fidelity0
			const unsigned dst=face<2?32:40,phases=bits==4?2:4;
			for(unsigned phase=0;phase<phases;phase++)
				e.Op(0x26004000|dst|((face==3 && phase==phases-1)?0x00c00000:0));
		}
		// Matrix-unit instructions are queued in order;no source/config/L1
		// rewrite or scalar result read can interleave. This REAL completion
		// is mandatory before output reads or next-tile reuse.
		e.Drain();return true;
	}
};
#endif
