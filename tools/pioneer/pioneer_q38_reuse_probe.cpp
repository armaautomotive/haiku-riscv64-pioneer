/* SPDX-License-Identifier: MIT */
// DEBUG ONLY:preload all public row cases BEFORE any RUN,then alternate
// weight formats WITHOUT another upload. No model/Assistant/CPU fallback.
#define PIONEER_Q38_DISPATCH_CLIENT
#define main UnusedOrdinaryDispatchClientMain
#include "pioneer_q38_cohort_probe.cpp"
#undef main
struct ReuseCase {
	std::unique_ptr<Q38Probe> request{new Q38Probe{}};
	std::vector<float> expected=std::vector<float>(2*256,0);
};
static bool PrepareReuseCase(int fd,unsigned type,unsigned ordinal,unsigned firstBank,
	const std::vector<uint8_t>& rows,const unsigned* offsets,ReuseCase& c)
{
	auto& r=*c.request; r.version=1; r.type=type; r.inputs=5120; r.peers=2;
	unsigned blocks=r.inputs/PioneerQ38CohortBlockInputs(type),bytes=blocks*PioneerQ38PanelTileBytes(type);
	for (unsigned k=0;k<r.inputs;k++) r.activation[k/2]|=uint32_t(PioneerInt8Operand(int((k*29+17)%256)-128))<<((k%2)*16);
	for (unsigned b=0;b<blocks;b++) r.scales[b]=PioneerAttentionBits(b%3?-.03125f:.012345f);
	for (unsigned w=0;w<2;w++) {
		unsigned at=offsets[ordinal+w]; if (at>rows.size() || bytes>rows.size()-at) return false;
		std::vector<uint8_t> packed(rows.begin()+at,rows.begin()+at+bytes),upload(64+bytes,0);
		r.part[w]={firstBank+w,1,64};
		if (!Reference(r,packed,w,c.expected.data()+w*256)) return false;
		memcpy(upload.data()+64,packed.data(),bytes);
		if (!Upload(fd,firstBank+w,0,upload)) return false;
	}
	return PioneerQ38DispatchValid(r);
}
#ifndef PIONEER_Q38_REUSE_MAIN
#define PIONEER_Q38_REUSE_MAIN main
#endif
int PIONEER_Q38_REUSE_MAIN(int argc,char** argv)
{
	if (argc!=2 || strcmp(argv[1],"--public-mixed-inputs")) return 2;
	setvbuf(stdout,nullptr,_IOLBF,0);
	std::vector<uint8_t> rows(PIONEER_Q38_ROW_FIXTURE_BYTES);
	if (!PioneerQ38LoadRowFixtures(rows.data(),unsigned(rows.size()))) return 1;
	puts("PUBLIC_MIXED_ROWS_MEMORY_SHA_PASS;DEBUG_CPU_REFERENCE_ONLY;NO_ACCELERATOR_FD_YET");
	int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC); if (fd<0) return 1;
	tt_q38_dispatch_support support={}; tt_q38_bank_support storage={};
	bool okay=Call(fd,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,&support,sizeof(support))
		&& support.driver==TT_Q38_DISPATCH_DRIVER && support.version==1 && support.peers==119 && support.rows==256
		&& support.max_inputs==17408 && support.banks==7 && support.offset_alignment==64 && support.persistent==1 && !support.model_inference
		&& Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&storage,sizeof(storage)) && storage.driver==45 && storage.version==1
		&& storage.banks==7 && storage.page_bytes==4096 && storage.max_pages==4 && storage.storage_only==1
		&& storage.bytes_per_bank==PioneerResidentBanks::Capacity;
	if (!okay) { close(fd); return 1; }
	tt_q38_bank_session session={}; session.version=1;
	okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) && session.result.version==1
		&& session.result.state==TT_RESIDENT_SESSION_READY && session.result.active==1 && session.result.host_ready==1
		&& !session.result.cold && !session.result.capabilities && session.result.bytes==PioneerResidentBanks::Total;
	const unsigned types[]={8,8,12,12,14,14,12,12},widths[]={5120,5120,5120,5120,5120,5120,17408,17408};
	unsigned offsets[8],at=0;
	for (unsigned i=0;i<8;i++) { offsets[i]=at; at+=(widths[i]/PioneerQ38CohortBlockInputs(types[i]))*PioneerQ38PanelTileBytes(types[i]); }
	ReuseCase cases[3];
	okay=okay && at==rows.size() && PrepareReuseCase(fd,12,2,0,rows,offsets,cases[0])
		&& PrepareReuseCase(fd,14,4,2,rows,offsets,cases[1]) && PrepareReuseCase(fd,8,0,4,rows,offsets,cases[2]);
	PioneerQ38DispatchHistory history; unsigned completed=0;
	for (unsigned which:{0U,1U,0U,1U,2U,2U,1U,0U,1U,1U,0U,0U}) {
		if (!okay) break;
		auto& c=cases[which]; auto& r=*c.request; double start=Seconds();
		okay=start>=0 && Call(fd,TT_PCI_PROBE_Q38_DISPATCH_RUN,&r,sizeof(r)) && history.Accept(r);
		if (!okay) break; // HOST copies ONLY after failure;NO END/retry/card read
		double finish=Seconds(); unsigned errors=ReturnedNumericErrors(r,c.expected,stderr);
		printf("REUSE_MIXED_DEBUG type%u K5120 peers2 nonce%u exact_errors%u checked_call_s%.6f;NOT_MODEL_OR_TOKEN_RATE\n",
			r.type,r.result.nonce,errors,finish-start);
		okay=!errors && finish>=start;
		if (okay) completed++;
	}
	if (!okay) { fputs("STOP:MIXED_REUSE_FAILURE;CLOSE_ONLY_NO_END_RETRY_RESET\n",stderr); close(fd); return 1; }
	session={}; session.version=1;
	okay=completed==12 && Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) && session.result.version==1
		&& session.result.state==TT_RESIDENT_SESSION_CLOSED && !session.result.active
		&& session.result.pci_restored==1 && session.result.host_released==1 && !session.result.cold;
	if (close(fd)) okay=false;
	printf("%s_MIXED_REUSE_DEBUG cases3 runs%u KNOWN_CLEAN_END%u;ALL_UPLOADS_BEFORE_FIRST_RUN;NOT_MODEL_OR_TOKEN_RATE\n",
		okay?"PASS":"FAIL",completed,unsigned(okay));
	return okay?0:1;
}
