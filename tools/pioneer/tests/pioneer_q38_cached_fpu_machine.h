/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_Q38_COMPACT_FPU_PIPELINE_MACHINE_ONLY
#include "pioneer_q38_q4_fpu_compact_pipeline_test.cpp"
#include "../PioneerQ38Q4FpuCachedLoader.h"
#include "../PioneerQ38Q4FpuCompactStreamCompute.h"
#include <memory>
using CL=PioneerQ38Q4FpuCachedLayout;
struct CachedIO {
	struct Packet {CL::Read data;unsigned due,buffer,block;};
	CL::Job job;const std::vector<uint8_t>& root;
	unsigned dramBase=CL::DramWeight; // Byte endpoint address-window map,not a descriptor rewrite.
	unsigned width,route,events=0,reads=0,writes=0,ticks=0,issues=0,completed=0,headerWrites=0,headerReads=0,maxQueued=0;
	unsigned fault=0,dropCommand=99,reader=2,overlap=0,jobStart=0,jobs=0,pairBase=0,requestStart=0;
	std::array<unsigned,10> command;std::array<uint32_t,16> counters;
	std::vector<Packet> queued,responses;std::vector<uint8_t> ownedL1,dram;std::vector<uint8_t>& l1;
	unsigned copies[2]={},dataBlock[2]={~0U,~0U};bool ready[2]={};
	unsigned counterSeen=0;uint32_t creditSeen=0;bool emptySeen=false,busySeen=false;uint32_t observed[9]={};
	explicit CachedIO(const CL::Job& selected,const std::vector<uint8_t>& produced,std::vector<uint8_t>* shared=nullptr)
		:job(selected),root(produced),width(job.words[2]),route(job.words[5]),ownedL1(shared?0:0x180000,0xa7),dram(0x300000),l1(shared?*shared:ownedL1) {
		command.fill(0xcccccccc);counters.fill(UINT32_MAX-17);
		for(unsigned i=0;i<dram.size();i++)dram[i]=uint8_t((i*17+i/4608*13)^0x8b);
	}
	void Select(const CL::Job& selected){
		assert(CL::Shape(selected) && queued.empty() && responses.empty() && reader==2 && completed==issues);
		assert(selected.words[6]==job.words[6] && selected.words[7]==job.words[7] && selected.words[11]>job.words[11]);
		// Observer selects the caller's NEXT descriptor,not device memory or
		// counter state. The guest must perform its own normal initialization.
		job=selected;width=job.words[2];route=job.words[5];pairBase=jobs;requestStart=issues;
	}
	void Progress() {
		ticks++;
		if(!queued.empty() && fault!=4) {Packet p=queued.front();queued.erase(queued.begin());p.due=ticks+100;responses.push_back(p);}
		for(auto it=responses.begin();it!=responses.end();) {
			if(it->due>ticks){it++;continue;}
			const auto& p=*it;assert(reader!=p.buffer);
			if(p.data.route!=CL::InputRoot)assert(p.data.source>=dramBase && p.data.source-dramBase<=dram.size()-p.data.bytes);
			const auto* bytes=p.data.route==CL::InputRoot?root.data()+p.data.source:dram.data()+p.data.source-dramBase;
			memcpy(l1.data()+p.data.destination,bytes,p.data.bytes);
			counters[2]++;counters[3]+=p.data.bytes/64;counters[4]++;counters[5]++;counters[14]++;completed++;copies[p.buffer]++;
			if(fault==5)counters[2]++;if(fault==6)counters[3]++;if(fault==7)counters[1]++;
			if(fault==8)counters[4]--;if(reader<2)overlap++;
			it=responses.erase(it);
		}
	}
	unsigned Read(unsigned a) {
		events++;reads++;Progress();
		if(CL::HeaderWord(a)){headerReads++;return l1[a]|unsigned(l1[a+1])<<8|unsigned(l1[a+2])<<16|unsigned(l1[a+3])<<24;}
  if(a==0xffb20044)return ((17<<12)|(12<<19)|job.words[7])^(fault==1?4U:0U);
		if(a==0xffb20048)return 0x10000;if(a==0xffb20148)return job.words[7];
		if((a>=0xffb20118 && a<=0xffb20120) || (a>=0xffb20130 && a<=0xffb20134)){
			unsigned first=a<=0xffb20120?(a-0xffb20118)/4*6:(a-0xffb20130)/4*6,value=0;
			for(unsigned i=0;i<6;i++)value|=(first+i)<<(i*5);return value;
		}
		if(a==0xffb20150 || a==0xffb20154)return 0;
		if(a==0xffb20100)return fault==2?0x4000:0x14000;if(a==0xffb20068)return fault==3?1:0;
		if(a==0xffb20064){if(queued.empty())emptySeen=true;return fault==16?0x11101010:0x10101010-unsigned(queued.size());}
		if(a==0xffb20040){busySeen=true;return fault==9 && issues?2:0;}
		if(a==0xffb20840 || a==0xffb21040 || a==0xffb21840)return fault==10?1:0;
		if(a>=0xffb20240 && a<0xffb202c0){unsigned value=a==0xffb20240?unsigned(responses.size()):(fault==11 && issues?1:0);if(!value)creditSeen|=1U<<((a-0xffb20240)/4);return value;}
		if(a>=0xffb20200 && a<0xffb20240){unsigned index=(a-0xffb20200)/4;const unsigned indices[]={1,2,3,4,5,8,10,12,14};for(unsigned i=0;i<9;i++)if(index==indices[i]){counterSeen|=1U<<i;observed[i]=counters[index];}return counters[index];}
		if(a>=0xffb20000 && a<0xffb20028)return command[(a-0xffb20000)/4]^(fault==13 && a==0xffb20008?1U:0U);
		assert(false && "NO scalar payload read");return 0;
	}
	void Write(unsigned a,unsigned v) {
		events++;writes++;Progress();
		if(CL::HeaderWord(a)){
			if(a==CL::HeaderAddress(0,0)){assert(queued.empty() && responses.empty() && reader==2);jobStart=issues;jobs++;copies[0]=copies[1]=0;ready[0]=ready[1]=false;}
			assert(!v && issues==jobStart);headerWrites++;if(fault!=12 || headerWrites!=1)memset(l1.data()+a,0,4);return;
		}
		if(a==0xffb20040) {
			assert(v==1 && queued.size()<16);unsigned which=(issues-jobStart)/CL::Packets,packet=(issues-jobStart)%CL::Packets,buffer=which&1;
			counterSeen=creditSeen=0;emptySeen=busySeen=false;
			CL::Read expected={};assert(CL::Packet(job,jobs-pairBase-1,which,buffer,packet,expected));
			assert(command[0]==expected.source && command[3]==expected.destination && command[8]==expected.bytes && command[2]==expected.route
				&& !command[1] && !command[4] && command[5]==job.words[7] && !command[6] && !command[7] && !command[9]);
			assert(!(expected.source&63) && !(expected.destination&63) && !(expected.bytes&63) && reader!=buffer);
			if(!packet){copies[buffer]=0;ready[buffer]=false;dataBlock[buffer]=which;}
			queued.push_back({expected,0,buffer,which});issues++;maxQueued=std::max(maxQueued,unsigned(queued.size()));return;
		}
		assert(a>=0xffb20000 && a<0xffb20028);unsigned i=(a-0xffb20000)/4;
		if(!issues && i==dropCommand)return;command[i]=v;
	}
	void Fence(){
		events++;Progress();
		if(issues && queued.empty() && responses.empty() && emptySeen && busySeen && counterSeen==511 && creditSeen==~0U) {
			const unsigned indices[]={1,2,3,4,5,8,10,12,14};bool stable=true;for(unsigned i=0;i<9;i++)stable&=observed[i]==counters[indices[i]];
			if(stable)for(unsigned b=0;b<2;b++)if(copies[b]==CL::Packets)ready[b]=true;
		}
	}
	void Math(unsigned buffer,unsigned block) {
		assert(buffer<2 && reader==buffer && copies[buffer]==CL::Packets && dataBlock[buffer]==block && ready[buffer]);
		for(unsigned i=0;i<40;i++)Progress();
	}
};

