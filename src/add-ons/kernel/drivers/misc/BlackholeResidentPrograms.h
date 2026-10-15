/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RESIDENT_PROGRAMS_H
#define BLACKHOLE_RESIDENT_PROGRAMS_H
#include "BlackholeResidentMathImage.h"
#include "BlackholeResidentGemvImage.h"

// Kernel-selected fixed programs, never user-defined executable code,
// coordinates, NoC flags or memory descriptors.
struct BlackholeResidentMatrixProgram {
	bool Valid() const { return true; }
	unsigned Version() const { return 27; }
	unsigned CodeWords() const { return TT_RESIDENT_MATH_CODE_WORDS; }
	unsigned ImageWords() const { return TT_RESIDENT_MATH_IMAGE_WORDS; }
	unsigned StatusIndex() const { return TT_RESIDENT_MATH_STATUS_INDEX; }
	unsigned GddrWords() const { return TT_RESIDENT_MATH_GDDR_WORDS; }
	unsigned OutputWords() const { return 256; }
	unsigned Fixtures() const { return 2; }
	unsigned Requests() const { return 2; }
	unsigned Flits() const { return 34; }
	uint32_t FinalPhase() const { return TT_UNPACK_FINAL_PHASE; }
	uint32_t ParkPc() const { return TT_UNPACK_PARK_PC; }
	uint32_t ImageWord(unsigned w) const { return BlackholeResidentMathImageWord(w); }
	uint32_t GddrWord(unsigned w) const { return BlackholeResidentMathGddrWord(w); }
	uint32_t CommandWord(unsigned w) const { return BlackholeResidentMathCommandWord(w); }
	uint32_t ExpectedWord(unsigned w) const { return BlackholeNonuniformExpected(21,w); }
};
struct BlackholeResidentGemvProgram:BlackholeResidentMatrixProgram {
	const unsigned inputs;
	explicit BlackholeResidentGemvProgram(unsigned count):inputs(count) {}
	bool Valid() const { return BlackholeResidentGemvShape(inputs); }
	unsigned Version() const { return 28; }
	unsigned GddrWords() const { return (inputs/16)*TT_RESIDENT_MATH_GDDR_WORDS; }
	unsigned OutputWords() const { return 128; }
	unsigned Fixtures() const { return inputs/16; }
	unsigned Requests() const { return inputs/16; }
	unsigned Flits() const { return (inputs/16)*17; }
	uint32_t FinalPhase() const { return TT_RESIDENT_GEMV_FINAL_PHASE; }
	uint32_t ParkPc() const { return TT_RESIDENT_GEMV_PARK_PC; }
	uint32_t ImageWord(unsigned w) const { return BlackholeResidentGemvImageWord(w,inputs); }
	uint32_t GddrWord(unsigned w) const { return BlackholeResidentGemvGddrWord(w,inputs); }
	uint32_t ExpectedWord(unsigned w) const { return BlackholeResidentGemvExpected(w,inputs); }
};
#endif
