/* SPDX-License-Identifier: MIT */
// Actual linked RAW ROOT Closed()/Scope()/packet code with injected, fixed
// predecessor history. The host supplies only fake NoC transport, not the
// command stream, result counters, source checks, or success decision.
#define PIONEER_TENSIX_MACHINE_ONLY
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38GlobalChainPackage.h"
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38TokenMetadata.h"
#include "../PioneerQ38TokenLayout.h"
#include <algorithm>
#include <array>
#include <memory>
#include <string>

namespace T=PioneerQ38TokenLayout;namespace C=T::C;namespace R=C::R;
namespace G=PioneerQ38GlobalHeadLayout::G;namespace M=PioneerQ38TokenMetadata;
static constexpr uint32_t src[16]={0x100,0x118,0x11c,0x120,0x124,0x128,0x12c,0x130,
	0x134,0x138,0x13c,0x140,0x144,0x150,0x154,0x16c};
static constexpr uint32_t count[8]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};

struct Image {
	std::vector<std::pair<unsigned,std::vector<uint8_t>>> rom;
	unsigned closed=0,fault=0;
	explicit Image(const std::string& dir){
		for(auto p:{std::make_pair("text",0x40000U),{"terminal",0x60000U},{"conv",0x64000U}})
			rom.push_back({p.second,Read((dir+"/"+p.first+".bin").c_str())});
		std::ifstream f(dir+"/program.symbols");std::string line;
		while(std::getline(f,line)){
			if(line.find(" t GlobalChain::Closed(")!=std::string::npos){assert(!closed);closed=std::stoul(line,nullptr,16);}
			if(line.find(" T q38_token_fault_report")!=std::string::npos){assert(!fault);fault=std::stoul(line,nullptr,16);}
		}
		assert(closed && fault);
	}
};