// Unchanged Machine::Step;all scalar and tensor/NoC bytes share canonical L1.
struct ServiceMachine:Machine {
 std::vector<uint8_t>& memory;std::vector<uint8_t> code;
 unsigned minSP=0x70000,minStore=0x70000,stores=0;bool stackSeen=false;
 ServiceMachine(const char* directory,std::vector<uint8_t>& bytes):memory(bytes){
  const std::string path=directory;code=Read((path+"/text.bin").c_str());auto boot=Read((path+"/boot.bin").c_str());
  assert(boot.size()==4 && code.size()<=16384);std::fill(std::begin(l1),std::end(l1),0xdeadbeef);regs[27]=0x13579bdf;
  Put(0x6000,::Word(boot,0));for(unsigned i=0;i<code.size();i+=4)Put(0x70000+i,::Word(code,i));
  for(unsigned i=0;i<3;i++)Put(0x17ffc0+i*4,0);
 }
 static bool Range(unsigned a,unsigned begin,unsigned bytes){return a>=begin && a-begin<bytes;}
 unsigned Index(unsigned a)override{
  assert(!(a&3));if(a==0x6000)return 0;
  if(Range(a,0x70000,16384))return 1+(a-0x70000)/4;
  if(Range(a,0x6e000,8192))return 4097+(a-0x6e000)/4;
  if(Range(a,0x74000,128))return 6145+(a-0x74000)/4;
  if(Range(a,0x17ffc0,12))return 6177+(a-0x17ffc0)/4;
  assert(Range(a,0x84000,1024));return 6200+(a-0x84000)/4;
 }
 bool CodeAddress(unsigned a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
 unsigned StepLimit()const override{return 120000000;}
 unsigned Get(unsigned a)const{return ::Word(memory,a);}
 void Put(unsigned a,unsigned v){
  assert(!(a&3) && a+3<memory.size());for(unsigned b=0;b<4;b++)memory[a+b]=uint8_t(v>>(b*8));
  if(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x74000,128) || Range(a,0x17ffc0,12) || Range(a,0x84000,1024))l1[Index(a)]=v;
 }
 void Store(unsigned a,unsigned value)override{
  assert(!(a&3));assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,0xa0000,34816));
  if(Range(a,0x6e000,8192))minStore=std::min(minStore,a);else if(Range(a,0xa0000,34816))stores++;
  Put(a,value);
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x74000,128) || Range(a,0x17ffc0,12) || Range(a,0xb0000,69632));return Get(a);
 }
 virtual void Progress(){}
 bool Execute(unsigned magic){
  while(pc!=0x70038 && pc!=0x70058){
   const unsigned instruction=l1[Index(pc)];Step();assert(regs[27]==0x13579bdf);if(instruction==0x0ff0000f)FenceObserved();Progress();
   if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
  }
  assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
  if(pc==0x70058){assert(Get(0x17ffc8)==((magic&0xffffff)|0x45000000));return false;}
  assert(Get(0x17ffc0)==0x7000c && Get(0x17ffc8)==magic);return true;
 }
 virtual void FenceObserved(){}
};

