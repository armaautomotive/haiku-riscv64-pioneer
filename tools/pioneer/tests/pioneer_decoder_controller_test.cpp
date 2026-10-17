/* SPDX-License-Identifier: MIT */
#define PIONEER_WIDE_CONTROLLER_MOCK_ONLY
#include "pioneer_wide_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderController.h"
struct DecoderControllerIO:WideControllerIO {
	std::vector<uint32_t> decoderBackup=std::vector<uint32_t>(8192);
	PioneerDecoderLease* decoder=nullptr;
	bool attentionLost=false,mutateSequence=false,mutateModel=false,peerMaps=false,peerMapsLost=false;
	uint32_t* DecoderSavedWords() { return decoderBackup.data(); }
	bool DecoderMappingStable() { return LayerMappingStable(); }
	bool DecoderAttentionMappingStable() { return !(attentionLost && starts); }
	bool DecoderPeerMappingsStable() { return peerMaps && !(peerMapsLost && starts); }
	void RuntimeBufferWrite(unsigned w,uint32_t value) { assert(!current && C().reset==0x47800 && w>=4096 && w<5120); C().buffers[w]=value; }
	uint32_t RuntimeBufferRead(unsigned w) { assert(!current && C().reset==0x47800 && w>=4096 && w<5120); return C().buffers[w]; }
	void RuntimeResetWrite(uint32_t value) {
		if (value!=0x46800) {
			if (C().reset==0x46800 && value==0x87ffff) assert(!attentionLost && !mutateSequence && !mutateModel);
			WideControllerIO::RuntimeResetWrite(value); return;
		}
		assert(!current && !quarantined && !wideQuarantined && !wideDirty && C().reset==0x87ffff && C().code[0]==TT_DECODER_BOOT && decoder->running);
		for (unsigned i=0;i<8192;i++) assert(layerProgram[i]==(decoder->parallel?BlackholeParallelDecoderCodeWord(i):BlackholeDecoderCodeWord(i)));
		auto& j=*reinterpret_cast<PioneerDecoderJob*>(data.data()+6144); assert(PioneerDecoderJobValid(j,decoder->parallel?2:1));
		for (unsigned w=1;w<120;w++) assert(fleet[w].Compute().running && !fleet[w].persistentIdle
			&& fleet[w].persistentSequence==j.sequence[w-1]+PioneerDecoderSequenceAdvance(w-1,decoder->parallel)-1);
		C().reset=value; starts++; if (executeTimeout) return;
		unsigned tokens=j.tokens,r=decoder->parallel?45732:43716+448*((tokens+63)>>6),rf=decoder->parallel?87733:77205+1792*tokens,
			w=decoder->parallel?22981:22309,wf=decoder->parallel?37286:33030;
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		data[3072]=r; data[3073]=rf; data[3074]=w; data[3075]=wf; data[3076]=119;
		data[3077]=151936; data[3078]=j.nonce-(staleSummary?1:0); data[3079]=8;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++; if (badSummary) data[3088]++;
		for (unsigned i=0;i<119;i++) j.sequence[i]+=PioneerDecoderSequenceAdvance(i,decoder->parallel);
		if (mutateSequence) j.sequence[118]++; if (mutateModel) j.layers[27].k+=4096;
		if (!omitWidePayload) for (unsigned i=0;i<151936;i++) wideData[i]=nonfinite && !i?0x7f800000:0x40000000;
		status[0]=TT_DECODER_DONE; status[1]=119+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
	}
};
struct DecoderControllerFixture:LayerFixtureBase<DecoderControllerIO> {
	PioneerDecoderLease decoder; PioneerWideOutputLease wide; PioneerAttentionTileLease attention; PioneerResidentAttentionLease kv;
	PioneerAttentionTileLease peers[8]; bool parallel;
	tt_decoder_candidate_result decoded={}; PioneerDecoderJob model={};
	DecoderControllerFixture(bool concurrent=false):parallel(concurrent) {
		io.decoder=&decoder; attention.ready=attention.saved=attention.resident=true; kv.reserved=kv.payloadBlocks=kv.coordinated=true;
		model.finalNorm=model.vocabulary=0x04000000;
		for (auto& l:model.layers) for (unsigned i=0;i<11;i++) reinterpret_cast<uint32_t*>(&l)[i]=0x04000000;
		if (parallel) { io.peerMaps=true; for (unsigned i=0;i<8;i++) { peers[i].saved=peers[i].ready=peers[i].code_dirty=peers[i].coordinatedGroup=true; fleet[i+1].attentionDispatch=true; } }
	}
	bool PrepareDecoder() { return PioneerPrepareWideOutput(io,host,transfer,wide)
		&& PioneerDecoderPrepare(io,io.core[0]->Baseline(),fleet.data(),coordinator,code,attention,decoder,parallel,parallel?peers:nullptr); }
	bool Decode(unsigned tokens=1) {
		uint32_t input[1024],rope[128]={}; for (auto& value:input) value=0x3f000000; for (unsigned i=0;i<64;i++) rope[2*i]=0x3f800000;
		std::vector<uint32_t> output(151936,0xabcdef00);
		bool okay=PioneerDecoderRun(io,io.core[0]->Baseline(),host,transfer,wide,fleet.data(),bulk,coordinator,decoder,attention,kv,
			model,tokens,input,rope,output.data(),decoded);
		if (okay) { assert(PioneerDecoderProof(decoded,tokens,coordinator.nonce,parallel) && !wide.pending && !transfer.pending && !decoder.running && !attention.running
				&& !coordinator.inputValid && !io.pioWords && io.C().pciCommand[0]==0x402 && io.C().pciCommand[1]==0x402);
			for (auto value:output) assert(value==0x40000000); for (unsigned i=0;i<28;i++) assert(kv.tokens[i]==tokens);
			if (parallel) for (auto& p:peers) assert(p.data_dirty && !p.running && !p.unsafe); }
		return okay;
	}
	bool CloseDecoder(bool stopped=true) { return PioneerDecoderClose(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),decoder,stopped); }
};
int main()
{
	(void)Run; (void)Input;
	{ DecoderControllerFixture f(true); assert(f.PrepareDecoder()); for (unsigned n:{1U,2U,3U,1U}) assert(f.Decode(n));
		for (unsigned word=0;word<sizeof(f.decoded)/4;word++) { auto r=f.decoded; reinterpret_cast<uint32_t*>(&r)[word]^=1;
			if (word!=10) assert(!PioneerDecoderProof(r,1,f.coordinator.nonce,true)); }
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	for (unsigned i=0;i<8;i++) for (unsigned fault=0;fault<8;fault++) {
		DecoderControllerFixture f(true); auto& p=f.peers[i];
		if (!fault) p.saved=false; if (fault==1) p.ready=false; if (fault==2) p.code_dirty=false; if (fault==3) p.coordinatedGroup=false;
		if (fault==4) p.resident=true; if (fault==5) p.unsafe=true; if (fault==6) p.running=true; if (fault==7) f.fleet[i+1].attentionDispatch=false;
		assert(!f.PrepareDecoder() && !f.io.starts && !f.decoder.saved);
	}
	for (unsigned fault=0;fault<16;fault++) {
		DecoderControllerFixture f(true); assert(f.PrepareDecoder());
		f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitWidePayload=fault==10; f.io.layerLost=fault==11;
		f.io.attentionLost=fault==12; f.io.mutateSequence=fault==13; f.io.mutateModel=fault==14; f.io.peerMapsLost=fault==15;
		assert(!f.Decode() && f.io.quarantined && f.io.wideQuarantined && f.decoder.unsafe && f.decoded.master.recovery_required);
		for (auto& p:f.peers) assert(p.unsafe);
		if (fault!=5 && fault!=9 && fault!=10) assert(!f.io.stops);
		unsigned resets=f.io.stops; assert(!f.Decode() && !f.CloseDecoder() && f.io.stops==resets);
	}
	{ DecoderControllerFixture f; assert(f.PrepareDecoder()); for (unsigned n:{1U,2U,3U,1U}) assert(f.Decode(n));
		for (unsigned word=0;word<sizeof(f.decoded)/4;word++) { auto result=f.decoded; reinterpret_cast<uint32_t*>(&result)[word]^=1;
			if (word!=10) assert(!PioneerDecoderProof(result,1,f.coordinator.nonce)); }
		assert(f.io.starts==f.io.stops && !f.io.inputReads && f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	for (unsigned fault=0;fault<15;fault++) {
		DecoderControllerFixture f; assert(f.PrepareDecoder());
		f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitWidePayload=fault==10; f.io.layerLost=fault==11;
		f.io.attentionLost=fault==12; f.io.mutateSequence=fault==13; f.io.mutateModel=fault==14;
		assert(!f.Decode() && f.io.quarantined && f.io.wideQuarantined && f.transfer.quarantined && f.wide.quarantined
			&& f.decoder.unsafe && f.attention.unsafe && !f.coordinator.inputValid && f.decoded.master.recovery_required && !f.io.pioWords
			&& f.io.C().pciCommand[0]==0x402 && f.io.C().pciCommand[1]==0x402);
		if (fault!=5 && fault!=9 && fault!=10) assert(!f.io.stops);
		unsigned writes=f.io.layerWrites; assert(!f.CloseDecoder(false) && f.io.layerWrites==writes && !PioneerReleaseWideOutput(f.io,f.wide,false));
	}
	{ DecoderControllerFixture f; assert(f.PrepareDecoder()); assert(!f.Decode(2) && !f.io.starts && f.CloseDecoder()); }
	for (unsigned word:{1U,4094U,8192U}) { DecoderControllerFixture f; f.io.dropLayer=word;
		assert(!f.PrepareDecoder() && f.decoder.saved && !f.io.starts && f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	{ DecoderControllerFixture f; assert(f.PrepareDecoder()); f.io.layerRestoreFails=true; assert(!f.CloseDecoder() && f.decoder.unsafe && f.io.quarantined); }
	{ DecoderControllerFixture f; assert(f.PrepareDecoder()); f.io.badWidePage=255; assert(!f.Decode() && !f.io.starts && f.wide.quarantined && f.decoder.unsafe); }
	puts("PASS OFFLINE whole-decoder ownership controller,complete immutable metadata/all119 sequence proof,28-layer KV commit,all-owner uncertain-state quarantine without reset/retry/restore/free;NO driver exposure");
}