struct HeadMachine:Machine {
	enum {Niu=0xffb20000U,Parent=0x6f800U,Header=0x17e000U,Return=0x60004U};
	enum Fault {None,ChangedOwner,WrongSource,WrongCounter,LostReply};
	using Command=std::array<uint32_t,10>;
	const Image& image;const bool queued,stop;const Fault fault;const unsigned worker;
	std::vector<uint32_t> memory=std::vector<uint32_t>(0x180000/4);
	std::vector<Command> submitted,pending;
	std::vector<std::pair<unsigned,unsigned>> stream;
	std::vector<unsigned> completedDestinations;
	std::array<uint32_t,16> rootSource={};
	std::array<uint32_t,8> remoteCounts={},counters={};
	uint32_t boot[64]={},job[64]={},status[16]={};Command command={};
	unsigned maxPending=0,batches=0;
	bool fifoVisibleEmpty=true;
	uint32_t& Data(unsigned a){assert(!(a&3) && a>=0x6e000 && a<0x180000);return memory[a/4];}
	uint32_t& Frontier(unsigned i){return Data(Parent+(192+8+i)*4);}
	HeadMachine(const Image& code,bool batch,bool stopping,unsigned w,Fault inject)
		:image(code),queued(batch && !stopping),stop(stopping),fault(inject),worker(w){
		auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
		const unsigned bank=3,position=23,token=17,workspace=c->Base+c->Workspace(bank)->offset;
		std::array<uint32_t,1920> sources;
		for(unsigned i=0;i<sources.size();i++)sources[i]=i%16?0x34001200U+i:0x14000;
		const PioneerQ38GdnIdentity identity={1,2,3,4};
		auto parent=std::make_unique<PioneerQ38GlobalChainPackage>();
		assert(parent->Build(*c,63,bank,identity,sources.data(),bank,workspace+R::FinalOffset));
		const auto* b=parent->Boot();uint32_t outer[128];std::copy_n(parent->Outer(),128,outer);
		assert(M::Outer(outer,63,position,token,bank,workspace,b) && M::Frame(outer,63,position,token,bank,workspace,b));
		for(unsigned i=0;i<64;i++)Data(Parent+i*4)=Data(R::Boot+i*4)=b[i];
		for(unsigned i=0;i<128;i++)Data(Parent+(64+i)*4)=Data(R::Outer+i*4)=outer[i];
		std::copy_n(b+16,16,rootSource.data());Data(R::Doorbell)=outer[48];
		uint32_t state[64]={},session[128]={};
		state[0]=bank;state[1]=workspace;state[2]=63;state[3]=64;state[6]=64;state[16]=position;state[17]=token;
		state[18]=position+unsigned(stop);state[19]=state[18]&1;state[20]=25;state[21]=248319;state[22]=2;
		state[24]=248318;state[25]=248317;state[26]=state[28]=1;
		for(unsigned i=0;i<8;i++)state[8+i]=counters[i]=i==6?2:i==1?0xfffffff8U:i;
		for(unsigned i=0;i<64;i++)Data(Parent+(192+i)*4)=state[i];Data(Parent+256*4)=stop?6:4;
		const uint32_t prefix[]={T::Magic,1,128,bank,workspace,25,2,248319};
		std::copy_n(prefix,8,session);std::copy_n(b+16,16,session+8);std::copy_n(b+8,8,session+24);
		session[33]=248318;session[34]=248317;session[35]=8192;assert(T::Session(session,b,bank,workspace));
		for(unsigned i=0;i<128;i++)Data(T::SessionLocal+i*4)=session[i];
		auto head=std::make_unique<PioneerQ38GlobalHeadPlan>();assert(head->Build(*c,bank,b));
		for(unsigned i=0;i<128;i++)Data(C::Head+i*4)=Data(Header+i*4)=head->Data()[i];
		auto output=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(output->Build(*c,position,identity,sources.data()));
		assert(output->WorkerBoot(worker+1,boot));
		if(stop){const unsigned prefixStop[]={G::Magic,1,G::Stop,worker+1,G::Peer(worker+1),64,position,9999};
			std::copy_n(prefixStop,8,job);std::copy_n(boot+8,8,job+8);
		}else assert(T::Job(head->Data(),position,worker,0,boot,job));
		assert(G::JobShape(job,boot,0,0));
		for(unsigned i=0;i<64;i++){Data(R::PeerBoots+worker*256+i*4)=boot[i];Data(R::PeerJobs+worker*256+i*4)=job[i];}
		for(unsigned i=0;i<8;i++)Data(R::Counters+worker*32+i*4)=remoteCounts[i]=77+i-(i==6?4:0);
		Data(R::Nonces+worker*4)=1;
		status[0]=status[1]=1;status[3]=G::Magic;status[4]=stop?unsigned(G::Stop):0;status[6]=64;status[7]=position;
		regs[1]=Return;regs[2]=regs[27]=Parent;regs[10]=Header;regs[11]=worker;regs[12]=stop;pc=image.closed;
	}
	const std::vector<uint8_t>* Section(unsigned at,unsigned& base)const{
		for(const auto& p:image.rom)if(at>=p.first && at-p.first<p.second.size()){base=p.first;return &p.second;}
		return nullptr;
	}
	bool CodeAddress(unsigned at)const override{unsigned base;return Section(at,base);}
	unsigned Index(unsigned at)override{unsigned base;const auto* s=Section(at,base);assert(s);l1[8190]=Word(*s,at-base);return 8190;}
	unsigned StepLimit()const override{return 20000000;}
	unsigned Peer(unsigned at){
		if(at==0xffb121b0)return 0x46800;
		if(at==0xffb13140)return stop?0x20060:0x2004c;
		if(at>=G::Boot && at<G::Boot+256)return boot[(at-G::Boot)/4];
		if(at>=G::Job && at<G::Job+256)return job[(at-G::Job)/4];
		if(at>=G::Status && at<G::Status+64)return status[(at-G::Status)/4];
		if(at==G::Doorbell)return 1;
		for(unsigned i=0;i<16;i++)if(at==Niu+src[i])return boot[16+i]^unsigned(fault==WrongSource && i==5);
		for(unsigned i=0;i<8;i++)if(at==Niu+count[i])return remoteCounts[i]^unsigned(fault==WrongCounter && i==3);
		assert(false);return 0;
	}
	void Complete(const Command& read){
		const unsigned bytes=read[8];for(unsigned i=0;i<bytes/4;i++)Data(read[3]+i*4)=Peer(read[0]+i*4);
		for(unsigned i=0;i<8;i++)counters[i]+=i==1?(bytes+63)/64:unsigned((0x4dU>>i)&1U);
	}
	void Submit(){
		const unsigned index=submitted.size(),at=command[0],bytes=command[8];
		assert(command[2]==G::Peer(worker+1) && command[5]==129 && !command[1] && !command[4]
			&& !command[6] && !command[7] && !command[9] && (bytes==4 || bytes==64 || bytes==256));
		const bool group=queued && ((index>=4 && index<20) || index>=22);
		const unsigned slot=index>=22?index-22:(index-4)&7;
		assert(command[3]==R::Scratch+(at&31)+(group?slot*32:0));
		stream.emplace_back(at,bytes);submitted.push_back(command);
		if(!group){assert(pending.empty());Complete(command);return;}
		assert(bytes==4 && pending.size()==slot);pending.push_back(command);fifoVisibleEmpty=false;
		maxPending=std::max(maxPending,unsigned(pending.size()));
	}
	uint32_t Load(uint32_t at)override{
		if(CodeAddress(at))return l1[Index(at)];
		if(at>=0x6e000 && at<0x180000)return Data(at);
		assert(at>=Niu);const unsigned off=at-Niu;
		if(off<40)return command[off/4];
		if(off==0x40 || off==0x840 || off==0x1040 || off==0x1840 || off==0x68)return 0;
		if(off==0x64){
			if(!fifoVisibleEmpty && pending.size()==8){fifoVisibleEmpty=true;batches++;return 0x10101010;}
			if(fifoVisibleEmpty && !pending.empty() && (pending.size()!=1 || fault!=LostReply)){
				completedDestinations.push_back(pending.back()[3]);Complete(pending.back());pending.pop_back();
			}
			return fifoVisibleEmpty?0x10101010:0x10101010-unsigned(pending.size());
		}
		for(unsigned i=0;i<16;i++)if(off==src[i])return rootSource[i]^unsigned(fault==ChangedOwner && i==1 && submitted.size()==5);
		if(off>=0x240 && off<0x2c0){assert(pending.empty() || fifoVisibleEmpty);return off==0x240?pending.size():0;}
		for(unsigned i=0;i<8;i++)if(off==count[i]){assert(pending.empty());return counters[i];}
		assert(false);return 0;
	}
	void Store(uint32_t at,uint32_t value)override{
		if(at>=0x6e000 && at<0x180000){
			const unsigned countsAddress=Parent+(192+8)*4;
			assert(at<Parent || (at>=countsAddress && at<countsAddress+32 && pending.empty()));
			Data(at)=value;return;
		}
		if(at>=Niu && at<Niu+40){command[(at-Niu)/4]=value;return;}
		assert(at==Niu+0x40 && value==1);Submit();
	}
	bool Run(){
		while(pc!=Return && pc!=image.fault && pc!=0x60000){Step();assert(regs[2]>=0x6e000 && regs[2]<=Parent);}
		return pc==Return;
	}
};

