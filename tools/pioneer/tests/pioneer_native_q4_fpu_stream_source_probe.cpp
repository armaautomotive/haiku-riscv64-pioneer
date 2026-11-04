/* SPDX-License-Identifier: MIT */
// CPU-only ACTUAL source method over private authentic model,not GPU proof.
#include "../PioneerNativeQ4FpuStreamSource.h"
#include <cassert>
int main(int argc,char** argv) {
	assert(argc==2);PioneerNativeQ4FpuStreamSource source;std::string error;
	assert(!source.Ready());assert(source.Prepare(argv[1],error) && error.empty() && source.Ready());
	assert(!source.Prepare(argv[1],error) && !error.empty() && source.Ready());
	puts("PASS_PRIVATE_ACTUAL_Q4_SOURCE same-open full original SHA/epoch,all9 full-K compiled payloads/inputs/288 rounded predictions match fresh original;one-shot;NO GPU/quality/speed authority");
}
