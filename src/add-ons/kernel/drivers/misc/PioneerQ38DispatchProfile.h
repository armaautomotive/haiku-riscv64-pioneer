/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_PROFILE_H
#define PIONEER_Q38_DISPATCH_PROFILE_H
// Optional CPU-clock diagnostics ONLY. No device access, completion/source
// authority, ABI change, or control-flow decision may depend on these times.
// Disabled builds contain neither clock reads nor reporting calls.
template<class IO> class PioneerQ38DispatchProfile {
#ifdef TT_PIONEER_Q38_DISPATCH_PROFILE
	IO& io;
	uint64_t previous=0,elapsed[8]={};
	unsigned next=0;
	bool valid=true;
public:
	explicit PioneerQ38DispatchProfile(IO& transport):io(transport),previous(io.Q38CohortNow()) {}
	void Mark() {
		uint64_t now=io.Q38CohortNow();
		if (next>=8) { valid=false; return; }
		if (now<previous) valid=false;
		else elapsed[next]=now-previous;
		previous=now; next++;
	}
	void Publish(unsigned type,unsigned inputs,unsigned peers) {
		io.Q38DispatchProfile(type,inputs,peers,valid && next==8,elapsed);
	}
#else
public:
	explicit PioneerQ38DispatchProfile(IO&) {}
	void Mark() {}
	void Publish(unsigned,unsigned,unsigned) {}
#endif
};
#endif
