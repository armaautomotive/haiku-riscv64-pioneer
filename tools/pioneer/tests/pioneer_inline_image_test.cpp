/* SPDX-License-Identifier: MIT */
#define PIONEER_PERSISTENT_MACHINE_ONLY
#include "pioneer_persistent_image_test.cpp"
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeQ8InlineCodeWord(w));
	assert(BlackholeRuntimeQ8InlineCodeWord((TT_RUNTIME_Q8_PARK_PC-0x70000)/4)==0x0000006f);
	for (unsigned worker=0;worker<120;worker++) {
		PersistentMachine m(1024,32,worker); m.Inline(); m.Boot(); m.Job(0); m.Job(1); m.Job(9); m.Stop();
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		PersistentMachine m(k,64,119); m.Inline(); m.Boot(); m.Job(2); m.Job(3); m.Stop();
	}
	PersistentMachine full(1024,4096,1); full.Inline(); full.Boot(); full.Job(0); full.Job(1); full.Stop();
	PersistentMachine legacy(1024,32,0); legacy.Boot(); legacy.Job(0);
	PersistentMachine direct(1024,32,0); direct.Inline(); direct.Boot(); direct.Job(0);
	assert(direct.steps<legacy.steps && !memcmp(direct.buffers+4096,legacy.buffers+4096,32*4));
	printf("PASS OFFLINE actual inline .ttinsn image,all120 coordinates,all K,N4096,repeated exact IEEE outputs without reset,RISC steps%u vs%u; NOT hardware/cycle or inference performance\n",direct.steps,legacy.steps);
}