int main(int argc,char** argv){
	assert(argc==3);Image control(argv[1]),candidate(argv[2]);
	for(unsigned worker:{0U,59U,118U})for(bool stop:{false,true}){
		HeadMachine base(control,false,stop,worker,HeadMachine::None),batch(candidate,true,stop,worker,HeadMachine::None);
		assert(base.Run() && batch.Run());assert(base.stream==batch.stream && batch.stream.size()==(stop?32U:30U));
		assert(batch.batches==(stop?0U:3U) && batch.maxPending==(stop?0U:8U) && batch.pending.empty());
		for(unsigned i=0;i<8;i++)assert(base.Frontier(i)==batch.Frontier(i) && batch.Frontier(i)==batch.counters[i]);
		for(unsigned g=0;g<batch.batches;g++)for(unsigned i=1;i<8;i++)
			assert(batch.completedDestinations[g*8+i]<batch.completedDestinations[g*8+i-1]);
	}
	for(auto fault:{HeadMachine::ChangedOwner,HeadMachine::WrongSource,HeadMachine::WrongCounter,HeadMachine::LostReply}){
		HeadMachine batch(candidate,true,false,59,fault);assert(!batch.Run());
		const unsigned expected=fault==HeadMachine::ChangedOwner?5:fault==HeadMachine::WrongCounter?30:12;
		const unsigned published=fault==HeadMachine::ChangedOwner || fault==HeadMachine::WrongSource || fault==HeadMachine::LostReply?4:22;
		assert(batch.stream.size()==expected);
		for(unsigned i=0;i<8;i++){
			uint32_t frontier=i==6?2:i==1?0xfffffff8U:i;
			for(unsigned packet=0;packet<published;packet++)
				frontier+=i==1?(batch.stream[packet].second+63)/64:unsigned((0x4dU>>i)&1U);
			assert(batch.Frontier(i)==frontier);
		}
	}
	puts("PASS actual RV32 ROOT Closed/Scope: same checked wire operation order, three reordered 8-read batches, serial STOP, four faults stop before frontier update; no device or performance proof");
}
