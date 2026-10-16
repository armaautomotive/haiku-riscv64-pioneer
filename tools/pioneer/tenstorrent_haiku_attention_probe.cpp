/* SPDX-License-Identifier: MIT */
// Explicit, public numerical qualification only. Not an Assistant backend.
#include "TenstorrentHaikuTile.h"
#include "PioneerAttentionFixture.h"
#include "PioneerFloat16.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

static double Seconds() { timespec t={}; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
struct Fixture {
	unsigned tokens=0,layer=0,groups=0;
	std::vector<float> q,expected;
	std::vector<uint16_t> k,v;
	bool Load(const char* path) {
		FILE* f=fopen(path,"rb"); if (!f) return false;
		PioneerAttentionFixture h={}; bool valid=fread(&h,sizeof(h),1,f)==1 && PioneerAttentionFixtureValid(h);
		if (valid) {
			tokens=h.tokens; layer=h.layer; groups=8;
			q.resize(h.queryCount); expected.resize(h.resultCount); k.resize(h.kvCount); v.resize(h.kvCount);
			valid=fread(q.data(),4,q.size(),f)==q.size() && fread(k.data(),2,k.size(),f)==k.size()
				&& fread(v.data(),2,v.size(),f)==v.size() && fread(expected.data(),4,expected.size(),f)==expected.size()
				&& fgetc(f)==EOF;
		}
		if (fclose(f)) valid=false;
		return valid;
	}
	void Synthetic(unsigned n) {
		tokens=n; groups=1; q.resize(256); expected.resize(256); k.resize(n*128); v.resize(n*128);
		for (unsigned j=0;j<q.size();j++) q[j]=float(sin(j*.713)*1.7);
		for (unsigned j=0;j<k.size();j++) { k[j]=PioneerFloat16(float(sin(j*.429)*1.3)); v[j]=PioneerFloat16(float(cos(j*.139)*2)); }
		for (unsigned h=0;h<2;h++) PioneerAttentionHead(q.data()+h*128,k.data(),v.data(),n,128,expected.data()+h*128);
	}
	bool Pack(unsigned group,tt_pioneer_attention_tile_test& p,float* precise) const {
		memset(&p,0,sizeof(p)); p.version=1; p.tokens=tokens;
		memcpy(p.query,q.data()+group*256,1024);
		std::vector<uint16_t> pk(tokens*128),pv(tokens*128);
		for (unsigned t=0;t<tokens;t++) for (unsigned d=0;d<128;d++) {
			pk[t*128+d]=k[t*groups*128+group*128+d]; pv[t*128+d]=v[t*groups*128+group*128+d];
		}
		for (unsigned w=0;w<tokens*64;w++) {
			p.key[w]=pk[w*2]|(uint32_t(pk[w*2+1])<<16);
			p.value[w]=pv[w*2]|(uint32_t(pv[w*2+1])<<16);
		}
		return PioneerAttentionTileInputValid(tokens,p.query,p.key,p.value)
			&& PioneerAttentionHead(q.data()+group*256,pk.data(),pv.data(),tokens,128,precise)
			&& PioneerAttentionHead(q.data()+group*256+128,pk.data(),pv.data(),tokens,128,precise+128);
	}
};
static double Error(const uint32_t* result,const float* expected)
{
	double delta=0,norm=0;
	for (unsigned j=0;j<256;j++) {
		float a=PioneerAttentionFloat(result[j]);
		if (!isfinite(a) || !isfinite(expected[j])) return INFINITY;
		double d=double(a)-expected[j]; delta+=d*d; norm+=double(expected[j])*expected[j];
	}
	return sqrt(delta/(norm?norm:1));
}
int main(int argc,char** argv)
{
	bool support=argc==2 && !strcmp(argv[1],"--check-support");
	bool synthetic=argc==3 && !strcmp(argv[1],"--run-synthetic");
	bool files=argc>=3 && !strcmp(argv[1],"--run-public-fixtures");
	if (!support && !synthetic && !files) {
		fprintf(stderr,"Usage: %s --check-support | --run-synthetic TOKENS_1_TO_256 | --run-public-fixtures FIXTURE...\nExplicit exclusive qualification; no firmware/model/Assistant changes, no full-model speed claim.\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0);
	std::vector<Fixture> fixtures;
	if (synthetic) {
		char* end=NULL; unsigned long n=strtoul(argv[2],&end,10);
		if (!end || end==argv[2] || *end || n<1 || n>256) return 2;
		fixtures.resize(1); fixtures[0].Synthetic(unsigned(n));
	} else if (files) {
		if (argc>58) return 2; // bounded public capture set
		fixtures.resize(argc-2);
		for (int i=2;i<argc;i++) if (!fixtures[i-2].Load(argv[i])) { fprintf(stderr,"Invalid public fixture; no device opened\n"); return 1; }
	}
	auto* request=static_cast<tt_pioneer_attention_tile_test*>(calloc(1,sizeof(tt_pioneer_attention_tile_test)));
	if (!request) return 1;
	// Validate every public input/reference before opening or reserving card.
	for (const auto& f:fixtures) for (unsigned g=0;g<f.groups;g++) {
		float precise[256];
		if (!f.Pack(g,*request,precise)) { fprintf(stderr,"Unsupported/nonfinite input; no card opened\n"); free(request); return 1; }
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR);
	if (fd<0) { perror("attention open"); free(request); return 1; }
	tt_resident_session_result session={};
	int status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_SUPPORT,&session,sizeof(session));
	bool passed=!status && session.version==1 && (session.capabilities&TT_PIONEER_CAP_ATTENTION_TILE_TEST);
	printf("Attention qualification capability=%u status=%d; dedicated session only, NOT Assistant attention\n",passed,status);
	if (!passed || support) { close(fd); free(request); return passed?0:1; }
	session={}; session.version=1; session.bytes=4096;
	status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_BEGIN,&session,sizeof(session));
	bool active=session.active!=0,cold=session.cold!=0,closeOnly=status!=0;
	passed=!status && active && !cold && session.state==TT_RESIDENT_SESSION_READY;
	unsigned calls=0; double started=Seconds(),deviceWall=0;
	for (unsigned fi=0;fi<fixtures.size() && passed;fi++) {
		const auto& f=fixtures[fi];
		for (unsigned group=0;group<f.groups && passed;group++) {
			float precise[256]; passed=f.Pack(group,*request,precise); if (!passed) break;
			double before=Seconds();
			status=ioctl(fd,TT_PCI_PROBE_PIONEER_ATTENTION_TILE_TEST,request,sizeof(*request));
			double seconds=Seconds()-before; deviceWall+=seconds;
			const auto& r=request->result; cold=r.recovery_required!=0; closeOnly=status!=0;
			passed=!status && r.version==1 && r.state==TT_ATTN_TILE_PASSED && r.prepared && r.started
				&& r.completed && r.stopped && !cold && r.tokens==f.tokens && r.count==f.tokens && !r.nonfinite
				&& r.staged_words==256+f.tokens*128 && r.phase==TT_ATTENTION_TILE_DONE && r.marker==TT_ATTENTION_TILE_MARKER
				&& r.pc[0]==TT_ATTENTION_TILE_PARK && r.pc[1]==r.pc[0];
			for (unsigned w=0;w<4;w++) passed &= r.before[w]==r.after[w];
			printf("ATTENTION fixture=%u layer=%u group=%u n=%u state=%u launch/complete/stop=%u/%u/%u phase/count/marker/pc=%x/%u/%x/%x cold=%u wall=%.6fs\n",
				fi,f.layer,group,f.tokens,r.state,r.started,r.completed,r.stopped,r.phase,r.count,r.marker,r.pc[1],cold,seconds);
			if (passed) {
				double e=Error(request->output,f.expected.data()+group*256),p=Error(request->output,precise);
				passed=e<(synthetic?.00002:.02) && p<.00002;
				printf("  numerical relative-L2 captured=%.9g scalar-FP32=%.9g %s\n",e,p,passed?"PASS":"FAIL"); calls++;
			}
			if (!passed) fprintf(stderr,"STOP: no fallback/retry; preserve completion/cold diagnostics\n");
		}
	}
	bool clean=!active;
	if (active && !cold && !closeOnly) {
		session={}; status=ioctl(fd,TT_PCI_PROBE_PIONEER_SESSION_END,&session,sizeof(session));
		clean=!status && session.state==TT_RESIDENT_SESSION_CLOSED && session.pci_restored && session.host_released && !session.cold;
		printf("Attention END state=%u PCI/host=%u/%u cold=%u status=%d\n",session.state,session.pci_restored,session.host_released,session.cold,status);
	}
	close(fd); free(request);
	printf("%s: attention qualification calls=%u ioctl-wall=%.6fs total=%.6fs; NOT inference tokens/s or CPU speedup\n",passed && clean?"PASS":"STOP",calls,deviceWall,Seconds()-started);
	if (!clean) fprintf(stderr,"STOP: cold recovery/state inspection required; no retry/reset/reload\n");
	return passed && clean?0:1;
}
