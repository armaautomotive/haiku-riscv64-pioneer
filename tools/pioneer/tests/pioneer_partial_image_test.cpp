/* SPDX-License-Identifier: MIT */
#define PIONEER_PERSISTENT_MACHINE_ONLY
#include "pioneer_persistent_image_test.cpp"
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8PartialCodeWord(w));
	assert(Word(code,TT_RUNTIME_Q8_PARK_PC-0x70000)==0x0000006f);
	unsigned different=0;
	for (unsigned worker=0;worker<120;worker++) {
		PersistentMachine m(1024,32,worker); m.Partial(); m.Boot();
		for (unsigned t:{0U,1U,9U}) { m.Job(t); different+=!m.CorrectIeee(); }
		m.Stop();
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		PersistentMachine m(k,64,119); m.Partial(); m.Boot(); m.Job(2); m.Job(3); m.Stop(); different+=!m.CorrectIeee();
	}
	PersistentMachine full(1024,4096,1); full.Partial(); full.Boot(); full.Job(0); full.Job(1); full.Stop(); different+=!full.CorrectIeee();
	assert(different); // This candidate must NOT be mislabeled IEEE-exact.
	PersistentMachine direct(1024,32,1),precise(1024,32,1);
	direct.Partial(); precise.Inline(); direct.Boot(); precise.Boot(); direct.Job(0); precise.Job(0);
	printf("PASS OFFLINE partial image word-for-word/all120 peers/all K/N4096/repeated jobs/STOP/native oracle;IEEE-different jobs%u,steps%u vs exact%u;NOT native quality or speed\n",different,direct.steps,precise.steps);
}
