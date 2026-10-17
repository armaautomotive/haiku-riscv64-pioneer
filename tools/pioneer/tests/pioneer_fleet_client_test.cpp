/* SPDX-License-Identifier: MIT */
// REAL native client with synthetic syscalls only. No card/model access.
#include <cassert>
#include <cstring>
#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
static unsigned calls=0,ends=0,closes=0,fault=0,target=0,field=0,missing=0;
static bool failed=false;
static unsigned coordinatedNonce=0;
static unsigned coordinatedContract=7;
static unsigned decoderTokens=0,decoderPositions=1;
static bool batchEarly=false;
static int MockOpen(const char*,int) { return 91; }
static int MockClose(int fd) { assert(fd==91); closes++; return 0; }
static int MockIoctl(int fd,unsigned long op,void* data,size_t bytes) {
	assert(fd==91 && !failed); calls++;
	if (op==TT_PCI_PROBE_PIONEER_SESSION_SUPPORT) {
		auto& s=*(tt_resident_session_result*)data; s.version=1; s.capabilities=TT_PIONEER_RESIDENT_CAPABILITIES&~missing; return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_COORDINATED_SUPPORT) {
		assert(bytes==sizeof(tt_coordinated_support)); auto& s=*static_cast<tt_coordinated_support*>(data);
		s={coordinatedContract,119,3,1,8192}; return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_SESSION_BEGIN) { auto& s=*(tt_resident_session_result*)data; s.state=TT_RESIDENT_SESSION_READY; s.active=s.host_ready=1; return 0; }
	if (op==TT_PCI_PROBE_PIONEER_SESSION_END) { auto& s=*(tt_resident_session_result*)data; s.state=TT_RESIDENT_SESSION_CLOSED; s.pci_restored=s.host_released=1; ends++; return 0; }
	if (op==TT_PCI_PROBE_PIONEER_DECODER_STAGES) {
		assert(coordinatedContract>=15 && coordinatedContract<=33 && bytes==sizeof(tt_decoder_stage_profile)); auto& p=*static_cast<tt_decoder_stage_profile*>(data); p={};
		p.magic=0x31525044; p.nonce=coordinatedNonce; p.tokens=decoderTokens; p.version=coordinatedContract==33?5:coordinatedContract==31?4:coordinatedContract>=20?3:coordinatedContract>=16?2:1;
		const unsigned c[]={coordinatedContract>=20?168U:196U,coordinatedContract>=31?57U:113U,coordinatedContract>=16?85U:113U,coordinatedContract>=31?0U:28U,28,28,1,1}; for (unsigned i=0;i<8;i++) { p.cycles[i]=!c[i]?0:i==7?100:i+1; p.calls[i]=c[i]*(coordinatedContract==33 && i!=7?decoderPositions:1); }
		if (coordinatedContract==33) p.reserved[0]=decoderPositions;
		if (fault && target==3) { if (field>=4 && field<12) reinterpret_cast<uint32_t*>(&p)[field]=reinterpret_cast<uint32_t*>(&p)[field]?0:1;
			else if (field==2) p.tokens++; else reinterpret_cast<uint32_t*>(&p)[field]^=1; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_DECODER_MODEL) {
		auto& p=*static_cast<tt_pioneer_decoder_model*>(data); assert(bytes==sizeof(p) && p.version==(coordinatedContract==33?17U:coordinatedContract==31?16U:coordinatedContract==30?15U:coordinatedContract==29?14U:coordinatedContract==28?13U:coordinatedContract==27?12U:coordinatedContract==26?11U:coordinatedContract==25?10U:(coordinatedContract==24 || coordinatedContract==27)?9U:coordinatedContract==23?8U:coordinatedContract==22?7U:coordinatedContract==21?6U:coordinatedContract==20?5U:coordinatedContract>=17?4U:coordinatedContract==16?3U:coordinatedContract>=14?2U:1U) && (coordinatedContract==33?(p.reserved[0]==0x30000000 && p.reserved[1]==151643 && p.reserved[2]==5):(!p.reserved[0] && !p.reserved[1] && !p.reserved[2])));
		p.state=TT_RUNTIME_PASSED; p.ready=1; p.extents=coordinatedContract==33?311:310; p.recovery_required=0;
		if (fault && target==1) { failed=true; if (fault==1) { errno=EIO; return -1; } if (fault==2) p.ready=0; if (fault==3) p.extents=309; if (fault==4) p.recovery_required=1; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_DECODER_RUN) {
		auto& p=*static_cast<tt_pioneer_decoder_run*>(data); assert(bytes==sizeof(p) && p.version==1 && p.tokens && p.tokens<=256 && !p.reserved[0] && !p.reserved[1]);
		decoderTokens=p.tokens; decoderPositions=1;
		auto& r=p.result; r={}; r.tokens=p.tokens; r.sequences=119; r.words=151936;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=m.count=119; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_DECODER_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_DECODER_PARK;
		bool parallel=coordinatedContract>=14;
		bool glu=coordinatedContract>=16;
		unsigned reads=glu?47412:parallel?45732:43716+448*((p.tokens+63)>>6),rf=glu?102517:parallel?87733:77205+1792*p.tokens,w=glu?23821:parallel?22981:22309,wf=glu?48542:parallel?37286:33030;
		if (coordinatedContract>=20) { reads-=8176; rf-=6132; w-=4088; wf-=4088; }
		if (coordinatedContract==24 || coordinatedContract==26 || coordinatedContract>=27) { reads-=784; rf-=16128; w-=784; wf-=16128; }
		if (coordinatedContract==26) { reads+=112; rf+=112; }
		if (coordinatedContract>=28) { reads-=9219; rf-=9219; w-=9219; wf-=9219; }
		if (coordinatedContract==30) { reads+=15372; rf+=10248; w+=5124; wf+=5124; }
		if (coordinatedContract==33) { reads+=2; rf+=16; }
		const unsigned delta[]={reads,rf,reads+w,reads,w,wf,reads+w,w};
		r.summary[0]=reads; r.summary[1]=rf; r.summary[2]=w; r.summary[3]=wf; r.summary[4]=119; r.summary[5]=151936; r.summary[6]=m.nonce; r.summary[7]=8;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		for (unsigned i=0;i<151936;i++) p.output[i]=0x3f800000+i;
		if (fault && !target) { failed=true; if (fault==1) { errno=EIO; return -1; } if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=1; if (fault==4) p.output[field]=0x7f800000; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_DECODER_GREEDY) {
		auto& p=*static_cast<tt_pioneer_decoder_greedy*>(data); assert(coordinatedContract>=17 && coordinatedContract<=33 && bytes==sizeof(p) && p.version==1 && p.tokens && p.tokens<=256 && !p.reserved[0] && !p.reserved[1]);
		decoderTokens=p.tokens; decoderPositions=1; auto& r=p.result; r={}; r.tokens=p.tokens; r.sequences=119; r.words=32;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=m.count=119; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_DECODER_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_DECODER_PARK;
		bool paired=coordinatedContract>=20;
		const unsigned reads=coordinatedContract==33?29117:coordinatedContract==30?44487:coordinatedContract>=28?29115:(coordinatedContract==24 || coordinatedContract==27)?38334:paired?39118:47294,rf=coordinatedContract==33?61560:coordinatedContract==30?71792:coordinatedContract>=28?61544:(coordinatedContract==24 || coordinatedContract==27)?70763:paired?86891:93023,w=coordinatedContract==30?14736:coordinatedContract>=28?9612:(coordinatedContract==24 || coordinatedContract==27)?18831:paired?19615:23703,wf=coordinatedContract==30?14737:coordinatedContract>=28?9613:(coordinatedContract==24 || coordinatedContract==27)?18832:paired?34960:39048,delta[]={reads,rf,reads+w,reads,w,wf,reads+w,w};
		r.summary[0]=reads; r.summary[1]=rf; r.summary[2]=w; r.summary[3]=wf; r.summary[4]=119; r.summary[5]=151936; r.summary[6]=m.nonce; r.summary[7]=9;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		memset(p.output,0,sizeof(p.output)); p.output[0]=TT_DECODER_GREEDY_MAGIC; p.output[1]=m.nonce; p.output[2]=151936;
		p.output[3]=111; p.output[4]=0x3f123456; p.output[5]=p.tokens; p.output[6]=119; p.output[7]=9;
		if (fault && !target) { failed=true; if (fault==1) { errno=EIO; return -1; } if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=1;
			if (fault==4) p.output[field]=field==3?151936:field==4?0x7fc00000:p.output[field]^1; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_DECODER_BATCH) {
		auto& p=*static_cast<tt_pioneer_decoder_batch*>(data); assert(coordinatedContract==33 && bytes==sizeof(p) && p.version==1 && p.reserved[0] && p.reserved[0]<=2 && !p.reserved[1]);
		for (auto value:p.rope) assert(!value);
		unsigned count=batchEarly?1:p.reserved[0]; decoderTokens=p.tokens+count-1; decoderPositions=count;
		auto& r=p.result; r={}; r.tokens=p.tokens; r.sequences=119; r.words=64;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=m.count=119; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_DECODER_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_DECODER_PARK;
		unsigned reads=29116*count+64*(count-1)+1,rf=61558*count+576*(count-1)+4,w=9611*count+1,wf=9611*count+4;
		const unsigned delta[]={reads,rf,reads+w,reads,w,wf,reads+w,w};
		r.summary[0]=reads; r.summary[1]=rf; r.summary[2]=w; r.summary[3]=wf; r.summary[4]=119; r.summary[5]=151936; r.summary[6]=m.nonce; r.summary[7]=10;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		memset(p.output,0,sizeof(p.output)); p.output[0]=TT_DECODER_BATCH_MAGIC; p.output[1]=m.nonce; p.output[2]=151936;
		p.output[3]=count; p.output[4]=p.tokens; p.output[5]=decoderTokens; p.output[6]=119; p.output[7]=10;
		for (unsigned i=0;i<count;i++) { p.output[8+i]=batchEarly?151645:111+i; p.output[24+i]=0x3f123456; }
		if (fault && !target) { failed=true; if (fault==1) { errno=EIO; return -1; } if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=1;
			if (fault==4) p.output[field]=field>=8 && field<8+count?151936:field>=24 && field<24+count?0x7fc00000:p.output[field]^1; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_LAYER_SPAN) {
		auto& p=*static_cast<tt_pioneer_layer_span*>(data); assert(p.version==1 && !p.reserved[0] && !p.reserved[1] && bytes==PioneerSpanRequestBytes(p.bytes));
		auto& r=p.result; r={}; r.bytes=p.bytes; r.pages=p.bytes/4096; r.matched_words=p.bytes/4; r.host_exact=1;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=1; m.count=r.pages; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_LAYER_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_LAYER_PARK;
		unsigned chunks=(p.bytes+16383)/16384,reads=4*chunks,rf=p.bytes/16,writes=3*chunks,wf=3*(p.bytes/64);
		const unsigned delta[]={reads,rf,reads+writes,reads,writes,wf,reads+writes,writes};
		r.summary[0]=reads; r.summary[1]=rf; r.summary[2]=writes; r.summary[3]=wf;
		r.summary[4]=r.pages; r.summary[5]=p.bytes; r.summary[6]=m.nonce; r.summary[7]=7;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		if (fault) { failed=true; if (fault==1) { errno=EIO; return -1; } if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=1; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_LAYER_WIDE || op==TT_PCI_PROBE_PIONEER_LAYER_NARROW) {
		bool narrow=op==TT_PCI_PROBE_PIONEER_LAYER_NARROW;
		auto& p=*static_cast<tt_pioneer_coordinated_fleet*>(data);
		assert(p.version==1 && bytes==PioneerCoordinatedRequestBytes(p.outputs)); PioneerFleetPlan plan; assert(PioneerFleetPartition(p.inputs,p.outputs,p.workers,plan));
		auto& r=p.result; r={}; r.inputs=p.inputs; r.outputs=p.outputs; r.workers=plan.workers;
		r.staged_pages=(p.inputs*2+p.inputs/8+4095)/4096; r.gathered=r.dma_words=p.outputs;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=m.count=plan.workers; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_LAYER_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_LAYER_PARK;
		unsigned reads=narrow?4*plan.workers+1:5*plan.workers,flits=narrow?reads-plan.workers+p.outputs/8-1:reads-2*plan.workers+p.outputs/8;
		unsigned writes=narrow?2*plan.workers+1:3*plan.workers,wf=2*plan.workers+p.outputs/16;
		const unsigned delta[]={reads,flits,reads+writes,reads,writes,wf,reads+writes,writes};
		r.summary[0]=reads; r.summary[1]=flits; r.summary[2]=writes; r.summary[3]=wf;
		r.summary[4]=plan.workers; r.summary[5]=p.outputs; r.summary[6]=m.nonce; r.summary[7]=narrow?3:6;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		for (unsigned i=0;i<p.outputs;i++) p.output[i]=0x3f800000+i;
		if (fault) { failed=true; if (fault==1) { errno=EIO; return -1; } if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=field==offsetof(tt_coordinated_fleet_result,staged_pages)/4?8:1; if (fault==4) p.output[field]=0x7f800000; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_LAYER_GLU) {
		auto& p=*static_cast<tt_pioneer_layer_glu*>(data); unsigned n=p.outputs;
		assert(bytes==sizeof(p) && p.version==1 && n && n<=3072 && !(n&31) && !p.reserved[0] && !p.reserved[1]);
		auto& r=p.result; r={}; r.outputs=n; r.workers=1; r.dma_words=r.gathered=n;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=m.count=1; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_LAYER_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_LAYER_PARK;
		const unsigned delta[]={1,n/16,2,1,1,n/16,2,1};
		r.summary[0]=r.summary[2]=r.summary[4]=1; r.summary[1]=r.summary[3]=n/16;
		r.summary[5]=n; r.summary[6]=m.nonce; r.summary[7]=5;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		for (unsigned i=0;i<n;i++) p.output[i]=0x3f800000+i;
		if (fault) { failed=true; if (fault==1) { errno=EIO; return -1; } if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=1; if (fault==4) p.output[field]=0x7f800000; }
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_LAYER_FFN) {
		auto& p=*static_cast<tt_pioneer_layer_ffn*>(data);
		assert(bytes==sizeof(p) && p.version==1 && p.workers && p.workers<=119 && !p.reserved[0] && !p.reserved[1]
			&& p.gate_offset==0 && p.up_offset==0x800000 && p.down_offset==0x1000000);
		auto& r=p.result; r={}; unsigned pg=std::min(p.workers,96U),pd=std::min(p.workers,32U),peers=2*pg+pd;
		r.inputs=r.outputs=1024; r.workers=pg; r.staged_pages=1; r.dma_words=r.gathered=1024;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=m.count=pg; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_LAYER_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_LAYER_PARK;
		unsigned reads=4*peers+1,flits=reads-peers+511,writes=2*peers+1,writeFlits=2*peers+64;
		const unsigned delta[]={reads,flits,reads+writes,reads,writes,writeFlits,reads+writes,writes};
		r.summary[0]=reads; r.summary[1]=flits; r.summary[2]=writes; r.summary[3]=writeFlits;
		r.summary[4]=pg; r.summary[5]=1024; r.summary[6]=m.nonce; r.summary[7]=4;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		for (unsigned i=0;i<1024;i++) p.output[i]=0x3f800000+i;
		if (fault) {
			failed=true; if (fault==1) { errno=EIO; return -1; }
			if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=1;
			if (fault==4) p.output[field]=0x7f800000;
		}
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_COORDINATED_FLEET) {
		auto& p=*(tt_pioneer_coordinated_fleet*)data; assert((p.version==1 || p.version==2) && p.workers<=119 && bytes==PioneerCoordinatedRequestBytes(p.outputs));
		PioneerFleetPlan plan; assert(PioneerFleetPartition(p.inputs,p.outputs,p.workers,plan));
		auto& r=p.result; r={}; r.inputs=p.inputs; r.outputs=p.outputs; r.workers=plan.workers;
		bool fused=p.version==1 && p.outputs<=4096;
		r.staged_pages=p.version==2?4:(p.inputs*2+p.inputs/8+4095)/4096; r.gathered=p.outputs<=8192?p.outputs:0; r.dma_words=((p.version==2 && p.outputs<=8192) || fused)?p.outputs:0;
		auto& m=r.master; m.version=1; m.state=TT_RUNTIME_PASSED; m.workers=m.matched=m.count=plan.workers; m.nonce=++coordinatedNonce;
		m.prepared=m.started=m.completed=m.stopped=m.polls=m.summary_verified=1;
		m.phase=TT_COORDINATOR_DONE; m.marker=TT_COORDINATOR_MARKER; m.pc[0]=m.pc[1]=TT_COORDINATOR_PARK;
		unsigned reads=(p.outputs<=8192?11:10)*plan.workers+(fused?1:0),flits=p.outputs<=8192?reads-plan.workers+p.outputs/16+(fused?p.outputs/16-1:0):reads;
		unsigned writes=2*plan.workers+(fused?1:0),writeFlits=2*plan.workers+(fused?p.outputs/16:0);
		const unsigned delta[]={reads,flits,reads+writes,reads,writes,writeFlits,reads+writes,writes};
		r.summary[0]=reads; r.summary[1]=flits; r.summary[2]=writes; r.summary[3]=writeFlits; r.summary[4]=plan.workers;
		r.summary[5]=p.outputs; r.summary[6]=m.nonce; r.summary[7]=fused?3:1;
		for (unsigned i=0;i<8;i++) { m.before[i]=r.summary[8+i]=UINT32_MAX-7; m.after[i]=r.summary[16+i]=m.before[i]+delta[i]; }
		for (unsigned i=0;i<p.outputs;i++) p.output[i]=0x3f800000+i;
		if (fault) {
			failed=true;
			if (fault==1) { errno=EIO; return -1; }
			if (fault==2) reinterpret_cast<uint32_t*>(&r)[field]^=field==offsetof(tt_coordinated_fleet_result,staged_pages)/4?4:1;
			if (fault==4) p.output[field]=0x7fc00000;
		}
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_RUNTIME_Q8_FLEET) {
		auto& p=*(tt_pioneer_runtime_fleet*)data; assert(p.version==2 && bytes==PioneerFleetRequestBytes(p.outputs));
		PioneerFleetPlan plan; assert(PioneerFleetPartition(p.inputs,p.outputs,p.workers,plan));
		auto& r=p.result; r={}; r.version=1; r.state=TT_RUNTIME_PASSED; r.workers=r.started=r.completed=r.stopped=plan.workers;
		r.staged_pages=(p.inputs*2+4095)/4096+1;
		for (unsigned w=0;w<plan.workers;w++) {
			auto& c=r.core[w]; c.version=1; c.state=1; c.prepared=c.context_reset=c.started=c.completed=c.stopped=1;
			c.phase=TT_RUNTIME_Q8_FINAL_PHASE; c.tiles=(p.inputs/16)*(plan.rows[w]/16); c.marker=0x4d415431; c.pc[0]=c.pc[1]=TT_RUNTIME_Q8_PARK_PC;
			for (unsigned i=0;i<4;i++) { c.before[i]=0xfffffffe; c.after[i]=c.before[i]+(i==1?c.tiles/32*144+256*(w!=0):c.tiles/32+(w!=0)); }
		}
		for (unsigned row=0;row<p.outputs;row++) p.output[row]=0x3f800000+row;
		if (fault) {
			failed=true;
			if (fault==1) { errno=EIO; return -1; }
			if (fault==2) ((uint32_t*)&r.core[target])[field]^=1;
			if (fault==3) ((uint32_t*)&r)[field]^=1;
		}
		return 0;
	}
	if (op==TT_PCI_PROBE_PIONEER_RESIDENT_ATTENTION) {
		auto& p=*(tt_pioneer_resident_attention*)data;
		assert((p.version==1 || p.version==2) && bytes==sizeof(p) && PioneerResidentAttentionInput(p.tokens,p.layer,p.input));
		for (unsigned w=0;w<2048;w++) assert(p.input[w]==0x3f800000);
		for (unsigned g=0;g<8;g++) for (unsigned w=0;w<64;w++) {
			assert(p.input[2048+g*128+w]==0x38003800 && p.input[2048+g*128+64+w]==0x3c003c00);
		}
		auto& r=p.result.math; r={}; r.version=1; r.state=TT_ATTN_TILE_PASSED;
		r.prepared=r.context_reset=r.started=r.completed=r.stopped=r.polls=1;
		r.tokens=r.count=p.tokens; r.staged_words=3072; r.phase=TT_ATTENTION_TILE_DONE; r.marker=TT_ATTENTION_TILE_MARKER;
		r.pc[0]=r.pc[1]=TT_ATTENTION_TILE_PARK; p.result.layer=p.layer; p.result.groups=8;
		unsigned reads=16*((p.tokens+63)/64); const unsigned expected[8]={reads,64*p.tokens,reads+16,reads,16,64,reads+16,16};
		for (unsigned w=0;w<8;w++) { p.result.before[w]=UINT32_MAX-7; p.result.after[w]=p.result.before[w]+expected[w]; }
		for (unsigned w=0;w<4;w++) { r.before[w]=p.result.before[w]; r.after[w]=p.result.after[w]; }
		for (auto& w:p.output) w=0x3f800000;
		if (fault) {
			failed=true;
			if (fault==1) { errno=EIO; return -1; }
			if (fault==2) ((uint32_t*)&p.result)[field]^=1;
			if (fault==4) p.output[field]=0x7fc00000;
		}
		return 0;
	}
	assert(op==TT_PCI_PROBE_PIONEER_FLEET_ATTENTION);
	auto& p=*(tt_pioneer_fleet_attention*)data; assert(p.version==1 && bytes==PioneerFleetAttentionBytes(p.tokens));
	for (unsigned g=0;g<8;g++) {
		for (unsigned i=0;i<256;i++) assert(p.query[g*256+i]==0x3f800000);
		for (unsigned i=0;i<p.tokens*64;i++) { assert(p.prefix[g*p.tokens*128+i]==0x38003800); assert(p.prefix[g*p.tokens*128+p.tokens*64+i]==0x3c003c00); }
		auto& r=p.result[g]; r={}; r.version=1; r.state=TT_ATTN_TILE_PASSED; r.prepared=r.context_reset=r.started=r.completed=r.stopped=1;
		r.tokens=r.count=p.tokens; r.staged_words=256+p.tokens*128; r.phase=TT_ATTENTION_TILE_DONE; r.marker=TT_ATTENTION_TILE_MARKER;
		r.pc[0]=r.pc[1]=TT_ATTENTION_TILE_PARK;
		for (unsigned i=0;i<4;i++) r.before[i]=r.after[i]=0xfffffffe;
	}
	for (auto& word:p.output) word=0x3f800000;
	if (fault) { failed=true; if (fault==1) { errno=EIO; return -1; } ((uint32_t*)&p.result[target])[field]^=1; }
	return 0;
}
#define open MockOpen
#define close MockClose
#define ioctl MockIoctl
#include "../TenstorrentHaikuResident.h"
#undef ioctl
#undef close
#undef open
static void Reset(unsigned f=0,unsigned t=0,unsigned word=0) { coordinatedNonce=calls=ends=closes=missing=0; decoderPositions=1; batchEarly=false; failed=false; fault=f; target=t; field=word; }
static bool Open(PioneerResidentDevice& d) { return d.Open(2*1024*1024,false,true,true,true,true,true,true,true,1,120); }
static bool RegisterTestDecoder(PioneerResidentDevice& d,const PioneerDecoderModelOffsets& model) {
	return coordinatedContract==33?d.RegisterDecoder(model,0x30000000,151643,5):d.RegisterDecoder(model);
}
int main() {
	uint32_t x[512],scale[32]; for (auto& v:x) v=0x40014001; for (auto& v:scale) v=0x3c800000;
	std::vector<uint32_t> output(8192,0xdeadbeef);
	for (unsigned w=0;w<120;w++) for (unsigned f:{0U,1U,2U}) for (unsigned word:{0U,1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U}) {
		if ((f<2 && (w || word)) || (!f && w)) continue;
		Reset(f,w,word); std::fill(output.begin(),output.end(),0xdeadbeef);
		{ PioneerResidentDevice d; assert(Open(d)); bool okay=d.Q8Matvec(0,1024,8192,x,scale,output.data()); assert(okay==!f);
			if (!f) { assert(d.peakWorkers==120); for (unsigned row=0;row<8192;row++) assert(output[row]==0x3f800000+row); assert(d.Finish()); }
			else { unsigned previous=calls; assert(!d.Q8Matvec(0,1024,8192,x,scale,output.data()) && !d.Finish() && calls==previous); for (auto v:output) assert(v==0xdeadbeef); }
		}
		assert(closes==1 && ends==unsigned(!f));
	}
	for (unsigned word=0;word<8;word++) {
		Reset(3,0,word); PioneerResidentDevice d; assert(Open(d)); assert(!d.Q8Matvec(0,1024,8192,x,scale,output.data())); assert(!d.Finish() && !ends);
	}
	for (unsigned n:{1U,2U,255U,256U}) for (unsigned g=0;g<8;g++) for (unsigned word:{0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U,24U}) {
		Reset(2,g,word); uint32_t query[2048]; for (auto& v:query) v=0x3f800000;
		std::vector<uint32_t> keys(n*512,0x38003800),values(n*512,0x3c003c00),result(2048,0xdeadbeef);
		{ PioneerResidentDevice d; assert(Open(d)); assert(!d.AttentionGroups(n,query,keys.data(),values.data(),result.data()));
			unsigned previous=calls; assert(!d.Finish() && calls==previous); for (auto v:result) assert(v==0xdeadbeef); }
		assert(!ends && closes==1);
	}
	for (unsigned n:{1U,256U}) {
		Reset(); uint32_t query[2048]; for (auto& v:query) v=0x3f800000;
		std::vector<uint32_t> keys(n*512,0x38003800),values(n*512,0x3c003c00),result(2048,0xdeadbeef);
		{ PioneerResidentDevice d; assert(Open(d) && d.AttentionGroups(n,query,keys.data(),values.data(),result.data()));
			assert(d.attentionCalls==8); for (auto v:result) assert(v==0x3f800000); assert(d.Finish()); }
		assert(ends==1 && closes==1);
	}
	for (unsigned absent:{TT_PIONEER_CAP_Q8_FLEET,TT_PIONEER_CAP_FLEET_ATTENTION}) {
		Reset(); missing=absent; { PioneerResidentDevice d; assert(!Open(d)); } assert(calls==1 && closes==1 && !ends);
	}
	for (bool blocks:{false,true}) for (unsigned n:{1U,65U,256U}) for (unsigned layer:{0U,27U}) for (unsigned word=0;word<43;word++) {
		Reset(2,0,word); uint32_t query[2048],keys[512],values[512],result[2048];
		for (auto& w:query) w=0x3f800000;
		for (auto& w:keys) w=0x38003800;
		for (auto& w:values) w=0x3c003c00;
		for (auto& w:result) w=0xdeadbeef;
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,120,true,blocks));
			assert(!d.ResidentAttentionStep(n,layer,query,keys,values,result));
			for (auto w:result) assert(w==0xdeadbeef);
			unsigned previous=calls; assert(!d.Finish() && calls==previous);
		}
		assert(!ends && closes==1);
	}
	for (unsigned absent:{TT_PIONEER_CAP_RESIDENT_ATTENTION,TT_PIONEER_CAP_BULK_UPLOAD}) {
		Reset(); missing=absent;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,2,120,true)); }
		assert(calls==1 && !ends && closes==1);
	}
	for (unsigned absent:{TT_PIONEER_CAP_PAYLOAD_BLOCKS,TT_PIONEER_CAP_WIDE_PAYLOAD}) {
		Reset(); missing=absent;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,120,true,true)); }
		assert(calls==1 && !ends && closes==1);
	}
	{
		Reset(); missing=TT_PIONEER_CAP_COORDINATED_FLEET;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true)); }
		assert(calls==1 && closes==1 && !ends);
	}
	for (bool debug:{false,true}) for (unsigned n:{64U,1024U,2048U,3072U,4096U,6144U,8192U,151936U}) {
		Reset(); output.assign(n,0xdeadbeef);
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,debug));
			assert(d.Q8Matvec(0,1024,n,x,scale,output.data()) && d.Q8Matvec(0,1024,n,x,scale,output.data()));
			for (unsigned i=0;i<n;i++) assert(output[i]==0x3f800000+i);
			assert(d.Finish()); }
		assert(ends==1 && closes==1);
	}
	for (unsigned contract:{0U,2U,3U,4U,5U,6U,34U}) {
		Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true)); }
		assert(calls==2 && closes==1 && !ends);
	}
	for (unsigned contract:{7U,8U,9U}) {
		Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,3)); }
		assert(calls==2 && closes==1 && !ends);
	}
	coordinatedContract=10;
	for (unsigned f:{0U,1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_coordinated_fleet_result)/4:1);word++) {
		Reset(f,0,word); output.assign(6144,0xdeadbeef);
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,3));
			bool okay=d.Q8WideMatvec(0,1024,6144,x,scale,output.data()); assert(okay==!f);
			if (!f) {
				for (unsigned i=0;i<6144;i++) assert(output[i]==0x3f800000+i);
				assert(d.Q8Matvec(0,1024,6144,x,scale,output.data()) && d.LayerFfn(0,0x800000,0x1000000,x,scale,output.data()) && d.Q8Matvec(0,1024,32,x,scale,output.data()) && d.Finish());
			} else {
				for (auto value:output) assert(value==0xdeadbeef);
				unsigned before=calls; assert(!d.Q8WideMatvec(0,1024,6144,x,scale,output.data()) && !d.Finish() && calls==before);
			}
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	coordinatedContract=11;
	for (unsigned f:{0U,1U,2U}) for (unsigned word=0;word<(f==2?sizeof(tt_layer_span_result)/4:1);word++) {
		Reset(f,0,word);
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,3));
			assert(d.UploadPageLimit()==256); std::vector<uint32_t> source(262144,0x7fc00001);
			assert(d.UploadPages(0,source.data(),256)==!f);
			if (!f) {
				assert(d.uploads==256 && d.spanCalls==1 && d.UploadPages(0x100000,source.data(),1));
				assert(d.Q8WideMatvec(0,1024,6144,x,scale,output.data()) && d.LayerFfn(0,0x800000,0x1000000,x,scale,output.data()) && d.Finish());
			} else { unsigned before=calls; assert(!d.UploadPages(0,source.data(),256) && !d.Finish() && calls==before && !d.uploads); }
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	coordinatedContract=12;
	for (unsigned f:{0U,1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_coordinated_fleet_result)/4:1);word++) {
		Reset(f,0,word); output.assign(6144,0xdeadbeef);
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,3));
			assert(d.NarrowEnabled()); bool okay=d.Q8Matvec(0,1024,2048,x,scale,output.data()); assert(okay==!f);
			if (!f) {
				for (unsigned i=0;i<2048;i++) assert(output[i]==0x3f800000+i);
				assert(d.narrowCalls==1 && !d.wideCalls && d.Q8Matvec(0,1024,6144,x,scale,output.data()));
				std::vector<uint32_t> source(1024,0x7fc00001);
				assert(d.UploadPages(0x200000,source.data(),1) && d.LayerFfn(0,0x800000,0x1000000,x,scale,output.data())
					&& d.Q8Matvec(0,1024,32,x,scale,output.data()) && d.Finish());
				assert(d.narrowCalls==2 && d.wideCalls==1 && d.spanCalls==1);
			} else { for (auto value:output) assert(value==0xdeadbeef); unsigned before=calls;
				assert(!d.Q8Matvec(0,1024,32,x,scale,output.data()) && !d.Finish() && calls==before); }
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	for (unsigned contract:{7U,8U,9U,10U,11U,12U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,4)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,15U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,5)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,16U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,6)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,17U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,7)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,18U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,8)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,19U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,9)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,20U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,10)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,21U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,11)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,22U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,12)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,23U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,13)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,24U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,14)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U,25U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,15)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U,24U,26U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,16)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U,24U,25U,27U}) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,17)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract=7;contract<=28;contract++) if (contract!=27) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,18)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract=7;contract<=29;contract++) if (contract!=28) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,19)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract=7;contract<=30;contract++) if (contract!=29) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,20)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract=7;contract<=31;contract++) if (contract!=30) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,21)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract=7;contract<=34;contract++) if (contract!=33) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,23)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned f:{0U,1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_decoder_candidate_result)/4:f==4?64:1);word++) {
		if (f==2 && word==10) continue;
		Reset(f,0,word); coordinatedContract=33; uint32_t packet[64],embedding[1024]={}; for (auto& value:packet) value=0xabcdef01;
		{ PioneerResidentDevice d; PioneerDecoderModelOffsets model={};
			assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,23) && RegisterTestDecoder(d,model) && d.BatchEnabled());
			unsigned before=calls; assert(!d.DecodeBatch(1,0,embedding,packet) && !d.DecodeBatch(256,2,embedding,packet) && !d.DecodeBatch(1,3,embedding,packet) && calls==before);
			assert(d.DecodeBatch(1,2,embedding,packet)==!f);
			if (!f) { assert(packet[3]==2 && packet[5]==2 && d.batchCalls==1 && d.greedyCalls==2 && d.calls==394 && d.attentionCalls==56);
				tt_decoder_stage_profile profile={}; assert(d.DecoderStages(profile) && profile.version==5 && profile.reserved[0]==2);
				assert(d.DecodeBatch(3,1,embedding,packet) && packet[3]==1 && d.calls==591 && d.Finish()); }
			else { for (auto value:packet) assert(value==0xabcdef01); before=calls; assert(!d.DecodeBatch(1,2,embedding,packet) && !d.Finish() && calls==before); }
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	{ Reset(); coordinatedContract=33; batchEarly=true; uint32_t packet[64],embedding[1024]={}; PioneerResidentDevice d; PioneerDecoderModelOffsets model={};
		assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,23) && RegisterTestDecoder(d,model)
			&& d.DecodeBatch(1,2,embedding,packet) && packet[3]==1 && packet[8]==151645 && packet[5]==1 && d.calls==197 && d.Finish()); }
	for (unsigned contract=7;contract<=33;contract++) if (contract!=31) { Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,22)); }
		assert(calls==2 && closes==1 && !ends); }
	for (unsigned contract:{13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U,28U,29U,30U,31U,33U}) { coordinatedContract=contract; unsigned requirement=(contract==33?23:contract-9);
	for (unsigned f:{1U,2U,3U,4U}) { Reset(f,1); { PioneerResidentDevice d; PioneerDecoderModelOffsets model={};
		assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,requirement) && !RegisterTestDecoder(d,model));
		unsigned before=calls; assert(!RegisterTestDecoder(d,model) && !d.Finish() && calls==before); } assert(!ends && closes==1); }
	for (unsigned f:{0U,1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_decoder_candidate_result)/4:1);word++) {
		Reset(f,0,word); output.assign(151936,0xdeadbeef); uint32_t embedding[1024]={},rope[128]={};
		{ PioneerResidentDevice d; PioneerDecoderModelOffsets model={};
			assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,requirement) && RegisterTestDecoder(d,model));
			assert(!d.UploadPageLimit() && !d.UploadPages(0,x,1));
			assert(d.DecodeWhole(1,embedding,rope,output.data())==!f);
			if (!f) { for (unsigned i=0;i<151936;i++) assert(output[i]==0x3f800000+i);
				if (contract>=15) { tt_decoder_stage_profile profile={}; assert(d.DecoderStages(profile) && profile.tokens==1 && profile.nonce==coordinatedNonce); }
				assert(d.DecodeWhole(2,embedding,rope,output.data()) && d.calls==394 && d.attentionCalls==56 && d.wideCalls==2 && d.Finish()); }
			else { for (auto value:output) assert(value==0xdeadbeef); unsigned before=calls;
				assert(!d.DecodeWhole(2,embedding,rope,output.data()) && !d.Finish() && calls==before); }
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	}
	for (unsigned contract:{17U,18U,19U,20U,21U,22U,23U,24U,25U,27U,28U,29U,30U,31U,33U}) { coordinatedContract=contract;
	for (unsigned f:{0U,1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_decoder_candidate_result)/4:f==4?32:1);word++) {
		if (f==2 && word==10) continue;
		Reset(f,0,word); uint32_t token=0xdeadbeef,logit=0xabcdef01,embedding[1024]={},rope[128]={};
		{ PioneerResidentDevice d; PioneerDecoderModelOffsets model={};
			assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,(contract==33?23:contract-9)) && RegisterTestDecoder(d,model));
			assert(d.DecodeGreedy(1,embedding,rope,token,logit)==!f);
			if (!f) { assert(token==111 && logit==0x3f123456 && d.greedyCalls==1 && !d.wideCalls);
				assert(d.DecodeGreedy(2,embedding,rope,token,logit) && d.calls==394 && d.attentionCalls==56 && d.greedyCalls==2 && d.Finish()); }
			else { assert(token==0xdeadbeef && logit==0xabcdef01); unsigned before=calls;
				assert(!d.DecodeGreedy(2,embedding,rope,token,logit) && !d.Finish() && calls==before); }
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	}
	{ Reset(); coordinatedContract=26; PioneerResidentDevice d; PioneerDecoderModelOffsets model={}; uint32_t embedding[1024]={},rope[128]={},token=0xdeadbeef,logit=0xabcdef01;
		assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,17) && RegisterTestDecoder(d,model));
		unsigned before=calls; assert(!d.DecodeGreedy(1,embedding,rope,token,logit) && calls==before && token==0xdeadbeef && logit==0xabcdef01 && d.Finish()); }
	for (unsigned contract:{15U,16U,17U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U,28U,29U,30U,31U,33U}) { Reset(); coordinatedContract=contract; PioneerResidentDevice d; PioneerDecoderModelOffsets model={}; uint32_t embedding[1024]={},rope[128]={}; output.assign(151936,0);
		assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,(contract==33?23:contract-9)) && RegisterTestDecoder(d,model)
			&& d.DecodeWhole(1,embedding,rope,output.data()));
		for (field=0;field<32;field++) { fault=2; target=3; tt_decoder_stage_profile p={}; assert(!d.DecoderStages(p)); }
		fault=target=0; assert(d.DecodeWhole(2,embedding,rope,output.data()) && d.Finish());
	}
	for (unsigned contract:{7U,8U}) {
		Reset(); coordinatedContract=contract;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,2)); }
		assert(calls==2 && closes==1 && !ends);
	}
	coordinatedContract=9;
	for (unsigned f:{0U,1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_coordinated_fleet_result)/4:1);word++) {
		Reset(f,0,word); output.assign(3072,0xdeadbeef);
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,2));
			std::vector<uint32_t> gate(3072,0x3f800000),up(3072,0x40000000);
			bool okay=d.LayerGlu(3072,gate.data(),up.data(),output.data()); assert(okay==!f);
			if (!f) { for (unsigned i=0;i<3072;i++) assert(output[i]==0x3f800000+i); assert(d.Finish()); }
			else { for (auto value:output) assert(value==0xdeadbeef); unsigned before=calls; assert(!d.LayerGlu(3072,gate.data(),up.data(),output.data()) && !d.Finish() && calls==before); }
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	{
		Reset(); coordinatedContract=7;
		{ PioneerResidentDevice d; assert(!d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,true)); }
		assert(calls==2 && closes==1 && !ends);
	}
	coordinatedContract=8;
	for (unsigned f:{0U,1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_coordinated_fleet_result)/4:1);word++) {
		Reset(f,0,word); output.assign(1024,0xdeadbeef);
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true,false,true));
			assert(d.HasLayerFfn()); bool okay=d.LayerFfn(0,0x800000,0x1000000,x,scale,output.data()); assert(okay==!f);
			if (!f) {
				for (unsigned i=0;i<1024;i++) assert(output[i]==0x3f800000+i);
				assert(d.Q8Matvec(0,1024,1024,x,scale,output.data()) && d.LayerFfn(0,0x800000,0x1000000,x,scale,output.data()) && d.Finish());
			} else {
				for (auto value:output) assert(value==0xdeadbeef);
				unsigned previous=calls; assert(!d.LayerFfn(0,0x800000,0x1000000,x,scale,output.data()) && !d.Finish() && calls==previous);
			}
		}
		assert(ends==unsigned(!f) && closes==1);
	}
	coordinatedContract=7;
	for (unsigned f:{1U,2U,4U}) for (unsigned word=0;word<(f==2?sizeof(tt_coordinated_fleet_result)/4:1);word++) {
		Reset(f,0,word); output.assign(64,0xdeadbeef);
		{ PioneerResidentDevice d; assert(d.Open(0x80000000,false,true,true,true,true,true,true,true,1,119,true,true,true));
			assert(!d.Q8Matvec(0,1024,64,x,scale,output.data()));
			for (auto value:output) assert(value==0xdeadbeef);
			unsigned previous=calls; assert(!d.Q8Matvec(0,1024,64,x,scale,output.data()) && !d.Finish() && calls==previous); }
		assert(!ends && closes==1);
	}
	puts("PASS REAL fleet client, variable inline ABI,120 worker proofs/all8 attention groups,wrapped counters,no partial outputs,close-only/no retry on faults,capability refusal BEFORE BEGIN; NO hardware");
}
