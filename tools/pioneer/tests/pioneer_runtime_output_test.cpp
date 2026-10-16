/* SPDX-License-Identifier: MIT */
#include <initializer_list>
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeOutput.h"
struct OutputMock:StoreMock {
	uint32_t values[4096]={};
	unsigned pioReads=0,outputPages=0,writePage=0,visibilityReads=0;
	bool held=true,omitPayload=false,omitWord=false,badPadding=false,lateMutation=false,delayVisibility=false;
	bool omitReadback=false,corruptReadback=false,corruptFiniteWrite=false;
	unsigned faultWord=7;
	OutputMock() { for (auto& value:values) value=0x7fc00000; }
	uint32_t ResetRead() { return held?0x47800:0x46800; }
	void HostPageWrite(unsigned w,uint32_t value) { assert(!uncertain); PageMock::HostPageWrite(w,value); }
	uint32_t ResidentL1Read(unsigned w) { pioReads++; return StoreMock::ResidentL1Read(w); }
	uint32_t RuntimeBufferRead(unsigned w) { assert(w>=4096 && w<8192 && held && !uncertain); pioReads++; return values[w-4096]; }
	uint32_t HostPageRead(unsigned w) {
		if (delayVisibility && outputPages && ++visibilityReads==2049)
			for (unsigned i=0;i<1024;i++) page[i]=values[writePage*1024+i];
		return PageMock::HostPageRead(w);
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		if (offset!=0x40 || (command[7]!=0x212 && command[3]!=0x17f000)) {
			StoreMock::ResidentNocWrite(offset,value); return;
		}
		assert(held && !pending && !uncertain && value==1 && pciCommand[0]==0x406 && pciCommand[1]==0x406);
		assert(command[6]==0 && command[8]==4096 && command[9]==0);
		bool write=command[7]==0x212;
		if (write) {
			assert(command[0]>=0x84000 && command[0]<=0x87000 && command[0]%4096==0 && command[1]==0 && command[2]==(1|(2<<6)));
			assert(command[3]==uint32_t(physical) && command[4]==uint32_t(physical>>32) && command[5]==(19|(24<<6)));
		} else {
			assert(command[0]==uint32_t(physical) && command[1]==uint32_t(physical>>32) && command[2]==(19|(24<<6)));
			assert(command[4]==0 && command[5]==(1|(2<<6)) && command[7]==0);
		}
		doorbells++; doorbellRead=false;
		if (++Mock::writes==Mock::drop) { uncertain=true; return; }
		requests++; pending=true;
		if (failRequest==doorbells) { uncertain=true; return; }
		if (write) {
			outputPages++; writePage=(command[0]-0x84000)/4096; visibilityReads=0;
			if (!omitPayload && !delayVisibility) for (unsigned w=0;w<1024;w++)
				if (!omitWord || w!=7) page[w]=values[writePage*1024+w];
			if (badPadding) page[1023]=0x40000000;
			if (corruptFiniteWrite) page[faultWord]^=1;
			writeCounters[0]++; writeCounters[1]+=64; accepted++; writeCounters[3]++;
		} else {
			if (!omitReadback) for (unsigned w=0;w<1024;w++) destination[1024+w]=page[w];
			if (corruptReadback) destination[1024+faultWord]^=1;
			if (lateMutation) page[5]^=1;
			counters[0]++; counters[1]+=64; accepted++; counters[3]++;
		}
		pending=false; done=true;
	}
};
static void SetValues(OutputMock& io,unsigned count) {
	for (unsigned w=0;w<count;w++) io.values[w]=0x3f000000+w*17;
}
static bool RunOutput(OutputMock& io,const tt_host_page_result& h,PioneerResidentLease& l,unsigned words,
	uint32_t* output,tt_resident_stream_result& r) {
	uint32_t snapshot[1024]={};
	return PioneerRuntimeOutputPage(io,io.Baseline(),io.Compute(),h,l,0,words,snapshot,output,r);
}
int main() {
	for (unsigned count:{16U,32U,1024U,2048U,4096U}) {
		OutputMock io; tt_host_page_result h; PioneerResidentLease l; tt_resident_stream_result r;
		uint32_t output[4096]={},snapshot[1024]={}; SetValues(io,count);
		assert(Prepared(io,h,l)); unsigned reads=io.pioReads;
		for (unsigned p=0;p<(count+1023)/1024;p++) {
			unsigned words=count-p*1024<1024?count-p*1024:1024;
			assert(PioneerRuntimeOutputPage(io,io.Baseline(),io.Compute(),h,l,p,words,snapshot,output+p*1024,r));
			assert(r.requests==3 && r.completed==3 && r.host_visible && r.readback_complete && !r.mismatches && !r.recovery_required);
		}
		assert(io.pioReads==reads+2048*((count+1023)/1024) && !memcmp(output,io.values,count*4));
		assert(PioneerResidentLeaseEnd(io,io.Compute(),l) && io.ReleaseHostPage()==0); io.Safe();
	}
	unsigned writes=0;
	{
		OutputMock io; tt_host_page_result h; PioneerResidentLease l; tt_resident_stream_result r; uint32_t output[32];
		SetValues(io,32); assert(Prepared(io,h,l) && RunOutput(io,h,l,32,output,r)); writes=io.Mock::writes;
	}
	for (unsigned drop=1;drop<=writes;drop++) {
		OutputMock io; tt_host_page_result h; PioneerResidentLease l; tt_resident_stream_result r; uint32_t output[32]={};
		SetValues(io,32); assert(Prepared(io,h,l)); if (drop<=io.Mock::writes) continue; io.Mock::drop=drop;
		bool okay=RunOutput(io,h,l,32,output,r);
		if (okay) assert(!memcmp(output,io.values,sizeof(output)));
		else for (unsigned v:output) assert(v==0);
		if (r.recovery_required) assert(io.quarantined && !PioneerResidentLeaseEnd(io,io.Compute(),l));
		else { assert(PioneerResidentLeaseEnd(io,io.Compute(),l) && io.ReleaseHostPage()==0); io.Safe(); }
	}
	for (unsigned drop=1;drop<=1024;drop++) {
		OutputMock io; tt_host_page_result h; PioneerResidentLease l; tt_resident_stream_result r; uint32_t output[32]={};
		SetValues(io,32); assert(Prepared(io,h,l)); io.PageMock::drop=io.PageMock::writes+drop;
		assert(!RunOutput(io,h,l,32,output,r) && !r.host_write_issued && !r.recovery_required && r.mismatches);
		assert(PioneerResidentLeaseEnd(io,io.Compute(),l) && io.ReleaseHostPage()==0); io.Safe();
	}
	for (unsigned fault=0;fault<17;fault++) {
		OutputMock io; tt_host_page_result h; PioneerResidentLease l; tt_resident_stream_result r; uint32_t output[32]={};
		SetValues(io,32); assert(Prepared(io,h,l));
		io.held=fault!=0; io.failRequest=fault>=1 && fault<=3?fault:0;
		io.omitPayload=fault==4; io.omitWord=fault==5; io.badPadding=fault==6; io.lateMutation=fault==7;
		io.changed=fault==8; io.lateMapChange=fault==9; io.clearFails=fault==10; io.badFlits=fault==11;
		io.pciePending=fault==12; if (fault==13) io.values[0]=0xffffffff;
		io.omitReadback=fault==14; io.corruptReadback=fault==15; io.corruptFiniteWrite=fault==16;
		assert(!RunOutput(io,h,l,32,output,r) && r.recovery_required && io.quarantined);
		assert(!PioneerResidentLeaseEnd(io,io.Compute(),l) && !io.releases);
		for (unsigned v:output) assert(v==0);
		if (fault==4 || fault==5 || fault==6 || fault==13) assert(r.host_write_issued && !r.host_visible && !r.readback_complete);
		if (fault==7) assert(r.host_visible && r.readback_complete && r.mismatches);
	}
	for (unsigned word=0;word<1024;word++) for (unsigned fault=0;fault<2;fault++) {
		OutputMock io; tt_host_page_result h; PioneerResidentLease l; tt_resident_stream_result r; uint32_t output[1024]={};
		SetValues(io,1024); assert(Prepared(io,h,l)); io.faultWord=word;
		io.corruptReadback=fault==0; io.corruptFiniteWrite=fault==1;
		assert(!RunOutput(io,h,l,1024,output,r) && r.recovery_required && io.quarantined && r.mismatches);
		for (unsigned v:output) assert(!v);
	}
	{
		OutputMock io; tt_host_page_result h; PioneerResidentLease l; tt_resident_stream_result r; uint32_t output[32]={},snapshot[1024]={};
		SetValues(io,32); assert(Prepared(io,h,l)); unsigned initial=io.Mock::writes;
		for (unsigned words:{0U,1025U}) assert(!RunOutput(io,h,l,words,output,r));
		assert(!PioneerRuntimeOutputPage(io,io.Baseline(),io.Compute(),h,l,4,32,snapshot,output,r));
		assert(io.Mock::writes==initial && !io.requests && !l.recovery_required);
		io.delayVisibility=true;
		for (unsigned w=0;w<4;w++) io.counters[w]=io.writeCounters[w]=UINT32_MAX;
		io.accepted=UINT32_MAX;
		assert(RunOutput(io,h,l,32,output,r) && r.host_visible && !memcmp(output,io.values,sizeof(output)));
		assert(PioneerResidentLeaseEnd(io,io.Compute(),l) && io.ReleaseHostPage()==0); io.Safe();
	}
}
