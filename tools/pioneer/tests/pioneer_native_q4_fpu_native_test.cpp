/* SPDX-License-Identifier: MIT */
// Actual adapter/control bodies over explicit OS/PCI fixtures,NOT hardware.
#include "endpoints.h"
#define PIONEER_NATIVE_Q4_FPU_LEASE_TEST_ONLY
#include "pioneer_native_q4_fpu_lease_test.cpp"
namespace N=PioneerNativeQ4Fpu85;
template<class T,class=void>struct HasDramWrite:std::false_type{};
template<class T>struct HasDramWrite<T,std::void_t<decltype(&T::NativeDramWrite)>>:std::true_type{};
template<class T,class=void>struct HasNiuWrite:std::false_type{};
template<class T>struct HasNiuWrite<T,std::void_t<decltype(&T::NativeNiuWrite)>>:std::true_type{};
static_assert(!HasDramWrite<N::IO>::value && !HasNiuWrite<N::IO>::value,"NO GDDR/NIU write capability");
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
	auto& Result(){return const_cast<PioneerNativeQ4FpuResult&>(c->lease.Result());}
	void Select() {
		for(unsigned i=0;i<3;i++){c->io.tlb[-3+int(i)]=BlackholeDramFixedTlbWord(0,1,2,false,i);c->io.tlb[i]=BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i);}
	}
	void Reset(uint32 value){*c->io.resetWrite=value;*const_cast<volatile uint32*>(c->io.reset)=value;}
	void Map(unsigned phase=1) {
		assert(c->Scope() && c->io.Map() && views.size()==16);
		const uint32 offsets[]={0x19000000,0x19312000,0x19313000};
		const area_id ids[]={c->io.memoryArea,c->io.controlArea,c->io.pcArea};
		for(unsigned i=0;i<3;i++)assert(physicalViews.at(ids[i])==c->profile.bar0_host_start+offsets[i]
			&& views.at(ids[i]).size==(i?B_PAGE_SIZE:0x200000)
			&& views.at(ids[i]).protection==(B_KERNEL_READ_AREA|B_UNCACHED_MEMORY|(i==2?0:B_KERNEL_WRITE_AREA)));
		config[Key(1,0)]=0xb1401e52;config[Key(1,0x2c)]=0x00431e52;config[Key(0,0)]=0x20421e30;config[Key(0,0x18)]=0x003f0100;
		config[Key(1,4)]=config[Key(0,4)]=0x402;mirrors[c->io.resetWrite]=const_cast<volatile uint32*>(c->io.reset);
		c->ready=true;Result().phase=phase;Select();Reset(0x47800);assert(c->Views() && c->Identity(true) && c->io.Active());
	}
	void Restored() {
		auto& r=Result();r.compute.state=PIONEER_NATIVE_BFP_CLOSED;r.compute.l1_restored=r.compute.tlb_restored=1;
		r.niu_restored=r.clock_restored=1;c->transaction.config_attempted=true;
		for(unsigned i=0;i<6;i++)c->mapping.bars_before[i]=0xab000001+i;
		for(unsigned i=0;i<3;i++)c->mapping.bridge_before[i]=0xcd000001+i;
	}
	~NativeFixture() { // Explicit TEST teardown,never production recovery.
		for(const auto& v:views)free(v.second.address);views.clear();mirrors.clear();physicalViews.clear();
		delete c;N::current=nullptr;sRuntimeOwner=nullptr;sColdRecoveryRequired=false;
	}
};
static PioneerNativeQ4FpuRequest Request(unsigned sample=PioneerNativeQ4FpuNone) {
	PioneerNativeQ4FpuRequest r={};r.version=1;r.sample=sample;r.expected_tag=PioneerNativeQ4FpuVariant::Tag;r.expected_generation=7;return r;
}
int main(int argc,char** argv) {
	assert(argc==2);static_cast<void>(&NoMore);unsigned valid=0,refused=0;
	{
		NativeFixture f;f.Map();auto* hooks=find_device(TT_PCI_PROBE_DEVICE);
		assert(hooks && hooks->open==N::Open && hooks->free==N::Free && hooks->control==N::Control && !hooks->read && !hooks->write
			&& !find_device(nullptr) && !find_device("old-Work"));
		f.Restored();assert(N::Finish(*f.c) && views.empty() && deletes==16 && pciWrites==11 && f.c->mapping.restored);
		const auto before=ioCalls;assert(!N::Finish(*f.c) && ioCalls==before);
		assert(N::DisposeKnown(f.c) && unreserves==1 && !N::current && !sRuntimeOwner);f.c=nullptr;valid++;
	}
	for(unsigned fault=0;fault<20;fault++) {
		NativeFixture f;f.Map();
		if(fault==0)f.fd.magic=0;if(fault==1)f.fd.team++;if(fault==2)currentThread++;if(fault==3)currentTeam++;
		if(fault==4)semCount=1;if(fault==5)sReloadGeneration++;if(fault==6)sReloadQuiesced=true;if(fault==7)sRuntimeOwner=nullptr;
		if(fault==8)sBankStorageOnly=true;if(fault==9)sOpenCookies=2;if(fault==10)sPCI=nullptr;if(fault==11)sReloadGuard=nullptr;
		if(fault==12)f.fd.context=(PioneerStoreContext*)uintptr_t(1);if(fault==13)f.cookie.used=false;if(fault==14)N::current=nullptr;
		if(fault==15)sCard.bus++;if(fault==16)guard.version=2;if(fault==17)sLock++;if(fault==18)f.c->reserved=false;if(fault==19)f.cookie.base=nullptr;
		const auto before=ioCalls;uint32 v=0;uint64 now=0;
		assert(!f.c->Scope() && !f.c->io.NativeSourceRead(0,v) && f.c->retained && sColdRecoveryRequired && ioCalls==before);
		assert(!f.c->io.NativePause() && !f.c->io.NativeNow(now) && ioCalls==before);refused++;
	}
	for(unsigned index=0;index<16;index++)for(unsigned fault=0;fault<9;fault++) {
		NativeFixture f;f.Map();auto it=views.begin();std::advance(it,index);auto& v=it->second;const area_info original=v;
		if(fault==0)v.area++;if(fault==1)v.size-=4;if(fault==2)v.address=(uint8*)v.address+4;if(fault==3)v.protection|=B_WRITE_AREA;
		if(fault==4)v.protection^=B_KERNEL_READ_AREA;if(fault==5)v.protection|=B_EXECUTE_AREA;if(fault==6)v.protection|=B_KERNEL_EXECUTE_AREA;
		if(fault==7)v.protection^=B_KERNEL_WRITE_AREA;if(fault==8)v.protection|=B_READ_AREA;
		const auto reads=wordReads,writes=wordWrites,pciBefore=pciReads;uint32 word=0;
		assert(!f.c->Views() && !f.c->io.NativeSourceRead(0,word) && f.c->retained && wordReads==reads && wordWrites==writes
			&& pciReads==pciBefore && !unreserves && !deletes);v=original;
		const auto stopped=ioCalls;assert(!f.c->io.NativePause() && ioCalls==stopped);refused++;
	}
	for(unsigned fault=1;fault<=6;fault++) {
		NativeFixture f;f.Map();badPciRead=pciReads+fault;uint32 word=0;const auto before=wordReads;
		assert(!f.c->io.NativeSourceRead(0,word) && f.c->retained && wordReads==before);
		const auto stopped=ioCalls;assert(!f.c->io.NativeSourceRead(0,word) && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();uint32 v=0;const auto before=ioCalls;
		for(unsigned a:{1U,0x5ffcU,0x6004U,0x72000U,0x74000U,0x8ffbcU,0x98100U,0x17ffccU,UINT32_MAX}) {
			assert(!N::L1Address(a) && !f.c->io.NativeL1Read(a,v) && !f.c->io.NativeL1Write(a,1));refused++;
		}
		for(unsigned a:{0x28U,0x60U,0x2c0U,0x1ffcU,0x2000U,1U,UINT32_MAX})assert(!f.c->io.NativeNiuRead(a,v));
		assert(!f.c->io.NativeTlbRead(2,0,v) && !f.c->io.NativeTlbRead(0,3,v) && !f.c->io.NativeTlbWrite(0,3,0)
			&& !f.c->io.NativeTlbWrite(2,0,0) && !f.c->io.NativeControlRead(3,v) && ioCalls==before && !f.c->retained);
		for(unsigned i=0;i<Lease::L1Words;i++)assert(N::L1Address(Lease::L1Address(i)));valid++;
	}
	{
		NativeFixture f;f.Map(3);uint32 v=0;
		for(unsigned a:{0x6000U,0x70000U,0x71ffcU,0x6e000U,0x6fffcU,0x8ffc0U,0x90000U,0x94000U,0x96000U,0x97000U,0x98000U,0x980fcU,0x17ffc0U,0x17ffc8U}) {
			assert(f.c->io.NativeL1Write(a,a^0x12340000) && f.c->io.NativeL1Read(a,v) && v==(a^0x12340000));valid++;
		}
		f.Result().phase=6;f.c->resetStage=4;assert(f.c->io.NativeL1Write(0x6000,0x11223344));valid++;
		f.Result().phase=4;f.c->resetStage=2;f.Reset(0x46800);
		assert(!f.c->io.NativeL1Read(0x70000,v) && !f.c->io.NativeL1Write(0x6000,0) && !f.c->io.NativeL1Read(Layout::Output,v));refused++;
		assert(f.c->io.NativeL1Read(0x17ffc8,v));auto& r=f.Result().compute;r.phase=0x7000c;r.count=PioneerNativeQ4FpuVariant::Count;
		r.marker=PioneerNativeQ4FpuVariant::Marker;r.pc[0]=r.pc[1]=0x70038;
		assert(f.c->io.NativeL1Read(Layout::Output,v));r.marker^=1;assert(!f.c->io.NativeL1Read(Layout::Output,v));valid++;refused++;
	}
	{
		NativeFixture f;f.Map();uint32 v=0;const unsigned offsets[]={0x234,0xa0,0x90};auto* page=(volatile uint32*)views.at(f.c->io.controlArea).address;
		for(unsigned i=0;i<3;i++){page[offsets[i]/4]=0x12340000+i;assert(f.c->io.NativeControlRead(i,v) && v==0x12340000+i);valid++;}
		uint64 now=0;assert(f.c->io.NativeNow(now) && now==123456000);fakeTime=0;assert(!f.c->io.NativeNow(now));fakeTime=-1;assert(!f.c->io.NativeNow(now));refused+=2;
	}
	{
		NativeFixture f;f.Map();uint32 v=0;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){assert(f.c->io.NativeTlbRead(s,i,v) && f.c->haveTlb[s][i]);valid++;}
		for(unsigned phase:{1U,5U}) {
			f.Result().phase=phase;f.c->resetStage=phase==1?0:4;
			for(unsigned x:{1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U})for(unsigned y=2;y<=11;y++) {
				for(unsigned i=0;i<3;i++)assert(f.c->io.NativeTlbWrite(1,i,BlackholeDramFixedTlbWord(0xffa00000,x,y,true,i)));
				assert(f.c->io.NativeResetRead(v) && v==0x47800);valid++;
			}
		}
		f.Result().phase=4;const auto before=ioCalls;assert(!f.c->io.NativeTlbWrite(1,1,BlackholeDramFixedTlbWord(0xffa00000,14,11,true,1)) && ioCalls==before);refused++;
		f.Result().phase=7;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){auto saved=f.c->savedTlb[s][i];assert(!f.c->io.NativeTlbWrite(s,i,saved^1) && f.c->io.NativeTlbWrite(s,i,saved));valid++;refused++;}
	}
	{
		NativeFixture f;f.Map(4);auto& r=f.Result().compute;
		assert(f.c->io.NativeResetWrite(0x87ffff) && f.c->resetStage==1);const auto before=ioCalls;
		assert(!f.c->io.NativeResetWrite(0x87ffff) && ioCalls==before);refused++;
		r.started=1;assert(f.c->io.NativeResetWrite(0x46800) && f.c->resetStage==2);assert(!f.c->io.NativeResetWrite(0x47800));refused++;
		f.Result().phase=5;r.completed=1;r.state=PIONEER_NATIVE_BFP_RESULT;
		assert(f.c->io.NativeResetWrite(0x87ffff) && f.c->resetStage==3 && f.c->io.NativeResetWrite(0x47800) && f.c->resetStage==4);valid++;
	}
	for(unsigned fail=1;fail<=16;fail++) {
		NativeFixture f;f.Map();f.Restored();dropDelete=fail;
		assert(!N::Finish(*f.c) && f.c->retained && deletes==fail && !pciWrites && !unreserves);
		const auto stopped=ioCalls;assert(!N::Finish(*f.c) && !N::DisposeKnown(f.c) && ioCalls==stopped);refused++;
	}
	for(unsigned fail=1;fail<=11;fail++) {
		NativeFixture f;f.Map();f.Restored();dropPciWrite=fail;assert(!N::Finish(*f.c) && deletes==16 && pciWrites==fail && !unreserves);
		f.c->Retain();const auto stopped=ioCalls;assert(!N::Finish(*f.c) && !N::DisposeKnown(f.c) && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();f.Restored();assert(N::Finish(*f.c));failUnreserve=true;assert(!N::DisposeKnown(f.c) && f.c->retained && unreserves==1);
		const auto stopped=ioCalls;assert(!N::DisposeKnown(f.c) && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();const auto before=ioCalls;assert(N::Free(&f.cookie)==B_OK && f.c->retained && sOpenCookies==0
			&& !probeFrees && ioCalls==before && !deletes && !pciWrites && !unreserves);refused++;
	}
	{
		NativeFixture f;f.Map();auto r=Request();const auto before=ioCalls;
		assert(N::Control(&f.cookie,PIONEER_NATIVE_Q4_FPU_SUPPORT,&r,sizeof(r))==B_OK && !r.result.compute.cold && ioCalls==before);valid++;
		for(unsigned byte=0;byte<sizeof(r.result);byte++){r=Request();reinterpret_cast<uint8*>(&r.result)[byte]=1;assert(N::Control(&f.cookie,PIONEER_NATIVE_Q4_FPU_SUPPORT,&r,sizeof(r))==B_BAD_VALUE);refused++;}
		for(unsigned fault=0;fault<12;fault++) {
			r=Request();uint32 op=PIONEER_NATIVE_Q4_FPU_SUPPORT;size_t length=sizeof(r);
			if(fault==0)r.expected_tag--;if(fault==1)r.version=0;if(fault==2)r.expected_generation=0;if(fault==3)r.expected_generation++;
			if(fault==4)r.reserved=1;if(fault==5)r.sample=0;if(fault==6)r.nonce=1;if(fault==7)r.accept=1;
			if(fault==8)length--;if(fault==9)op--;if(fault==10)effectiveUid=1;if(fault==11)currentTeam++;
			assert(N::Control(&f.cookie,op,&r,length)!=B_OK && ioCalls==before);effectiveUid=0;currentTeam=20;refused++;
		}
		for(unsigned base=0x7b00;base<=0x8000;base+=0x100)for(unsigned op=0;op<3;op++) {
			r=Request();assert(N::Control(&f.cookie,B_DEVICE_OP_CODES_END+base+op,&r,sizeof(r))==B_NOT_ALLOWED && ioCalls==before);refused++;
		}
		r=Request(0);assert(N::Control(&f.cookie,PIONEER_NATIVE_Q4_FPU_RUN,&r,sizeof(r))==B_BUSY && ioCalls==before);refused++;
		// Wrong calling thread must reject END before touching the retained lease.
		f.Result().compute.state=PIONEER_NATIVE_BFP_RESULT;currentThread++;r=Request(0);r.nonce=r.accept=1;
		assert(N::Control(&f.cookie,PIONEER_NATIVE_Q4_FPU_END,&r,sizeof(r))==B_NOT_ALLOWED && ioCalls==before && !f.c->retained);refused++;
	}
	for(unsigned fault=0;fault<5;fault++) {
		NativeFixture f;delete f.c;f.c=nullptr;N::current=nullptr;sRuntimeOwner=nullptr;f.cookie.used=false;failProfile=true;
		if(fault==1)failUnreserve=true;if(fault==2)failCopy=1;if(fault==3)failCopy=2;if(fault==4)sOpenCookies=2;
		auto r=Request(0);const auto status=N::Control(&f.cookie,PIONEER_NATIVE_Q4_FPU_RUN,&r,sizeof(r));f.c=N::current;
		assert(views.empty() && !pciWrites && !wordWrites && !wordReads);
		if(fault==0 || fault==3){assert(status==(fault==0?B_OK:B_NOT_ALLOWED) && !N::current && !sRuntimeOwner && !sColdRecoveryRequired && unreserves==1 && f.cookie.used);valid++;}
		else if(fault==1){assert(status==B_OK && f.c && f.c->retained && sColdRecoveryRequired && unreserves==1 && r.result.compute.cold);refused++;}
		else assert(status!=B_OK && !ioCalls && !f.cookie.used && !N::current);refused++;
	}
	for(unsigned phase:{0U,2U,3U,4U,5U,6U,7U,9U}) {
		NativeFixture f;f.Map();f.Result().phase=phase;f.c->clockCall=true;const auto stopped=ioCalls;
		assert(f.c->clock.TlbRead(0)==UINT32_MAX && f.c->retained && ioCalls==stopped && f.c->clock.CsmRead(0x10000000)==UINT32_MAX && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();f.c->clockCall=true;f.c->clock.TlbWrite(0,0x1234);assert(f.c->clock.TlbRead(0)==0x1234 && !f.c->retained);valid++;
		f.c->clockCall=false;const auto stopped=ioCalls;assert(f.c->clock.TlbRead(0)==UINT32_MAX && f.c->retained && ioCalls==stopped);refused++;
	}
	{
		NativeFixture f;f.Map();Image::code=Read((std::string(argv[1])+"/text.bin").c_str());Image::Boot=::Word(Read((std::string(argv[1])+"/boot.bin").c_str()),0);
		for(unsigned i=0;i<3;i++)Data::samples.emplace_back(i);
		LeaseIO shadow(f.c->lease,argv[1]);f.c->dram=shadow.Baseline();shadow.proof=&f.c->dram;
		assert((f.c->lease.Run<LeaseIO,Image,Data>(shadow,f.c->dram,0)) && shadow.executed && shadow.backing);
		f.c->resetStage=2;f.c->clockEntered=true;assert(f.c->io.NativeScope(&f.c->lease,&f.c->dram,shadow.backing,Lease::L1Words));valid++;
		for(unsigned fault=0;fault<6;fault++) {
			const Lease* subject=&f.c->lease;const tt_dram_samples_result* proof=&f.c->dram;const uint32* l1=shadow.backing;unsigned n=Lease::L1Words;
			if(fault==0)subject=nullptr;if(fault==1)proof=nullptr;if(fault==2)l1++;if(fault==3)n--;if(fault==4)f.c->resetStage=3;if(fault==5)f.c->clockEntered=false;
			const auto before=ioCalls;assert(!f.c->io.NativeScope(subject,proof,l1,n) && ioCalls==before);f.c->resetStage=2;f.c->clockEntered=true;refused++;
		}
	}
	printf("PASS actual native85/control fixture bodies:valid%u refused%u,16 checked views,NO GDDR/NIU write method;NO prerequisite/ARC/native-engine/hardware/model/speed authority\n",valid,refused);
}
