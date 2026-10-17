/* SPDX-License-Identifier: MIT */
#include "TenstorrentHaikuResident.h"
int main(int argc,char** argv)
{
	bool coordinator=argc==2 && !strcmp(argv[1],"--coordinator-probe-and-restore");
	if (!coordinator && (argc!=2 || strcmp(argv[1],"--read-held-peers-and-restore"))) {
		fprintf(stderr,"Usage: %s --read-held-peers-and-restore|--coordinator-probe-and-restore\nFixed read-only peer-PC transport qualification, NOT an inference benchmark\n",argv[0]); return 2;
	}
	setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
	PioneerResidentDevice device;
	if (!device.Open(UINT64_C(0x80000000),false,true,true,true,true,true,true,true,1,120,true,true)) return 1;
	uint32_t weights[18432]={},x[512],scale[32],y[64]={};
	for (unsigned row=0;row<64;row++) for (unsigned b=0;b<32;b++) {
		weights[PioneerQ8NativeF32ScaleByte(1024,b,row)/4]=0x3f800000;
		for (unsigned i=0;i<32;i++) {
			uint8_t encoded=0;
			if (!PioneerInt8UnpackEncode(int((row*13+(b*32+i)*7)%31)-15,encoded)) return 1;
			uint64_t at=PioneerQ8NativeF32ValueByte(1024,b*32+i,row);
			weights[at/4]|=uint32_t(encoded)<<(8*(at&3));
		}
	}
	for (auto& v:x) v=0x40014001;
	for (auto& v:scale) v=0x3f800000;
	bool okay=true;
	for (unsigned at=0;at<73728 && okay;at+=16384) {
		unsigned pages=(73728-at)/4096; if (pages>4) pages=4;
		okay=device.UploadPages(at,weights+at/4,pages);
	}
	okay=okay && device.Q8Matvec(0,1024,64,x,scale,y);
	if (coordinator) {
		const unsigned peers[]={1,2,119,1};
		for (unsigned count:peers) if (okay) okay=device.CoordinatorProbe(count);
	} else for (unsigned worker=1;worker<120 && okay;worker++) okay=device.PeerPcProbe(worker);
	bool clean=device.Finish();
	printf("%s %s peer-PC read transport; cleanup%u; no peer writes/RISC releases in probe; NOT inference speed\n",okay && clean?"PASS":"FAIL",coordinator?"on-card coordinator":"fixed119",clean);
	return okay && clean?0:1;
}
