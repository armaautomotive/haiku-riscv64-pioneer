/* SPDX-License-Identifier: MIT */
// Actual new native/control bodies against explicit KernelExport/PCI fixtures.
// Manually assigned result phases below are DOMAIN fixtures,not engine proofs.
#include "endpoints.h"
#define PIONEER_NATIVE_DRAM_MODEL_LEASE_TEST_ONLY
#include "pioneer_native_dram_model_band_lease_test.cpp"
namespace N=PioneerNativeDram81;
struct NativeFixture {
	PioneerProbeCookie fd;N::Cookie cookie;N::Owner* c;
	NativeFixture() {
		assert(views.empty() && !N::current);mirrors.clear();physicalViews.clear();config.clear();
		ioCalls=deletes=dropDelete=unreserves=pciWrites=dropPciWrite=pciReads=badPciRead=0;
		areaQueries=wordReads=wordWrites=effectiveUid=copyCalls=failCopy=probeFrees=reloadCalls=0;
		currentThread=10;currentTeam=20;semCount=0;fakeTime=123456;
		sColdRecoveryRequired=sReloadQuiesced=sBankStorageOnly=failUnreserve=failProfile=false;
		sCard=pci_info{};guard.version=1;sLock=42;sOpenCookies=1;sReloadGeneration=7;sReloadGuard=&guard;sPCI=&pci;sRuntimeOwner=&fd;
		cookie.base=&fd;cookie.used=true;c=new N::Owner(&cookie,thread_info{},0);N::current=c;c->reserved=true;
		c->profile.root_bus=0;c->profile.bar0_host_start=UINT64_C(0x4900000000);
	}
	auto& Result(){return const_cast<PioneerNativeDramModelBandResult&>(c->lease.Result());}
	void Select(bool gddr=false) {
		for(unsigned i=0;i<3;i++) {
			c->io.tlb[-3+int(i)]=BlackholeDramFixedTlbWord(gddr?0x04000000:0,gddr?18:1,gddr?12:2,false,i);
			c->io.tlb[i]=BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i);
		}
	}
	void Reset(uint32 value){*c->io.resetWrite=value;*const_cast<volatile uint32*>(c->io.reset)=value;}
	void Map(unsigned phase=1) {
		assert(c->Scope() && c->io.Map() && views.size()==17);
		const uint32 offsets[]={0x19000000,0x19312000,0x19313000,0x19320000};
		const area_id ids[]={c->io.memoryArea,c->io.controlArea,c->io.pcArea,c->io.writeArea};
		for(unsigned i=0;i<4;i++)assert(physicalViews.at(ids[i])==c->profile.bar0_host_start+offsets[i]
			&& views.at(ids[i]).size==(i?B_PAGE_SIZE:0x200000)
			&& views.at(ids[i]).protection==(B_KERNEL_READ_AREA|B_UNCACHED_MEMORY|(i==2?0:B_KERNEL_WRITE_AREA)));
		config[Key(1,0)]=0xb1401e52;config[Key(1,0x2c)]=0x00431e52;
		config[Key(0,0)]=0x20421e30;config[Key(0,0x18)]=0x003f0100;
		config[Key(1,4)]=config[Key(0,4)]=0x402;
		mirrors[c->io.resetWrite]=const_cast<volatile uint32*>(c->io.reset);
		for(unsigned a=0;a<0x1000;a+=4)mirrors[c->io.niuWrite+a/4]=const_cast<volatile uint32*>(c->io.nocRegisters)+a/4;
		c->dram.translated[0][0]=18;c->dram.translated[0][1]=12;c->dram.source[0]=0x6005;
		c->ready=true;Result().phase=phase;Select();Reset(0x47800);
		assert(c->Views() && c->Identity(true) && c->io.Active());
	}
	void Restored() {
		auto& r=Result();r.compute.state=PIONEER_NATIVE_BFP_CLOSED;r.compute.l1_restored=r.compute.tlb_restored=1;
		r.gddr_restored=r.niu_restored=r.clock_restored=1;c->transaction.config_attempted=true;
		for(unsigned i=0;i<6;i++)c->mapping.bars_before[i]=0xab000001+i;
		for(unsigned i=0;i<3;i++)c->mapping.bridge_before[i]=0xcd000001+i;
	}
	~NativeFixture() {
		// Explicit TEST endpoint teardown;never production recovery/cleanup.
		for(const auto& v:views)free(v.second.address);views.clear();mirrors.clear();physicalViews.clear();
		delete c;N::current=nullptr;sRuntimeOwner=nullptr;sColdRecoveryRequired=false;
	}
};
static PioneerNativeDramModelBandRequest Request(unsigned sample=PioneerNativeDramModelBandNone) {
	PioneerNativeDramModelBandRequest r={};r.version=1;r.sample=sample;
	r.expected_tag=PioneerNativeDramModelBandTag;r.expected_generation=7;return r;
}
int main(int argc,char** argv) {
	assert(argc==2);static_cast<void>(&NoMore);unsigned valid=0,refused=0;
	{
		NativeFixture f;f.Map();auto* hooks=find_device(TT_PCI_PROBE_DEVICE);
		assert(hooks && hooks->open==N::Open && hooks->free==N::Free && hooks->control==N::Control
			&& !hooks->read && !hooks->write && !find_device(nullptr) && !find_device("old-Work"));
		f.Restored();assert(N::Finish(*f.c) && views.empty() && deletes==17 && pciWrites==11 && f.c->mapping.restored);
		const auto before=ioCalls;assert(!N::Finish(*f.c) && ioCalls==before);
		assert(N::DisposeKnown(f.c) && unreserves==1 && !N::current && !sRuntimeOwner);f.c=nullptr;valid++;
	}
	for(unsigned fault=0;fault<20;fault++) {
		NativeFixture f;f.Map();
		if(fault==0)f.fd.magic=0;if(fault==1)f.fd.team++;if(fault==2)currentThread++;if(fault==3)currentTeam++;
		if(fault==4)semCount=1;if(fault==5)sReloadGeneration++;if(fault==6)sReloadQuiesced=true;
		if(fault==7)sRuntimeOwner=nullptr;if(fault==8)sBankStorageOnly=true;if(fault==9)sOpenCookies=2;
		if(fault==10)sPCI=nullptr;if(fault==11)sReloadGuard=nullptr;if(fault==12)f.fd.context=(PioneerStoreContext*)uintptr_t(1);
		if(fault==13)f.cookie.used=false;if(fault==14)N::current=nullptr;if(fault==15)sCard.bus++;
		if(fault==16)guard.version=2;if(fault==17)sLock++;if(fault==18)f.c->reserved=false;if(fault==19)f.cookie.base=nullptr;
		const auto before=ioCalls;uint32 v=0;uint64 now=0;
		assert(!f.c->Scope() && !f.c->io.NativeSourceRead(0,v) && f.c->retained && sColdRecoveryRequired && ioCalls==before);
		assert(!f.c->io.NativePause() && !f.c->io.NativeNow(now) && ioCalls==before);refused++;
	}
	for(unsigned index=0;index<17;index++)for(unsigned fault=0;fault<9;fault++) {
		NativeFixture f;f.Map();auto it=views.begin();std::advance(it,index);auto& v=it->second;const area_info original=v;
		if(fault==0)v.area++;if(fault==1)v.size-=4;if(fault==2)v.address=(uint8*)v.address+4;
		if(fault==3)v.protection|=B_WRITE_AREA;if(fault==4)v.protection^=B_KERNEL_READ_AREA;
		if(fault==5)v.protection|=B_EXECUTE_AREA;if(fault==6)v.protection|=B_KERNEL_EXECUTE_AREA;
		if(fault==7)v.protection^=B_KERNEL_WRITE_AREA;if(fault==8)v.protection|=B_READ_AREA;
		const auto beforeReads=wordReads,beforeWrites=wordWrites,beforePCI=pciReads;uint32 word=0;
		assert(!f.c->Views() && !f.c->io.NativeSourceRead(0,word) && f.c->retained);
		assert(wordReads==beforeReads && wordWrites==beforeWrites && pciReads==beforePCI && !unreserves && !deletes);
		v=original;const auto stopped=ioCalls;assert(!f.c->io.NativePause() && ioCalls==stopped);refused++;
	}
	for(unsigned fault=1;fault<=6;fault++) {
		NativeFixture f;f.Map();badPciRead=pciReads+fault;uint32 word=0;const auto before=wordReads;
		assert(!f.c->io.NativeSourceRead(0,word) && f.c->retained && wordReads==before);
		const auto stopped=ioCalls;assert(!f.c->io.NativeSourceRead(0,word) && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();uint32 v=0;const auto before=ioCalls;
		for(unsigned a:{1U,0x5ffcU,0x6004U,0x72000U,0x74020U,L::Weight-L::Header-4,L::WeightEnd(5120),L::InputEnd(5120),L::Output+256,0x17ffccU,UINT32_MAX}) {
			assert(!N::L1Address(a) && !f.c->io.NativeL1Read(a,v) && !f.c->io.NativeL1Write(a,1));refused++;
		}
		for(unsigned a:{0x03fffffcU,0x0402a800U,0x040ffffcU,0x04102800U,0x04000001U,UINT32_MAX}) {
			assert(!N::DramAddress(a) && !f.c->io.NativeDramRead(a,v) && !f.c->io.NativeDramWrite(a,1));refused++;
		}
		for(unsigned a:{0x28U,0x60U,0x2c0U,0x1ffcU,0x2000U,1U,UINT32_MAX})assert(!f.c->io.NativeNiuRead(a,v));
		for(unsigned a:{0x40U,0x60U,0x104U,0x840U,0x1040U,0x1840U,1U,UINT32_MAX})assert(!f.c->io.NativeNiuWrite(a,1));
		assert(!f.c->io.NativeTlbRead(2,0,v) && !f.c->io.NativeTlbRead(0,3,v)
			&& !f.c->io.NativeTlbWrite(0,3,0) && !f.c->io.NativeTlbWrite(2,0,0) && !f.c->io.NativeControlRead(3,v));
		assert(ioCalls==before && !f.c->retained);valid++;
		for(unsigned i=0;i<Lease::L1Words;i++)assert(N::L1Address(Lease::L1Address(i)));
		for(unsigned i=0;i<Lease::DramWords;i++)assert(N::DramAddress(Lease::DramAddress(i)));
		assert(!N::L1Address(Lease::L1Address(Lease::L1Words)) && !N::DramAddress(Lease::DramAddress(Lease::DramWords)));valid++;
	}
	{
		NativeFixture f;f.Map(4);uint32 v=0;
		for(unsigned a:{0x6000U,0x70000U,0x71ffcU,0x6e000U,0x6fffcU,0x74000U,0x7401cU,L::Weight-L::Header,L::WeightEnd(5120)-4,L::Input-L::Header,L::InputEnd(5120)-4,L::Output,L::Output+252,0x17ffc0U,0x17ffc8U}) {
			assert(f.c->io.NativeL1Write(a,a^0x12340000) && f.c->io.NativeL1Read(a,v) && v==(a^0x12340000));valid++;
		}
		f.Result().phase=8;f.c->resetStage=4;assert(f.c->io.NativeL1Write(0x6000,0x11223344));valid++;
		f.Result().phase=5;f.c->resetStage=2;f.c->fifoStage=1;f.Reset(0x46800);
		assert(!f.c->io.NativeL1Read(0x70000,v) && !f.c->io.NativeL1Write(0x6000,0) && !f.c->io.NativeL1Read(L::Output,v));refused++;
		assert(f.c->io.NativeL1Read(0x17ffc8,v));auto& r=f.Result().compute;r.phase=0x7000c;r.count=PioneerNativeDramModelBandVariant::Count;r.marker=PioneerNativeDramModelBandVariant::Marker;r.pc[0]=r.pc[1]=0x70038;
		assert(f.c->io.NativeL1Read(L::Output,v));r.marker^=1;assert(!f.c->io.NativeL1Read(L::Output,v));valid++;refused++;
	}
	{
		NativeFixture f;f.Map(3);f.Select(true);uint32 v=0;
		for(unsigned a:{0x04000000U,0x0402a7fcU,0x04100000U,0x041027fcU}) {
			assert(f.c->io.NativeDramWrite(a,a^0x98760000) && f.c->io.NativeDramRead(a,v) && v==(a^0x98760000));valid++;
		}
		f.Result().phase=2;assert(!f.c->io.NativeDramWrite(0x04000000,1) && f.c->io.NativeDramRead(0x04000000,v));refused++;
		f.Result().phase=7;f.c->resetStage=4;assert(f.c->io.NativeDramWrite(0x04000000,99));valid++;
		f.Select();assert(!f.c->io.NativeDramRead(0x04000000,v) && !f.c->io.NativeDramWrite(0x04000000,1));refused++;
	}
	{
		NativeFixture f;f.Map();uint32 v=0;auto* page=(volatile uint32*)views.at(f.c->io.controlArea).address;
		const unsigned offsets[]={0x234,0xa0,0x90};
		for(unsigned i=0;i<3;i++){page[offsets[i]/4]=0x12340000+i;assert(f.c->io.NativeControlRead(i,v) && v==0x12340000+i);valid++;}
		uint64 time=0;assert(f.c->io.NativeNow(time) && time==123456000);fakeTime=0;assert(!f.c->io.NativeNow(time));fakeTime=-1;assert(!f.c->io.NativeNow(time));valid++;refused+=2;
	}
	{
		NativeFixture f;f.Map();uint32 v=0;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++) {
			assert(f.c->io.NativeTlbRead(s,i,v) && f.c->haveTlb[s][i]);const auto first=v;
			f.c->io.tlb[-3+int(s*3+i)]^=1;assert(f.c->io.NativeTlbRead(s,i,v) && f.c->savedTlb[s][i]==first);
			f.c->io.tlb[-3+int(s*3+i)]=first;valid++;
		}
		const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
		for(unsigned phase:{1U,6U}) {
			f.Result().phase=phase;f.c->resetStage=phase==1?0:4;
			for(unsigned x:xs)for(unsigned y=2;y<=11;y++) {
				for(unsigned i=0;i<3;i++)assert(f.c->io.NativeTlbWrite(1,i,BlackholeDramFixedTlbWord(0xffa00000,x,y,true,i)));
				const auto reads=wordReads;assert(f.c->io.NativeResetRead(v) && v==0x47800 && wordReads==reads+4);valid++;
			}
		}
		f.Result().phase=5;const auto before=ioCalls;
		assert(!f.c->io.NativeTlbWrite(1,1,BlackholeDramFixedTlbWord(0xffa00000,14,11,true,1)) && ioCalls==before);refused++;
		f.Result().phase=9;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++) {
			const auto saved=f.c->savedTlb[s][i];assert(!f.c->io.NativeTlbWrite(s,i,saved^1));
			assert(f.c->io.NativeTlbWrite(s,i,saved));valid++;refused++;
		}
	}
	{
		NativeFixture f;f.Map(4);assert(f.c->io.NativeNiuWrite(0x100,0x16005) && f.c->fifoStage==1);
		assert(!f.c->io.NativeNiuWrite(0x100,0x16005));refused++;
		f.Result().phase=5;auto& r=f.Result().compute;
		assert(f.c->io.NativeResetWrite(0x87ffff) && f.c->resetStage==1 && *f.c->io.reset==0x87ffff);
		const auto before=ioCalls;assert(!f.c->io.NativeResetWrite(0x87ffff) && ioCalls==before);refused++;
		r.started=1;assert(f.c->io.NativeResetWrite(0x46800) && f.c->resetStage==2 && *f.c->io.reset==0x46800);
		assert(!f.c->io.NativeResetWrite(0x47800));refused++;
		f.Result().phase=6;r.completed=1;r.state=PIONEER_NATIVE_BFP_RESULT;
		assert(f.c->io.NativeResetWrite(0x87ffff) && f.c->resetStage==3 && *f.c->io.reset==0x87ffff);
		assert(f.c->io.NativeResetWrite(0x47800) && f.c->resetStage==4 && *f.c->io.reset==0x47800);
		f.Result().phase=9;assert(f.c->io.NativeNiuWrite(0x100,0x6005) && f.c->fifoStage==2);
		assert(!f.c->io.NativeNiuWrite(0x100,0x6005));valid++;refused++;
	}
	{
		NativeFixture f;f.Map();uint32 v=0;
		for(unsigned i=0;i<10;i++) {
			const_cast<volatile uint32*>(f.c->io.nocRegisters)[i]=0xab000000+i;
			assert(f.c->io.NativeNiuRead(i*4,v) && v==0xab000000+i && f.c->savedCommand[i]==v);valid++;
		}
		f.Result().phase=9;f.c->resetStage=4;
		for(unsigned i=0;i<10;i++) {
			assert(!f.c->io.NativeNiuWrite(i*4,0) && f.c->io.NativeNiuWrite(i*4,0xab000000+i));valid++;refused++;
		}
	}
	for(unsigned fail=1;fail<=17;fail++) {
		NativeFixture f;f.Map();f.Restored();dropDelete=fail;
		assert(!N::Finish(*f.c) && f.c->retained && deletes==fail && !pciWrites && !unreserves);
		const auto stopped=ioCalls;assert(!N::Finish(*f.c) && !N::DisposeKnown(f.c) && ioCalls==stopped);refused++;
	}
	for(unsigned fail=1;fail<=11;fail++) {
		NativeFixture f;f.Map();f.Restored();dropPciWrite=fail;
		assert(!N::Finish(*f.c) && deletes==17 && pciWrites==fail && !unreserves);
		// The actual END caller latches this failure;the closure helper never retries.
		f.c->Retain();const auto stopped=ioCalls;
		assert(!N::Finish(*f.c) && !N::DisposeKnown(f.c) && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();f.Restored();assert(N::Finish(*f.c));failUnreserve=true;
		assert(!N::DisposeKnown(f.c) && f.c->retained && unreserves==1);
		const auto stopped=ioCalls;assert(!N::DisposeKnown(f.c) && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();const auto before=ioCalls;
		assert(N::Free(&f.cookie)==B_OK && f.c->retained && sOpenCookies==0 && !probeFrees && ioCalls==before
			&& !deletes && !pciWrites && !unreserves);refused++;
	}
	{
		NativeFixture f;f.Map();auto r=Request();const auto before=ioCalls;
		assert(N::Control(&f.cookie,PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT,&r,sizeof(r))==B_OK && !r.result.compute.cold && ioCalls==before);valid++;
		for(unsigned byte=0;byte<sizeof(r.result);byte++) {
			r=Request();reinterpret_cast<uint8*>(&r.result)[byte]=1;
			assert(N::Control(&f.cookie,PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT,&r,sizeof(r))==B_BAD_VALUE);refused++;
		}
		for(unsigned fault=0;fault<12;fault++) {
			r=Request();uint32 operation=PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT;size_t length=sizeof(r);
			if(fault==0)r.expected_tag--;if(fault==1)r.version=0;if(fault==2)r.expected_generation=0;
			if(fault==3)r.expected_generation++;if(fault==4)r.reserved=1;if(fault==5)r.sample=0;
			if(fault==6)r.nonce=1;if(fault==7)r.accept=1;if(fault==8)length--;
			if(fault==9)operation--;if(fault==10)effectiveUid=1;if(fault==11)currentTeam++;
			assert(N::Control(&f.cookie,operation,&r,length)!=B_OK && ioCalls==before);effectiveUid=0;currentTeam=20;refused++;
		}
#if defined(PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64)
		for(unsigned base:{0x7d00U,0x7e00U,0x7f00U})for(unsigned op=0;op<3;op++) {
			r=Request();assert(N::Control(&f.cookie,B_DEVICE_OP_CODES_END+base+op,&r,sizeof(r))==B_NOT_ALLOWED && ioCalls==before);refused++;
		}
		for(uint64 tag:{UINT64_C(0x2026110400010081),UINT64_C(0x2026110400010082),UINT64_C(0x2026110400010083)}) {
			r=Request();r.expected_tag=tag;
			assert(N::Control(&f.cookie,PIONEER_NATIVE_DRAM_MODEL_BAND_SUPPORT,&r,sizeof(r))==B_BAD_VALUE && ioCalls==before);refused++;
		}
		uint32 word=0;
		for(unsigned oldOnly:{0x125fe0U,0x129bdcU,0x134000U,0x1340fcU}) {
			assert(!N::L1Address(oldOnly) && !f.c->io.NativeL1Read(oldOnly,word) && !f.c->io.NativeL1Write(oldOnly,0) && ioCalls==before);refused++;
		}
#endif
		r=Request(0);assert(N::Control(&f.cookie,PIONEER_NATIVE_DRAM_MODEL_BAND_RUN,&r,sizeof(r))==B_BUSY && ioCalls==before);refused++;
		r=Request(0);r.nonce=r.accept=1;assert(N::Control(&f.cookie,PIONEER_NATIVE_DRAM_MODEL_BAND_END,&r,sizeof(r))==B_BAD_VALUE && ioCalls==before);refused++;
	}
	for(unsigned fault=0;fault<5;fault++) {
		NativeFixture f;delete f.c;f.c=nullptr;N::current=nullptr;sRuntimeOwner=nullptr;f.cookie.used=false;failProfile=true;
		if(fault==1)failUnreserve=true;if(fault==2)failCopy=1;if(fault==3)failCopy=2;
		if(fault==4){sOpenCookies=2;}
		auto r=Request(0);const auto status=N::Control(&f.cookie,PIONEER_NATIVE_DRAM_MODEL_BAND_RUN,&r,sizeof(r));
		f.c=N::current;
		assert(views.empty() && !pciWrites && !wordWrites && !wordReads);
		if(fault==0 || fault==3) {
			assert(status==(fault==0?B_OK:B_NOT_ALLOWED) && !N::current && !sRuntimeOwner && !sColdRecoveryRequired
				&& unreserves==1 && f.cookie.used);
			if(!fault)assert(r.result.compute.state==PIONEER_NATIVE_BFP_REFUSED && !r.result.compute.cold);valid++;
		} else if(fault==1) {
			assert(status==B_OK && f.c && f.c->retained && sColdRecoveryRequired && unreserves==1
				&& r.result.compute.state==PIONEER_NATIVE_BFP_UNKNOWN && r.result.compute.cold);
			const auto stopped=ioCalls;r=Request(0);
			assert(N::Control(&f.cookie,PIONEER_NATIVE_DRAM_MODEL_BAND_RUN,&r,sizeof(r))==B_DEV_NOT_READY && ioCalls==stopped);refused++;
		} else assert(status!=B_OK && !ioCalls && !f.cookie.used && !N::current);
		refused++;
	}
	for(unsigned phase:{0U,2U,3U,4U,5U,6U,7U,8U,10U}) {
		NativeFixture f;f.Map();f.Result().phase=phase;f.c->clockCall=true;
		const auto stopped=ioCalls;assert(f.c->clock.TlbRead(0)==UINT32_MAX && f.c->retained && ioCalls==stopped);
		assert(f.c->clock.CsmRead(0x10000000)==UINT32_MAX && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();f.c->clockCall=true;
		f.c->clock.TlbWrite(0,0x1234);assert(f.c->clock.TlbRead(0)==0x1234 && !f.c->retained);valid++;
		f.c->clockCall=false;const auto stopped=ioCalls;
		assert(f.c->clock.TlbRead(0)==UINT32_MAX && f.c->retained && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();Image::code=Read((std::string(argv[1])+"/text.bin").c_str());
		const auto boot=Read((std::string(argv[1])+"/boot.bin").c_str());assert(boot.size()==4);Image::Boot=::Word(boot,0);
		LeaseIO shadow(f.c->lease,argv[1]);f.c->dram=shadow.Baseline();shadow.proof=&f.c->dram;
		assert((f.c->lease.Run<LeaseIO,Image>(shadow,f.c->dram,0)) && shadow.executed && shadow.recordedL1 && shadow.recordedDram);
		// Authentic private journal/proof binding from REAL header+RV32 co-sim,
		// combined with OS/PCI mapping FIXTURES,not fabricated native completion.
		f.c->resetStage=2;f.c->fifoStage=1;f.c->clockEntered=true;
		assert(f.c->io.NativeScope(&f.c->lease,&f.c->dram,shadow.recordedL1,Lease::L1Words,shadow.recordedDram,Lease::DramWords));valid++;
		for(unsigned fault=0;fault<8;fault++) {
			const Lease* subject=&f.c->lease;const tt_dram_samples_result* proof=&f.c->dram;
			const uint32* l1=shadow.recordedL1;const uint32* dram=shadow.recordedDram;unsigned n=Lease::L1Words,m=Lease::DramWords;
			if(fault==0)subject=nullptr;if(fault==1)proof=nullptr;if(fault==2)l1++;if(fault==3)dram++;
			if(fault==4)n--;if(fault==5)m--;if(fault==6)f.c->resetStage=3;if(fault==7)f.c->clockEntered=false;
			const auto before=ioCalls;assert(!f.c->io.NativeScope(subject,proof,l1,n,dram,m) && ioCalls==before);
			f.c->resetStage=2;f.c->clockEntered=true;refused++;
		}
	}
	printf("PASS actual native%u endpoint fixtures:valid%u refused%u;NO prerequisite/ARC/engine/hardware/model/speed authority\n",PioneerNativeDramModelBandVariant::Role,valid,refused);
}
