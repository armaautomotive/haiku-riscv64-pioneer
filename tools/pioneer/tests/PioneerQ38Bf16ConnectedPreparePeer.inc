/* SPDX-License-Identifier: MIT */
// Included INSIDE GlobalChainPeer, explicit HOST TEST only. Actual returning
// producer instructions use the SAME scalar/FPU state and cumulative counters
// as its prior Q8/Conv/GDN jobs. These hooks transport bytes, not products.
std::vector<uint8_t> bf16ProducerCode;
bool bf16Producing=false;
unsigned bf16ProducerPackets=0;
const std::vector<uint8_t>* Rom(unsigned a,unsigned& base)const override{
 if(Range(a,0x30000,unsigned(bf16ProducerCode.size()))){base=0x30000;return &bf16ProducerCode;}
 return NewCohortPeer::Rom(a,base);
}
uint32_t Load(uint32_t a)override{
 if(bf16Producing && Range(a,0x90000,3712))return memory.at(a);
 return NewCohortPeer::Load(a);
}
void SubmitActivation()override{
 if(!bf16Producing){FfnPersistentPeer::SubmitActivation();return;}
 namespace BP=PioneerQ38Bf16ProducerLayout;
 ClearTensorState();assert(bytes && !pending);
 const unsigned reads=BP::Reads(descriptor.data()),p=bf16ProducerPackets++;
 assert(p<reads+2);
 const bool write=p>=reads,trailer=p==reads+1,rootInput=!write && reads==1;
 const unsigned local=write?(trailer?0x90e00:0x90c00):(p?0x90400:0x90000);
 const unsigned size=write?(trailer?128U:512U):1024U;
 const unsigned offset=write?descriptor[trailer?13:12]:descriptor[10+p];
 const unsigned remote=rootInput?offset:descriptor[4]+offset;
 const unsigned fromRoute=rootInput?129U:descriptor[8];
 const unsigned expected[]={write?local:remote,0,write?selfRoute:fromRoute,write?remote:local,0,
  write?descriptor[8]:selfRoute,0,write?0x212U:0,size,0};
 for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
 for(unsigned i=0;i<size/4;i++){
  if(write)bytes->Put(descriptor[8],remote+i*4,memory.at(local+i*4));
  else memory[local+i*4]=rootInput?bytes->RootWord(remote+i*4):bytes->Get(fromRoute,remote+i*4);
 }
 const unsigned at=write?4:0;
 countBase[at]++;countBase[at+1]+=size/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
}
void ReceiveBf16Producer(){
 namespace BP=PioneerQ38Bf16ProducerLayout;
 assert(!active && (pc==waitPC || pc==waitPC+4));ClearTensorState();
 uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
 assert(memory.at(M::Job+4)==sequence+1);sequence=memory.at(M::Job+4);
 denseActive=gdnActive=convActive=outputActive=false;activation=bf16Producing=true;
 bf16ProducerPackets=0;requests=responses=0;
 for(unsigned i=0;i<8;i++)countBase[i]=physical[i];
 for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(BP::Descriptor+i*4);
 assert(BP::DescriptorShape(descriptor.data()) && descriptor[7]==logicalWorker);
 active=true;
} // Endpoint bookkeeping only: NO PC/register/seal/Tensix/completion setters.
