/* SPDX-License-Identifier: MIT */
#define PIONEER_HOST_READ_MOCK_ONLY
#include "pioneer_host_read_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeHostWriteProbe.h"
struct HostWriteMock : HostReadMock {
	uint32_t writeCounters[4]={100,200,30,400};
	unsigned accepted=30,hostReads=0,hostDelay=0,readProgrammingWrites=0,writeBytes=64;
	bool writeAttempt=false,readAttempt=false,writeDone=false,readDone=false,hostCopied=false;
	bool neverVisible=false,wrongHostData=false,guardCorrupt=false,readTimeout=false,readBadFlits=false,readBadData=false;
	bool afterReadMapChange=false,afterReadSourceChange=false;
	HostWriteMock() { transferBytes=128; }
	uint32_t HostPageRead(unsigned w) {
		if (writeDone && !hostCopied && !neverVisible && ++hostReads>hostDelay) {
			unsigned hostWord=writeBytes==64?32:0;
			for (unsigned i=0;i<writeBytes/4;i++) page[hostWord+i]=destination[i]^(wrongHostData?1:0);
			if (guardCorrupt) page[0]^=1;
			hostCopied=true;
		}
		return PageMock::HostPageRead(w);
	}
	int32_t HostPageMap(uint64_t& address,uint64_t& bytes) {
		int32_t status=HostReadMock::HostPageMap(address,bytes);
		if (afterReadMapChange && readAttempt) address+=4096;
		return status;
	}
	void ResidentL1Write(unsigned w,uint32_t value) {
		if (writeAttempt) assert(readDone); // no restore after uncertain host traffic
		HostReadMock::ResidentL1Write(w,value);
	}
	uint32_t ResidentNocRead(uint32_t offset) {
		assert(BlackholeHostWriteReadRegister(offset)); Window(1,0xffa00000,1,2,false);
		if (offset==0x204 || offset==0x220 || offset==0x228) {
			if (writeAttempt || readAttempt) assert(doorbellRead);
			return writeCounters[offset==0x204?0:offset==0x220?1:3];
		}
		if (offset==0x210) { if (writeAttempt || readAttempt) assert(doorbellRead); return accepted; }
		if (offset==0x40) {
			if (writeAttempt || readAttempt) { doorbellRead=true; orderedReads++; }
			return (readAttempt && !readDone) || (writeAttempt && !writeDone)?1:0;
		}
		if (afterReadSourceChange && readAttempt && offset==BlackholeDramSourceOffset(1)) return source[1]^1;
		return Mock::ResidentNocRead(offset);
	}
	void ResidentNocWrite(uint32_t offset,uint32_t value) {
		assert(BlackholeResidentWriteRegister(offset)); Window(1,0xffa00000,1,2,false);
		if (offset!=0x40) {
			if (readAttempt) assert(readDone);
			if (writeAttempt && !readAttempt) {
				assert(writeDone && hostCopied); readProgrammingWrites++;
			}
			if (++Mock::writes==Mock::drop) return;
			if (offset<=0x24) command[offset/4]=value; else { assert(offset==0x100); config=value; }
			return;
		}
		assert(value==1 && config==0x16005 && pciCommand[0]==0x406 && pciCommand[1]==0x406 && !cached);
		bool read=command[7]==0;
		uint64_t host=physical+(writeBytes==64?128:0);
		if (!read) {
			assert(!writeAttempt && !readAttempt && requests==0);
			const uint32_t expected[]={0x180000-2*writeBytes,0,1|(2<<6),uint32_t(host),uint32_t(host>>32),19|(24<<6),0,0x212,writeBytes,0};
			assert(!memcmp(command,expected,sizeof(expected))); writeAttempt=true;
		} else {
			assert(writeDone && hostCopied && !readAttempt && requests==1);
			const uint32_t expected[]={uint32_t(host),uint32_t(host>>32),19|(24<<6),0x180000-writeBytes,0,1|(2<<6),0,0,writeBytes,0};
			assert(!memcmp(command,expected,sizeof(expected))); readAttempt=true;
		}
		doorbellRead=false;
		if (++Mock::writes==Mock::drop) return;
		requests++;
		if (!read) {
			if (acceptedOnly) { accepted++; writeCounters[3]++; }
			if (!timeout && !acceptedOnly) {
				writeDone=done=true; writeCounters[0]++; writeCounters[1]+=writeBytes/64+(badFlits?1:0); accepted++; writeCounters[3]++;
			}
		} else if (!readTimeout) {
			readDone=done=true; counters[0]++; counters[1]+=writeBytes/64+(readBadFlits?1:0); accepted++; counters[3]++;
			unsigned words=writeBytes/4,hostWord=writeBytes==64?32:0;
			for (unsigned w=0;w<words;w++) destination[words+w]=page[hostWord+w]^(readBadData?1:0);
		}
	}
};
template<unsigned Bytes=64>
static bool Run(HostWriteMock& io,tt_host_write_result& r)
{
	io.writeBytes=Bytes; io.transferBytes=2*Bytes;
	auto b=io.Baseline(); auto n=io.Compute(); auto p=io.Path(); auto a=io.Audit();
	return BlackholeHostWriteProbe<HostWriteMock,Bytes>(io,b,n,p,a,r);
}
template<unsigned Bytes>
void Suite()
{
	HostWriteMock io; tt_host_write_result result;
	assert(Run<Bytes>(io,result)); auto& r=result.transfer;
	assert(r.completed && result.write_completed && result.host_visible && result.read_completed && io.requests==2);
	assert(r.page.released && !r.recovery_required && !r.quarantined); io.Safe();
	unsigned writes=io.Mock::writes;
	for (unsigned drop=1;drop<=writes;drop++) {
		io={}; io.Mock::drop=drop; Run<Bytes>(io,result);
		assert(io.requests<=2);
		if (r.recovery_required) assert(io.quarantined && io.PageMock::allocated && !io.releases);
		else io.Safe();
		if (r.issued && !result.read_completed) assert(r.recovery_required);
	}
	for (unsigned fault=0;fault<14;fault++) {
		io={}; io.timeout=fault==0; io.acceptedOnly=fault==1; io.badFlits=fault==2;
		io.neverVisible=fault==3; io.wrongHostData=fault==4; io.guardCorrupt=fault==5;
		io.readTimeout=fault==6; io.readBadFlits=fault==7; io.clearFails=fault==8;
		io.restoreFails=fault==9; io.lateMapChange=fault==10; io.pciePending=fault==11;
		io.afterReadMapChange=fault==12; io.afterReadSourceChange=fault==13;
		assert(!Run<Bytes>(io,result) && r.recovery_required && io.quarantined && io.PageMock::allocated && !io.releases);
	}
	io={}; io.hostDelay=Bytes/2;
	assert(Run<Bytes>(io,result) && result.host_polls==3); io.Safe();
	io={}; io.readBadData=true;
	assert(!Run<Bytes>(io,result) && result.read_completed && r.mismatches==Bytes/4 && !r.recovery_required); io.Safe();
	io={}; io.physical=UINT64_C(0xfffffffff000); io.accepted=UINT32_MAX;
	for (unsigned w=0;w<4;w++) io.counters[w]=io.writeCounters[w]=UINT32_MAX;
	assert(Run<Bytes>(io,result)); io.Safe();
	for (unsigned fault=0;fault<7;fault++) {
		io={}; io.allocFail=fault==0; io.backupFail=fault==1; io.cacheFail=fault==2;
		io.badHdma=fault==3; io.badSii=fault==4; io.active=fault==5; io.irq=fault==6;
		assert(!Run<Bytes>(io,result) && !r.issued && !io.requests && !io.bmeWrites && !r.recovery_required);
		if (fault<2) assert(!io.releases); else io.Safe();
	}
}
int main() { Suite<64>(); Suite<4096>(); }
