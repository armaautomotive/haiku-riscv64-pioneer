/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_ROWS_MODEL_ONLY
#include "pioneer_q38_vector_rows_image_test.cpp"
#include "../PioneerQ38VectorDstInit.h"
#include <sys/wait.h>
#include <unistd.h>
struct Initialization {
	std::vector<uint32_t> operations;
	uint32_t config[256]={},thread=99; unsigned drains=0,unpacks=0;
	bool bfloat,metadata,fixed;
	Initialization(bool b,bool m,bool f):bfloat(b),metadata(m),fixed(f) {}
	void Config(unsigned at,uint32_t value) { assert(at<256); config[at]=value; }
	void Drain() { drains++; }
	void Op(uint32_t op) {
		operations.push_back(op);
		if ((op&0xffff0000)==0xb2050000) thread=op&65535;
		if ((op>>24)==0x42) {
			assert(op==0x42000091 && thread==0 && config[1]==(bfloat?0x20200000U:0xa0200000U)
				&& config[49]==0 && config[64]==0x100010 && config[65]==16 && config[72]==0
				&& config[73]==0x13 && config[76]==0x9fff);
			assert(config[84]==0x00400000+((unpacks?96:64)+4)*16 && unpacks<(metadata?2U:1U));
			unpacks++;
		}
	}
};
static void UndefinedMustAbort(const char* image,bool omit,bool all)
{
	pid_t child=fork(); assert(child>=0);
	if (!child) {
		assert(freopen("/dev/null","w",stderr));
		VectorRowsMachine m(image,12,5120,1);
		m.omitDstInitialization=omit; m.allRowsClear=all;
		(void)m.RunVector(); _exit(0);
	}
	int status=0; assert(waitpid(child,&status,0)==child);
	assert(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
}
int main(int argc,char** argv)
{
	assert(argc==2);
	for (bool bf:{false,true}) for (bool metadata:{false,true}) for (bool fixed:{false,true}) {
		Initialization e(bf,metadata,fixed); PioneerQ38VectorDstInit(e,bf,metadata,fixed);
		assert(e.unpacks==(metadata?2U:1U) && e.drains==e.unpacks+2 && e.thread==4
			&& e.config[1]==(bf?0x203c8000U:fixed?0xa03c8000U:0xa0220000U)
			&& e.config[64]==((bf || fixed)?0x10001eU:0x100011U)
			&& e.config[72]==((bf || fixed)?14U:1U)
			&& e.config[73]==((bf || fixed)?0x13U:3U) && e.config[76]==0x7070 && e.config[84]==0x00400040);
	}
	UndefinedMustAbort(argv[1],true,false);
	UndefinedMustAbort(argv[1],false,true);
	for (unsigned type:{8U,12U,14U}) {
		VectorRowsMachine m(argv[1],type,5120,1); m.truncateRawByte=true;
		assert(m.RunVector() && !m.CorrectRows());
	}
	puts("PASS Dst initialization/config restoration,undefined partial writes and ALL-row ZEROACC rejected,mode5 truncation caught for ALL quant types;OFFLINE ISA observer only");
}
