/* SPDX-License-Identifier: MIT */
// Explicit CPU-checked LOCAL matrix qualification. NOT model inference,
// performance mode,or an Assistant backend. No arbitrary file/code/addresses.
#include <Drivers.h>
#include <OS.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#include <sys/select.h>
#endif
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38PanelProbe.h"
#include "PioneerQwen38Panel.h"
#include "PioneerQwen38Gguf.h"
static const char* const ModelPath="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
static bool FullSha(FILE* file)
{
	// Hash the SAME open FD before ANY device open/BEGIN/start. The child
	// inherits it as stdin,not a separately reopened path. No shell/template
	// execution; exact trusted system utility/fixed arguments only.
	struct stat before={},after={};
	if (fstat(fileno(file),&before) || !S_ISREG(before.st_mode) || uint64_t(before.st_size)!=PioneerQwen38Gguf::FullBytes || fseeko(file,0,SEEK_SET)) return false;
	int channel[2]; if (pipe(channel)) return false;
	pid_t child=fork();
	if (!child) {
		if (dup2(fileno(file),0)<0 || dup2(channel[1],1)<0) _exit(126);
		close(channel[0]); close(channel[1]);
		char name[]="sha256sum"; char* argv[]={name,NULL}; execv("/bin/sha256sum",argv); _exit(127);
	}
	close(channel[1]);
	if (child<0) { close(channel[0]); return false; }
	char digest[96]={}; unsigned used=0; bool overflow=false;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	uint64_t reported=0;
#endif
	for (;;) {
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
		fd_set ready; FD_ZERO(&ready); FD_SET(channel[0],&ready); timeval interval={5,0};
		int available=select(channel[0]+1,&ready,NULL,NULL,&interval);
		if (available<0 && errno==EINTR) continue;
		if (!available) {
			off_t at=lseek(fileno(file),0,SEEK_CUR);
			if (at>=0 && uint64_t(at)>=reported+UINT64_C(1073741824)) {
				reported=uint64_t(at); printf("MODEL_SHA_READ_OFFSET=%llu/%llu;NOT_DIGEST_ACCEPTANCE\n",(unsigned long long)reported,(unsigned long long)PioneerQwen38Gguf::FullBytes);
			}
			continue;
		}
		if (available<0) overflow=true;
#endif
		char data[96]; ssize_t count=read(channel[0],data,sizeof(data));
		if (count<0 && errno==EINTR) continue;
		if (count<=0) { if (count<0) overflow=true; break; }
		if (unsigned(count)>sizeof(digest)-1-used) overflow=true;
		else { memcpy(digest+used,data,size_t(count)); used+=unsigned(count); }
	}
	close(channel[0]); int status=0; pid_t done;
	do { done=waitpid(child,&status,0); } while (done<0 && errno==EINTR);
	return done==child && WIFEXITED(status) && !WEXITSTATUS(status) && !overflow && used==68
		&& !strcmp(digest,"c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747  -\n")
		&& !fstat(fileno(file),&after) && before.st_size==after.st_size && before.st_ino==after.st_ino
		&& before.st_mtime==after.st_mtime && before.st_ctime==after.st_ctime;
}
static bool Call(int fd,uint32_t operation,void* data,size_t bytes)
{
	if (!ioctl(fd,operation,data,bytes)) return true;
	fprintf(stderr,"STOP:op=%08x errno=%d %s;NO_RETRY\n",operation,errno,strerror(errno)); return false;
}
static bool Execute(int fd,unsigned type,unsigned panel,const uint8_t* tile,unsigned trial)
{
	tt_q38_panel_probe request={}; request.version=TT_Q38_PANEL_ABI_VERSION; request.type=type; request.bytes=PioneerQ38PanelTileBytes(type); request.panel=panel;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	request.activation_scale=PioneerAttentionBits(trial?-.03125f:.012345f);
#endif
	memcpy(request.tile,tile,request.bytes);
	for (unsigned k=0;k<16;k++) request.activation[k/2]|=uint32_t(PioneerInt8Operand(int((k*29+trial*17)%256)-128))<<((k%2)*16);
	PioneerQ38Panel expected={};
	if (!PioneerQ38UnpackPanel(type,tile,request.bytes,panel,&expected)) return false;
	if (!Call(fd,TT_PCI_PROBE_Q38_PANEL_RUN,&request,sizeof(request))) return false;
	const auto& r=request.result;
	bool okay=r.version==1 && r.state==TT_RUNTIME_PASSED && r.prepared && r.started && r.completed && r.stopped
		&& !r.recovery_required && r.context_reset && r.phase==TT_Q38_PANEL_DONE && r.panels==1 && r.marker==0x4d415431
		&& r.pc[0]==TT_Q38_PANEL_PARK && r.pc[1]==r.pc[0];
	for (unsigned i=0;i<8;i++) okay &= r.before[i]==r.after[i];
	unsigned mismatches=0;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	int inputSum=0;
	for (unsigned k=0;k<16;k++) {
		uint16_t input=uint16_t(request.activation[k/2]>>((k%2)*16)); inputSum+=int(input&1023)*((input&0x8000)?-1:1);
	}
#endif
	for (unsigned row=0;row<16;row++) {
		int expectedDot=0;
		for (unsigned k=0;k<16;k++) {
			unsigned element=k*16+row; uint16_t weight=uint16_t(expected.operand[element/2]>>((element%2)*16));
			uint16_t input=uint16_t(request.activation[k/2]>>((k%2)*16));
			expectedDot+=int(weight&1023)*((weight&0x8000)?-1:1)*int(input&1023)*((input&0x8000)?-1:1);
		}
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
		float scale=PioneerAttentionFloat(request.activation_scale);
		float product=(PioneerAttentionFloat(expected.coefficient[row])*scale)*float(expectedDot);
		float correction=(PioneerAttentionFloat(expected.minimum[row])*scale)*float(inputSum);
		mismatches+=request.dot[row]!=PioneerAttentionBits(product+correction) || request.coefficient[row]!=expected.coefficient[row] || request.minimum[row]!=expected.minimum[row];
#else
		mismatches+=int32_t(request.dot[row])!=expectedDot || request.coefficient[row]!=expected.coefficient[row] || request.minimum[row]!=expected.minimum[row];
#endif
	}
	printf("LOCAL_PANEL type=%u panel=%u trial=%u state=%u start/complete/stop=%u/%u/%u cold=%u pc=%x/%x exact_errors=%u\n",
		type,panel,trial,r.state,r.started,r.completed,r.stopped,r.recovery_required,r.pc[0],r.pc[1],mismatches); fflush(stdout);
	return okay && !mismatches;
}
int main(int argc,char** argv)
{
	if (argc!=2 || (strcmp(argv[1],"--support") && strcmp(argv[1],"--synthetic") && strcmp(argv[1],"--model"))) {
		fprintf(stderr,"Usage:%s --support|--synthetic|--model;DEBUG_LOCAL_16x16_DOTS_ONLY_NOT_INFERENCE\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IOLBF,0); bool useModel=!strcmp(argv[1],"--model");
	FILE* file=NULL; PioneerQwen38Gguf model;
	if (useModel) {
		file=fopen(ModelPath,"rb");
		if (!file || !FullSha(file) || !model.Open(file)) {
			if (file) fclose(file);
			fprintf(stderr,"STOP:FULL_SAME_FD_SHA_LAYOUT_REQUIRED_BEFORE_DEVICE_OPEN\n"); return 1;
		}
		puts("FULL_SAME_FD_SHA_AND_851_LAYOUTS_PASS;LOCAL_PANEL_TEST_ONLY");
	}
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
	if (fd<0) { if (file) fclose(file); perror("open P100A"); return 1; }
	tt_q38_panel_support support={};
	bool okay=Call(fd,TT_PCI_PROBE_Q38_PANEL_SUPPORT,&support,sizeof(support)) && support.driver==TT_Q38_PANEL_CURRENT_DRIVER
		&& support.version==TT_Q38_PANEL_ABI_VERSION && support.local_only==1 && support.rows==16 && support.columns==16 && support.type_mask==0x5100;
	if (!okay || !strcmp(argv[1],"--support")) { close(fd); if (file) fclose(file); return okay?0:1; }
	tt_q38_bank_session session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) && session.result.state==TT_RESIDENT_SESSION_READY
		&& session.result.active && session.result.host_ready && !session.result.cold && !session.result.capabilities && session.result.bytes==PioneerResidentBanks::Total;
	unsigned cases=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned end=0;end<2 && okay;end++) {
		uint8_t tile[3392]={}; unsigned block=PioneerQ38BlockBytes(type);
		if (useModel) {
			const char* name=type==8?"blk.0.attn_qkv.weight":type==12?"token_embd.weight":"output.weight";
			const auto* tensor=model.Find(name); if (!tensor || tensor->type!=type) { okay=false; break; }
			unsigned index=unsigned(tensor-model.Tensors().data()),rowBytes=unsigned(tensor->bytes/tensor->outputs),first=end?unsigned(tensor->outputs)-16:0;
			for (unsigned row=0;row<16 && okay;row++) {
				uint8_t raw[210]={}; okay=model.ReadPacked(index,uint64_t(first+row)*rowBytes+(end?rowBytes-block:0),block,raw,block);
				for (unsigned b=0;b<block;b++) tile[b*16+row]=raw[b];
			}
		} else for (unsigned row=0;row<16;row++) {
			for (unsigned b=0;b<block;b++) tile[b*16+row]=uint8_t(row*17+b*29+end*59);
			unsigned d=type==14?208:0; uint16_t half=row%3?uint16_t(0x3c00+row*13):uint16_t(1+row);
			tile[d*16+row]=uint8_t(half); tile[(d+1)*16+row]=uint8_t(half>>8);
			if (type==12) { tile[32+row]=0; tile[48+row]=0x38; }
			if (type==8) tile[32+row]=128;
		}
		for (unsigned trial=0;trial<2 && okay;trial++) {
			if (useModel && !model.Stable()) { okay=false; break; }
			okay=Execute(fd,type,end?(type==8?1:15):0,tile,trial); if (okay) cases++;
		}
	}
	if (file) { okay &= model.Stable(); fclose(file); }
	if (!okay) { fprintf(stderr,"STOP:LOCAL_PROBE_FAILURE;CLOSE_ONLY_NO_END_RETRY_OR_RESET\n"); close(fd); return 1; }
	session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) && session.result.state==TT_RESIDENT_SESSION_CLOSED
		&& !session.result.active && session.result.pci_restored && session.result.host_released && !session.result.cold;
	if (close(fd)) okay=false;
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
	printf("%s_LOCAL_SCALED_%s_PANELS cases=%u;KNOWN_CLEAN_END=%u;NOT_FULL_MATVEC_OR_INFERENCE_OR_SPEED\n",okay?"PASS":"FAIL",useModel?"MODEL":"SYNTHETIC",cases,unsigned(okay));
#else
	printf("%s_LOCAL_%s_PANELS cases=%u;KNOWN_CLEAN_END=%u;NOT_SCALED_MATVEC_OR_INFERENCE_OR_SPEED\n",okay?"PASS":"FAIL",useModel?"MODEL":"SYNTHETIC",cases,unsigned(okay));
#endif
	return okay && cases==12?0:1;
}
