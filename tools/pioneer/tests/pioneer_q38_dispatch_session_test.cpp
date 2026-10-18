/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SESSION_MOCK_ONLY
#include "pioneer_q38_cohort_session_test.cpp"
#include "../PioneerQ38DispatchSession.h"
static void CompleteDispatch(const PioneerQ38CohortWave& wave,tt_q38_dispatch_probe& j,unsigned nonce,uint32_t* peer)
{
	j.result={}; auto& r=j.result; r.version=1; r.state=TT_RUNTIME_PASSED;
	r.peers=r.started=r.completed=r.waiting=j.peers; r.root_sources_verified=r.root_started=r.root_completed=r.root_held=1;
	r.nonce=nonce; r.root_phase=0x7200c; r.root_count=j.peers; r.root_marker=0x314d3851; r.root_pc[0]=r.root_pc[1]=0x72038;
	auto* s=r.stats; s[0]=r.root_marker; s[1]=s[2]=1; s[3]=j.type; s[4]=j.inputs; s[5]=j.peers; s[12]=nonce;
	s[10]=s[11]=2*j.peers; unsigned groups=0; for (unsigned w=0;w<j.peers;w++) groups+=j.part[w].groups;
	unsigned reads=s[10]+3*s[11]+2*j.peers,flits=s[10]+3*s[11]+5*j.peers+groups,writes=2*j.peers;
	s[6]=reads; s[7]=s[9]=writes; s[8]=flits;
	for (unsigned c=0;c<8;c++) {
		unsigned delta=c==0 || c==3?reads:c==1?flits:c==2 || c==6?reads+writes:writes;
		s[16+c]=r.before[c]=UINT32_MAX-10-c; s[24+c]=r.after[c]=r.before[c]+delta;
	}
	for (unsigned c=0;c<16;c++) s[32+c]=c?0x12340000+c:0x16005;
	const unsigned routes[]={18|(12<<6),18|(15<<6),18|(18<<6),17|(12<<6),17|(15<<6),17|(18<<6),17|(21<<6)};
	for (unsigned w=0;w<j.peers;w++) {
		auto* p=r.peer[w]; unsigned blocks=j.inputs/PioneerQ38CohortBlockInputs(j.type),allBlocks=blocks*j.part[w].groups;
		unsigned rds=allBlocks+(j.inputs*2+16383)/16384+1;
		unsigned flt=PioneerQ38CohortBytes(j.type,j.inputs,j.part[w].groups)/64+(j.inputs*2+((blocks*4+63)&~63U))/64;
		for (unsigned c=0;c<8;c++) { p[c]=UINT32_MAX-10-c; p[8+c]=p[c]+(c==1?flt:(c==0 || c==2 || c==3 || c==6)?rds:0); }
		p[16]=j.part[w].groups*(j.inputs/16); p[17]=++peer[w]; p[18]=w+1; p[19]=0x32503851;
		p[20]=uint32_t(0x04000000+j.part[w].offset)+(allBlocks-1)*PioneerQ38PanelProbeBytes(j.type);
		p[22]=routes[j.part[w].bank]; p[23]=0x90000; p[25]=PioneerWorkerX(w+1)|(PioneerWorkerY(w+1)<<6);
		p[28]=PioneerQ38PanelProbeBytes(j.type); p[30]=0x16005; p[31]=p[25];
		for (unsigned c=0;c<16;c++) p[32+c]=c?0x12340000+c:0x16005;
		for (unsigned row=0;row<j.part[w].groups*16;row++) j.output[w][row]=PioneerAttentionBits(float(wave.firstRow[w]+row+1));
	}
}
struct DispatchSessionIO:SessionIO {
	uint32_t peer[119]={};
	DispatchSessionIO(FakeSource& source,unsigned tensor):SessionIO(source,tensor) {}
	bool Call(int fd,uint32_t op,void* data,size_t bytes) {
		assert(fd==73 && !stopped);
		if (op==TT_PCI_PROBE_Q38_DISPATCH_SUPPORT) {
			assert(bytes==sizeof(tt_q38_dispatch_support));
			unsigned driver=fault==1?52U:fault==14?(TT_Q38_DISPATCH_DRIVER==55?54U:55U):
				fault==15?(TT_Q38_DISPATCH_DRIVER==56?54U:56U):fault==16?(TT_Q38_DISPATCH_DRIVER==57?54U:57U):unsigned(TT_Q38_DISPATCH_DRIVER);
			*static_cast<tt_q38_dispatch_support*>(data)={driver,1,119,256,17408,7,64,1,0}; return true;
		}
		if (op!=TT_PCI_PROBE_Q38_DISPATCH_RUN) return SessionIO::Call(fd,op,data,bytes);
		assert(bytes==sizeof(tt_q38_dispatch_probe)); runs++;
		auto& r=*static_cast<tt_q38_dispatch_probe*>(data); unsigned waves=schedule.Jobs().back().wave+1;
		PioneerQ38CohortWave wave; assert(wave.Build(source.Model(),layout,schedule,cursor%waves));
		assert(!memcmp(r.part,wave.part,sizeof(r.part))); CompleteDispatch(wave,r,runs,peer); cursor++;
		if (runs==failWave) switch(fault) {
			case 4:stopped=true;return false;
			case 5:r.result.peer[r.peers-1][15]++;stopped=true;break;
			case 6:r.output[r.peers-1][r.part[r.peers-1].groups*16-1]=0x7fc00000;stopped=true;break;
			case 7:source.valid=false;stopped=true;break;
			case 8:r.activation[0]^=1;stopped=true;break;
			case 10:r.result.nonce--;stopped=true;break;
			case 11:r.result.peer[0][30]^=1;r.result.peer[0][32]^=1;stopped=true;break;
			case 12:r.result.stats[47]^=1;stopped=true;break;
			case 13:r.output[r.peers][0]=1;stopped=true;break;
		}
		return true;
	}
};
int main()
{
	(void)&Check; (void)&CohortHostTestMain;
	Fixture fixture; FakeSource source(fixture.file);
	const auto* tensor=source.Model().Find("blk.3.attn_k.weight"); assert(tensor); unsigned index=unsigned(tensor-source.Model().Tensors().data());
	std::vector<float> input(5120,.125f),output(1024,-99),before=output;
	{
		DispatchSessionIO io(source,index); PioneerQ38DispatchSession<DispatchSessionIO,FakeSource> session(io,source);
		assert(session.Open() && !session.Open() && io.begins==1);
		assert(session.Multiply(index,input.data(),5120,output.data(),1024,1));
		for (unsigned row=0;row<output.size();row++) assert(output[row]==float(row+1));
		unsigned uploads=io.uploads,runs=io.runs; assert(uploads && runs>1);
		assert(session.Multiply(index,input.data(),5120,output.data(),1024,1) && io.uploads==uploads && io.runs==runs*2);
		assert(session.Finish() && session.Finish() && io.ends==1 && io.closes==1);
	}
	for (unsigned fault=1;fault<=13;fault++) {
		source.valid=true; DispatchSessionIO io(source,index); io.fault=fault; io.failWave=2;
		PioneerQ38DispatchSession<DispatchSessionIO,FakeSource> session(io,source); output=before;
		bool opened=session.Open(); assert(opened==(fault>2));
		if (opened) {
			bool result=session.Multiply(index,input.data(),5120,output.data(),1024,1); assert(result==(fault==9));
			if (fault!=9) assert(output==before && io.runs==(fault==3?0U:2U));
		}
		assert(!session.Finish() && session.Failed() && !session.Open() && io.closes==1 && io.ends==(fault==9?1U:0U));
		unsigned calls=io.uploads+io.runs+io.ends;
		assert(!session.Multiply(index,input.data(),5120,output.data(),1024,1) && !session.Finish() && calls==io.uploads+io.runs+io.ends);
	}
	for (unsigned wrong:{14U,15U,16U}) {
		DispatchSessionIO io(source,index); io.fault=wrong;
		PioneerQ38DispatchSession<DispatchSessionIO,FakeSource> session(io,source);
		assert(!session.Open() && !io.begins && !io.uploads && !io.runs && io.closes==1 && !session.Finish());
	}
	source.valid=false;
	{
		DispatchSessionIO io(source,index); PioneerQ38DispatchSession<DispatchSessionIO,FakeSource> session(io,source);
		assert(!session.Open() && !io.opens && !io.begins && !session.Finish());
	}
	puts("PASS OFFLINE54 model session:strict metadata distinct119-WAIT protocol,authenticated source BEFORE FD,ALL bulk proofs,full multiwave atomic assembly/repeated retained weights,13 ABI/storage/source/late-wave/proof/source-seal faults preserve output,CLOSE ONLY/no END/retry/CPU products/fallback;clean END once;NO native inference qualification");
}