struct CachedPipeline:CompactFpuPipelineMachine {
 CachedIO* bus=nullptr;unsigned blocks=0,inputBlockBase=0;mutable unsigned readerStart=0;
 unsigned OperandOffset()const override{return ((inputLoads/8)&1)*0x10000;}
 bool OperandReady(unsigned a,unsigned bytes)const override{
  if(a==0x97000)return bytes==1024;
  const unsigned slot=(inputLoads/8)&1;
  const bool valid=bus && bus->ready[slot] && bus->copies[slot]==CL::Packets && bus->dataBlock[slot]==(inputLoads/8-inputBlockBase)%blocks
   && a>=CL::Raw+slot*0x10000 && a+bytes<=0x963c0+slot*0x10000;
  if(valid && a==CL::Metadata+slot*0x10000 && !(compactImports%2)){bus->reader=slot;readerStart=matrixOps;}
  return valid && bus->reader==slot;
 }
};
struct CachedMatrixRV32:ServiceMachine {
 CachedPipeline& native;CachedIO bus;bool awaiting=false;unsigned acks=0,forcedZero=0,dropAckAt=0;
 CachedMatrixRV32(const char* directory,CachedPipeline& engine,const CL::Job& job,const std::vector<uint8_t>& root)
  :ServiceMachine(directory,engine.l1),native(engine),bus(job,root,&engine.l1){native.bus=&bus;native.blocks=job.words[2]/256;native.delay[0]=100003;native.delay[1]=7;}
 void Reenter(const CL::Job& job){
  assert(pc==0x70038 && native.Clean() && !awaiting && !(native.inputLoads%8));
  bus.Select(job);native.blocks=job.words[2]/256;native.inputBlockBase=native.inputLoads/8;
  for(unsigned i=0;i<16;i++)Put(0x74000+i*4,job.words[i]);
  for(unsigned i=0;i<3;i++)Put(0x17ffc0+i*4,0);
  // Only the scalar harness call is reentered. All FPU/SFPU/L1/config,
  // NoC counters/commands and the reserved s11 register remain live.
  pc=0x6000;
 }
 bool InlineTensix()const override{return true;}
 void Progress()override{if(bus.reader<2 && native.matrixOps==native.readerStart+128 && native.Clean())bus.reader=2;bus.Progress();}
 void FenceObserved()override{bus.Fence();}
 void Instruction(unsigned op)override{
  if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
  else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}}
  else native.Op(op);Progress();
 }
 void Store(unsigned a,unsigned value)override{
  assert(!(a&3));
  if(a==0xffe40000){Instruction(value);return;}
  if(a==0xffe000fc){assert(!value);gpr63=0;return;}
  if(Range(a,0xffef0000,224*4)){native.Config((a-0xffef0000)/4,value);Progress();return;}
  if(CL::HeaderWord(a) || Range(a,0xffb20000,0x2000)){bus.Write(a,value);return;}
  assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,CL::Output,1024) || Range(a,CL::Zero-64,1088));
  if(Range(a,0x6e000,8192))minStore=std::min(minStore,a);
  if(Range(a,CL::Output,1024)){assert(native.Clean() && !awaiting && bus.queued.empty() && bus.responses.empty());stores++;}
  if(Range(a,CL::Zero-64,1088)){assert(!value && bus.issues==bus.requestStart);forcedZero++;}
  Put(a,value);Progress();
 }
 unsigned Load(unsigned a)override{
  assert(!(a&3));if(a==0xffe000fc)return gpr63;
  if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
  if(CL::HeaderWord(a) || Range(a,0xffb20000,0x2000))return bus.Read(a);
  if(Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)){
   if(!native.Clean() || awaiting)throw Q4FpuPipelineMachine::Fault{};
   const unsigned row=(a-0xffbd8000)/64,col=a%64/4;assert(!native.config[3] && (row==32 || row==40));native.outputReads++;return native.FloatDst(row,col);
  }
  assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x74000,128) || Range(a,0x17ffc0,12));return Get(a);
 }
};
