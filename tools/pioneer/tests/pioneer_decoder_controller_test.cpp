/* SPDX-License-Identifier: MIT */
#define PIONEER_WIDE_CONTROLLER_MOCK_ONLY
#include "pioneer_wide_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderController.h"
struct DecoderControllerIO:WideControllerIO {
	std::vector<uint32_t> decoderBackup=std::vector<uint32_t>(8192);
	PioneerDecoderLease* decoder=nullptr;
	bool attentionLost=false,mutateSequence=false,mutateModel=false,peerMaps=false,peerMapsLost=false,badStage=false;
	unsigned badPacket=0;
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
		for (unsigned i=0;i<8192;i++) assert(layerProgram[i]==PioneerDecoderCodeWord(*decoder,i));
		auto& j=*reinterpret_cast<PioneerDecoderJob*>(data.data()+6144); assert(PioneerDecoderJobValid(j,decoder->paired?5:decoder->greedy?4:decoder->glu?3:decoder->parallel?2:1));
		for (unsigned w=1;w<120;w++) assert(fleet[w].Compute().running && !fleet[w].persistentIdle
			&& fleet[w].persistentSequence==j.sequence[w-1]+PioneerDecoderSequenceAdvance(w-1,decoder->parallel,decoder->glu,decoder->paired)-1);
		C().reset=value; starts++; if (executeTimeout) return;
		unsigned tokens=j.tokens,r=decoder->glu?47412:decoder->parallel?45732:43716+448*((tokens+63)>>6),rf=decoder->glu?102517:decoder->parallel?87733:77205+1792*tokens,
			w=decoder->glu?23821:decoder->parallel?22981:22309,wf=decoder->glu?48542:decoder->parallel?37286:33030;
		bool small=j.operation==9; if (small) { r-=118; rf-=9494; w-=118; wf-=9494; }
		if (decoder->paired) { r-=8176; rf-=6132; w-=4088; wf-=4088; }
		const unsigned delta[]={r,rf,r+w,r,w,wf,r+w,w};
		data[3072]=r; data[3073]=rf; data[3074]=w; data[3075]=wf; data[3076]=119;
		data[3077]=151936; data[3078]=j.nonce-(staleSummary?1:0); data[3079]=small?9:8;
		for (unsigned i=0;i<8;i++) { data[3080+i]=nc[i]; nc[i]+=delta[i]; data[3088+i]=nc[i]; }
		if (badNoc) nc[7]++; if (badSummary) data[3088]++;
		for (unsigned i=0;i<119;i++) j.sequence[i]+=PioneerDecoderSequenceAdvance(i,decoder->parallel,decoder->glu,decoder->paired);
		if (mutateSequence) j.sequence[118]++; if (mutateModel) j.layers[27].k+=4096;
		if (!omitWidePayload) {
			if (!small) for (unsigned i=0;i<151936;i++) wideData[i]=nonfinite && !i?0x7f800000:0x40000000;
			else { for (unsigned i=0;i<32;i++) wideData[i]=0;
				wideData[0]=TT_DECODER_GREEDY_MAGIC; wideData[1]=j.nonce; wideData[2]=151936; wideData[3]=111;
				wideData[4]=nonfinite?0x7f800000:0x40000000; wideData[5]=tokens; wideData[6]=119; wideData[7]=9;
				if (badPacket) { unsigned i=badPacket-1; wideData[i]=i==3?151936:i==4?0x7f800000:wideData[i]^1; }
			}
		}
		status[0]=TT_DECODER_DONE; status[1]=119+(badCount?1:0); status[2]=badMarker?TT_COORDINATOR_FAULT:TT_COORDINATOR_MARKER;
		if (decoder->profiling) {
			tt_decoder_stage_profile p={}; p.magic=0x31525044; p.nonce=j.nonce; p.tokens=tokens; p.version=decoder->paired?3:decoder->glu?2:1;
			const unsigned c[]={decoder->paired?168U:196U,113,decoder->glu?85U:113U,28,28,28,1,1};
			for (unsigned i=0;i<8;i++) { p.cycles[i]=i==7?100:i+1; p.calls[i]=c[i]; }
			if (badStage) p.reserved[11]=1;
			for (unsigned i=0;i<32;i++) data[(TT_DECODER_STAGE_L1-TT_BULK_UPLOAD_L1)/4+i]=reinterpret_cast<uint32_t*>(&p)[i];
		}
	}
};
struct DecoderControllerFixture:LayerFixtureBase<DecoderControllerIO> {
	PioneerDecoderLease decoder; PioneerWideOutputLease wide; PioneerAttentionTileLease attention; PioneerResidentAttentionLease kv;
	PioneerAttentionTileLease peers[8]; bool parallel,profiling,glu,sampling,paired,inlineMath,partialMath,cachedQuery;
	tt_decoder_candidate_result decoded={}; PioneerDecoderJob model={};
	DecoderControllerFixture(bool concurrent=false,bool timing=false,bool activation=false,bool greedy=false,bool together=false,bool direct=false,bool partial=false,bool cached=false):parallel(concurrent),profiling(timing),glu(activation),sampling(greedy),paired(together),inlineMath(direct),partialMath(partial),cachedQuery(cached) {
		io.decoder=&decoder; attention.ready=attention.saved=attention.resident=true; kv.reserved=kv.payloadBlocks=kv.coordinated=true;
		model.finalNorm=model.vocabulary=0x04000000;
		for (auto& l:model.layers) for (unsigned i=0;i<11;i++) reinterpret_cast<uint32_t*>(&l)[i]=0x04000000;
		for (unsigned w=1;w<120;w++) fleet[w].inlineMath=fleet[w].coordinatedInline=inlineMath;
		for (unsigned w=1;w<120;w++) fleet[w].partialMath=partialMath;
		if (parallel) { io.peerMaps=true; for (unsigned i=0;i<8;i++) { peers[i].saved=peers[i].ready=peers[i].code_dirty=peers[i].coordinatedGroup=true;
			peers[i].coordinatedGlu=glu; peers[i].cachedQuery=cachedQuery; fleet[i+1].attentionDispatch=true; fleet[i+1].gluDispatch=glu; } }
	}
	bool PrepareDecoder() { return PioneerPrepareWideOutput(io,host,transfer,wide)
		&& PioneerDecoderPrepare(io,io.core[0]->Baseline(),fleet.data(),coordinator,code,attention,decoder,parallel,parallel?peers:nullptr,profiling,glu,sampling,paired,inlineMath,partialMath,cachedQuery); }
	bool Decode(unsigned tokens=1,bool small=false) {
		uint32_t input[1024],rope[128]={}; for (auto& value:input) value=0x3f000000; for (unsigned i=0;i<64;i++) rope[2*i]=0x3f800000;
		std::vector<uint32_t> output(151936,0xabcdef00);
		bool okay=PioneerDecoderRun(io,io.core[0]->Baseline(),host,transfer,wide,fleet.data(),bulk,coordinator,decoder,attention,kv,
			model,tokens,input,rope,output.data(),decoded,small);
		if (okay) { assert(PioneerDecoderProof(decoded,tokens,coordinator.nonce,parallel,glu,small,paired) && !wide.pending && !transfer.pending && !decoder.running && !attention.running
				&& !coordinator.inputValid && !io.pioWords && io.C().pciCommand[0]==0x402 && io.C().pciCommand[1]==0x402);
			if (small) { assert(PioneerDecoderGreedyValid(output.data(),coordinator.nonce,tokens) && output[3]==111 && output[4]==0x40000000);
				for (unsigned i=32;i<output.size();i++) assert(output[i]==0xabcdef00); }
			else for (auto value:output) assert(value==0x40000000);
			for (unsigned i=0;i<28;i++) assert(kv.tokens[i]==tokens);
			if (parallel) for (auto& p:peers) assert(p.data_dirty && !p.running && !p.unsafe); }
		return okay;
	}
	bool CloseDecoder(bool stopped=true) { return PioneerDecoderClose(io,io.core[0]->Baseline(),fleet[0].noc,fleet[0].Store(),decoder,stopped); }
};
int main()
{
	(void)Run; (void)Input;
	{ DecoderControllerFixture f(true,true,true,true,true,true,false,true); assert(f.PrepareDecoder());
		for (unsigned n:{1U,2U,3U,1U}) assert(f.Decode(n,true) && f.Decode(n,false));
		f.cachedQuery=false; assert(!f.PrepareDecoder()); f.cachedQuery=true;
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	for (unsigned w=0;w<8;w++) { DecoderControllerFixture f(true,true,true,true,true,true,false,true); f.peers[w].cachedQuery=false;
		assert(!f.PrepareDecoder() && !f.decoder.saved && !f.io.starts); }
	{ DecoderControllerFixture f(true,true,true,true,true,true,true,true); assert(!f.PrepareDecoder() && !f.decoder.saved && !f.io.starts); }
	{ DecoderControllerFixture f(true,true,true,true,true,true,true); assert(f.PrepareDecoder());
		for (unsigned n:{1U,2U,3U,1U}) assert(f.Decode(n,true) && f.Decode(n,false));
		f.partialMath=false; assert(!f.PrepareDecoder()); f.partialMath=true;
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	for (unsigned w=1;w<120;w++) { DecoderControllerFixture f(true,true,true,true,true,true,true); f.fleet[w].partialMath=false;
		assert(!f.PrepareDecoder() && !f.decoder.saved && !f.io.starts); }
	{ DecoderControllerFixture f(true,true,true,true,true,false,true); assert(!f.PrepareDecoder() && !f.decoder.saved && !f.io.starts); }
	{ DecoderControllerFixture f(true,true,true,true,true,true); assert(f.PrepareDecoder());
		for (unsigned n:{1U,2U,3U,1U}) assert(f.Decode(n,true) && f.Decode(n,false));
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	for (unsigned w=1;w<120;w++) for (bool field:{false,true}) { DecoderControllerFixture f(true,true,true,true,true,true);
		if (field) f.fleet[w].coordinatedInline=false; else f.fleet[w].inlineMath=false;
		assert(!f.PrepareDecoder() && !f.decoder.saved && !f.io.starts); }
	{ DecoderControllerFixture f(true,true,true,true,true); assert(f.PrepareDecoder());
		for (unsigned n:{1U,2U,3U,1U}) { assert(f.Decode(n,true) && f.Decode(n,false) && f.Decode(n,true));
			assert(PioneerDecoderStageValid(f.decoder.stages,f.coordinator.nonce,n,3)); }
		for (unsigned word=0;word<sizeof(f.decoded)/4;word++) { auto r=f.decoded; reinterpret_cast<uint32_t*>(&r)[word]^=1;
			if (word!=10) assert(!PioneerDecoderProof(r,1,f.coordinator.nonce,true,true,true,true)); }
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	{ DecoderControllerFixture f(true,true,true,false,true); assert(!f.PrepareDecoder() && !f.decoder.saved && !f.io.starts); }
	{ DecoderControllerFixture f(true,true,true,true); assert(f.PrepareDecoder());
		for (unsigned n:{1U,2U,3U,1U}) { assert(f.Decode(n,true) && f.Decode(n,false) && f.Decode(n,true)); }
		for (unsigned word=0;word<sizeof(f.decoded)/4;word++) { auto r=f.decoded; reinterpret_cast<uint32_t*>(&r)[word]^=1;
			if (word!=10) assert(!PioneerDecoderProof(r,1,f.coordinator.nonce,true,true,true)); }
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	for (unsigned i=1;i<=32;i++) { DecoderControllerFixture f(true,true,true,true); assert(f.PrepareDecoder()); f.io.badPacket=i;
		assert(!f.Decode(1,true) && f.decoder.unsafe && f.wide.quarantined); for (unsigned n:f.kv.tokens) assert(!n);
		unsigned stops=f.io.stops; assert(!f.Decode(1,true) && !f.CloseDecoder() && f.io.stops==stops); }
	{ DecoderControllerFixture f(true,true,true); assert(f.PrepareDecoder()); for (unsigned n:{1U,2U,3U,1U}) { assert(f.Decode(n)); assert(PioneerDecoderStageValid(f.decoder.stages,f.coordinator.nonce,n,2)); }
		for (unsigned word=0;word<sizeof(f.decoded)/4;word++) { auto r=f.decoded; reinterpret_cast<uint32_t*>(&r)[word]^=1;
			if (word!=10) assert(!PioneerDecoderProof(r,1,f.coordinator.nonce,true,true)); }
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true)); }
	for (unsigned p=0;p<8;p++) for (bool lease:{false,true}) { DecoderControllerFixture f(true,true,true);
		if (lease) f.peers[p].coordinatedGlu=false; else f.fleet[p+1].gluDispatch=false;
		assert(!f.PrepareDecoder() && !f.io.starts && !f.decoder.saved); }
	for (bool bad:{false,true}) { DecoderControllerFixture f(true,true); assert(f.PrepareDecoder()); f.io.badStage=bad;
		for (unsigned n:{1U,2U,3U}) { assert(f.Decode(n)); assert(PioneerDecoderStageValid(f.decoder.stages,f.coordinator.nonce,n)==!bad); }
		assert(f.CloseDecoder() && PioneerReleaseWideOutput(f.io,f.wide,true));
	}
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
	for (unsigned mode:{0U,1U,2U,3U,4U,5U,6U}) for (unsigned fault=0;fault<16;fault++) {
		bool glu=mode>=1,small=mode>=2; DecoderControllerFixture f(true,glu,glu,small,mode>=3,mode>=4,mode==5,mode==6); assert(f.PrepareDecoder());
		f.io.executeTimeout=fault==0; f.io.badPc=fault==1; f.io.badCount=fault==2; f.io.badNoc=fault==3;
		f.io.badMarker=fault==4; f.io.stopFails=fault==5; f.io.staleSummary=fault==6; f.io.badSummary=fault==7;
		f.io.badLiveMap=fault==8; f.io.nonfinite=fault==9; f.io.omitWidePayload=fault==10; f.io.layerLost=fault==11;
		f.io.attentionLost=fault==12; f.io.mutateSequence=fault==13; f.io.mutateModel=fault==14; f.io.peerMapsLost=fault==15;
		assert(!f.Decode(1,small) && f.io.quarantined && f.io.wideQuarantined && f.decoder.unsafe && f.decoded.master.recovery_required);
		for (auto& p:f.peers) assert(p.unsafe);
		if (fault!=5 && fault!=9 && fault!=10) assert(!f.io.stops);
		unsigned resets=f.io.stops; assert(!f.Decode(1,small) && !f.CloseDecoder() && f.io.stops==resets);
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
