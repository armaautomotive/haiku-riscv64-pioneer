/* SPDX-License-Identifier: MIT */
#define PIONEER_PERSISTENT_MACHINE_ONLY
#include "pioneer_persistent_image_test.cpp"
static void CheckPhases(const PersistentMachine& m) {
	assert(!m.fidelityPhase && !m.ownerA && !m.ownerB && m.CorrectIeee() && m.Guards());
	for (unsigned n:m.phaseVisits) assert(n*4==m.matrixInstructions);
}
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned i=0;i<1024;i++) assert(Word(code,i*4)==BlackholeRuntimeQ8PhaseCodeWord(i));
	for (unsigned worker=0;worker<120;worker++) {
		PersistentMachine m(1024,32,worker); m.Phase(); m.Boot();
		for (unsigned t:{0U,1U,9U}) { m.Job(t); CheckPhases(m); } m.Stop();
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		PersistentMachine m(k,64,119); m.Phase(); m.Boot(); m.Job(2); CheckPhases(m); m.Job(3); CheckPhases(m); m.Stop();
	}
	PersistentMachine full(1024,4096,1); full.Phase(); full.Boot(); full.Job(0); CheckPhases(full); full.Job(1); CheckPhases(full); full.Stop();
	PersistentMachine direct(1024,32,1),precise(1024,32,1); direct.Phase(); precise.Inline();
	direct.Boot(); precise.Boot(); direct.Job(0); precise.Job(0); CheckPhases(direct);
	assert(direct.steps<precise.steps && direct.drains<precise.drains && direct.matrixInstructions==precise.matrixInstructions);
	printf("PASS OFFLINE exact four-phase pipeline,all120 peers/all K/N4096/repeated IEEE outputs/STOP,balanced fidelity/source counters;steps%u vs%u,drains%u vs%u;NOT native performance\n",direct.steps,precise.steps,direct.drains,precise.drains);
}
